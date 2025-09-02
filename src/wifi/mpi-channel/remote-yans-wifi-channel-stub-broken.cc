#include "remote-yans-wifi-channel-stub.h"

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/node-list.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/simulator.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-ppdu.h"
#include "ns3/wifi-psdu.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-phy.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RemoteYansWifiChannelStub");

NS_OBJECT_ENSURE_REGISTERED(RemoteYansWifiChannelStub);

TypeId
RemoteYansWifiChannelStub::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RemoteYansWifiChannelStub")
                            .SetParent<YansWifiChannel>()
                            .SetGroupName("Wifi")
                            .AddConstructor<RemoteYansWifiChannelStub>();
    return tid;
}

RemoteYansWifiChannelStub::RemoteYansWifiChannelStub()
    : m_remoteChannelRank(0),
      m_localDeviceRank(1),
      m_sendCount(0),
      m_addCount(0),
      m_mpiInitialized(false),
      m_loggingFallback(true),
      m_messageId(0)
{
    NS_LOG_FUNCTION(this);
    LogMethodCall("Constructor", "MPI-enabled device-side channel stub created");
    
    // Attempt to initialize MPI automatically
    InitializeMpi();
}

RemoteYansWifiChannelStub::~RemoteYansWifiChannelStub()
{
    NS_LOG_FUNCTION(this);
    std::ostringstream oss;
    oss << "Stub destroyed. Total: " << m_addCount << " devices added, " << m_sendCount
        << " packets sent, " << m_messageId << " MPI messages sent"
        << ", MPI mode: " << (m_mpiInitialized ? "ENABLED" : "FALLBACK");
    LogMethodCall("Destructor", oss.str());
}

bool
RemoteYansWifiChannelStub::InitializeMpi()
{
    NS_LOG_FUNCTION(this);
    
    if (m_mpiInitialized)
    {
        return true; // Already initialized
    }
    
#ifdef NS3_MPI
    // Check if WiFi MPI is available and enabled
    if (!WifiMpi::IsEnabled())
    {
        // Try to initialize with current MPI settings
        if (MpiInterface::IsEnabled())
        {
            uint32_t rank = MpiInterface::GetSystemId();
            uint32_t size = MpiInterface::GetSize();
            
            if (WifiMpi::Initialize(rank, size))
            {
                m_localDeviceRank = rank;
                m_mpiInitialized = true;
                NS_LOG_INFO("WiFi MPI initialized - Rank: " << rank << ", Size: " << size);
                return true;
            }
        }
        
        NS_LOG_WARN("WiFi MPI initialization failed, falling back to logging mode");
        m_mpiInitialized = false;
        return false;
    }
    
    // WiFi MPI already enabled
    m_localDeviceRank = WifiMpi::GetInstance()->GetRank();
    m_mpiInitialized = true;
    NS_LOG_INFO("Using existing WiFi MPI - Rank: " << m_localDeviceRank);
    return true;
#else
    NS_LOG_WARN("MPI not available, using logging mode only");
    m_mpiInitialized = false;
    return false;
#endif // NS3_MPI
}

void
RemoteYansWifiChannelStub::SetLoggingFallback(bool enable)
{
    m_loggingFallback = enable;
    NS_LOG_INFO("Logging fallback " << (enable ? "enabled" : "disabled"));
}

void
RemoteYansWifiChannelStub::SetRemoteChannelRank(uint32_t rank)
{
    m_remoteChannelRank = rank;
    LogMethodCall("SetRemoteChannelRank", "Channel rank: " + std::to_string(rank));
}

void
RemoteYansWifiChannelStub::SetLocalDeviceRank(uint32_t rank)
{
    m_localDeviceRank = rank;
    LogMethodCall("SetLocalDeviceRank", "Device rank: " + std::to_string(rank));
}

void
RemoteYansWifiChannelStub::Add(Ptr<YansWifiPhy> phy)
{
    NS_LOG_FUNCTION(this << phy);
    m_addCount++;

    std::ostringstream details;
    details << "PHY #" << m_addCount;
    uint32_t deviceId = 0;
    uint32_t phyId = m_addCount; // Use add count as PHY ID
    
    if (phy && phy->GetDevice() && phy->GetDevice()->GetNode())
    {
        deviceId = phy->GetDevice()->GetNode()->GetId();
        details << ", NodeId: " << deviceId;
    }
    details << ", Frequency: " << (phy ? phy->GetFrequency() : 0) << " MHz";

    LogMethodCall("Add", details.str());

    // Send real MPI message or fall back to logging
#ifdef NS3_MPI
    if (m_mpiInitialized && WifiMpi::IsEnabled())
    {
        try
        {
            // Send device registration message via MPI
            Ptr<WifiMpiInterface> wifiMpi = WifiMpi::GetInstance();
            uint32_t channelNumber = phy ? phy->GetChannelNumber() : 0;
            uint32_t channelWidth = phy ? phy->GetChannelWidth() : 20;
            uint32_t phyType = phy ? phy->GetTypeId().GetHash() : 0;
            
            wifiMpi->SendDeviceRegister(deviceId, phyId, phyType, channelNumber, channelWidth);
            
            NS_LOG_INFO("Sent MPI device registration for device " << deviceId 
                        << ", PHY " << phyId << " to channel rank " << m_remoteChannelRank);
        }
        catch (const std::exception& e)
        {
            NS_LOG_ERROR("Failed to send MPI device registration: " << e.what());
            if (m_loggingFallback)
            {
#ifdef NS3_MPI
                SendMpiMessageOrLog(WIFI_MPI_DEVICE_REGISTER, details.str());
#else
                SendMpiMessageOrLog(1, details.str());  // 1 = DEVICE_REGISTER
#endif
            }
        }
    }
    else
#endif // NS3_MPI
    {
        // Fall back to logging simulation
        if (m_loggingFallback)
        {
#ifdef NS3_MPI
            SendMpiMessageOrLog(WIFI_MPI_DEVICE_REGISTER, details.str());
#else
            SendMpiMessageOrLog(1, details.str());  // 1 = DEVICE_REGISTER
#endif
        }
    }

    // Still call parent to maintain local state for compatibility in non-distributed mode
    // In full distributed mode, this could be skipped
    YansWifiChannel::Add(phy);
}

void
RemoteYansWifiChannelStub::Send(Ptr<YansWifiPhy> sender,
                                Ptr<const WifiPpdu> ppdu,
                                dBm_u txPower) const
{
    NS_LOG_FUNCTION(this << sender << ppdu << txPower);
    const_cast<RemoteYansWifiChannelStub*>(this)->m_sendCount++;

    std::ostringstream details;
    details << "Send #" << m_sendCount << ", Power: " << txPower;
    
    uint32_t deviceId = GetDeviceIdFromPhy(sender);
    uint32_t phyId = GetPhyIdFromPhy(sender);

    if (ppdu && ppdu->GetPsdu())
    {
        auto psdu = ppdu->GetPsdu();
        details << ", PSDU size: " << psdu->GetSize() << " bytes";

        if (psdu->GetNMpdus() > 0)
    {
        auto psdu = ppdu->GetPsdu();
        details << ", PSDU size: " << psdu->GetSize() << " bytes";

        if (psdu->GetNMpdus() > 0)
        {
            auto packet = psdu->GetPayload(0);
            details << ", Packet size: " << packet->GetSize() << " bytes";
        }
    }

    if (sender && sender->GetDevice() && sender->GetDevice()->GetNode())
    {
        details << ", Sender NodeId: " << deviceId;
    }

    const_cast<RemoteYansWifiChannelStub*>(this)->LogMethodCall("Send", details.str());

    // Send real MPI message or fall back to logging
#ifdef NS3_MPI
    if (m_mpiInitialized && WifiMpi::IsEnabled())
    {
        try
        {
            // Send transmission request via MPI
            Ptr<WifiMpiInterface> wifiMpi = WifiMpi::GetInstance();
            double txPowerW = std::pow(10.0, txPower / 10.0) / 1000.0; // Convert dBm to Watts
            
            // Get TxVector from PPDU if available
            WifiTxVector txVector;
            if (ppdu)
            {
                txVector = ppdu->GetTxVector();
            }
            
            wifiMpi->SendTxRequest(deviceId, phyId, txPowerW, ppdu, txVector);
            
            NS_LOG_INFO("Sent MPI TX request for device " << deviceId 
                        << ", PHY " << phyId << ", power " << txPower << " dBm");
        }
        catch (const std::exception& e)
        {
            NS_LOG_ERROR("Failed to send MPI TX request: " << e.what());
            if (m_loggingFallback)
            {
#ifdef NS3_MPI
                SendMpiMessageOrLog(WIFI_MPI_TX_REQUEST, details.str());
#else
                SendMpiMessageOrLog(3, details.str());  // 3 = TX_REQUEST
#endif
            }
        }
    }
    else
#endif // NS3_MPI
    {
        // Fall back to logging simulation
        if (m_loggingFallback)
        {
#ifdef NS3_MPI
            SendMpiMessageOrLog(WIFI_MPI_TX_REQUEST, details.str());
#else
            SendMpiMessageOrLog(3, details.str());  // 3 = TX_REQUEST
#endif
        }
    }

    // In distributed mode, we DON'T call parent Send() because the actual
    // channel processing happens on the remote channel rank
    // YansWifiChannel::Send(sender, ppdu, txPower);  // Commented out for distributed mode
}

void RemoteYansWifiChannelStub::SetPropagationLossModel(const Ptr<PropagationLossModel> loss)
{
    std::string modelName = loss ? loss->GetTypeId().GetName() : "nullptr";
    LogMethodCall("SetPropagationLossModel", "Model: " + modelName);

    // Simulate MPI message to configure remote channel
    std::ostringstream mpiDetails;
    mpiDetails << "CONFIG_LOSS_MODEL - Device rank " << m_localDeviceRank
               << " configuring loss model on channel rank " << m_remoteChannelRank
               << ", Model: " << modelName;
    LogSimulatedMpiMessage("CONFIG_LOSS_MODEL", mpiDetails.str());

    // Call parent to maintain local state
    YansWifiChannel::SetPropagationLossModel(loss);
}

void RemoteYansWifiChannelStub::SetPropagationDelayModel(const Ptr<PropagationDelayModel> delay)
{
    std::string modelName = delay ? delay->GetTypeId().GetName() : "nullptr";
    LogMethodCall("SetPropagationDelayModel", "Model: " + modelName);

    // Simulate MPI message to configure remote channel
    std::ostringstream mpiDetails;
    mpiDetails << "CONFIG_DELAY_MODEL - Device rank " << m_localDeviceRank
               << " configuring delay model on channel rank " << m_remoteChannelRank
               << ", Model: " << modelName;
    LogSimulatedMpiMessage("CONFIG_DELAY_MODEL", mpiDetails.str());

    // Call parent to maintain local state
    YansWifiChannel::SetPropagationDelayModel(delay);
}

void RemoteYansWifiChannelStub::SendMpiMessageOrLog(uint32_t messageType, const std::string& details) const
{
#ifdef NS3_MPI
    if (m_mpiInitialized && WifiMpi::IsEnabled())
    {
        // Real MPI mode - this is a fallback case, should rarely be used
        NS_LOG_WARN("SendMpiMessageOrLog called in MPI mode - this may indicate an error");
    }
#endif // NS3_MPI
    
    // Fall back to logging simulation
#ifdef NS3_MPI
    std::string typeName = WifiMpiMessageUtils::GetMessageTypeName(messageType);
#else
    std::string typeName = "WIFI_MPI_MESSAGE";
#endif
    std::ostringstream mpiDetails;
    mpiDetails << typeName << " - Device rank " << m_localDeviceRank
               << " to channel rank " << m_remoteChannelRank << ", " << details;
    LogSimulatedMpiMessage(typeName, mpiDetails.str());
}

uint32_t RemoteYansWifiChannelStub::GetDeviceIdFromPhy(Ptr<YansWifiPhy> phy) const
{
    if (phy && phy->GetDevice() && phy->GetDevice()->GetNode())
    {
        return phy->GetDevice()->GetNode()->GetId();
    }
    return 0; // Default device ID
}

uint32_t RemoteYansWifiChannelStub::GetPhyIdFromPhy(Ptr<YansWifiPhy> phy) const
{
    // For now, use a simple mapping based on device ID and frequency
    // In a more sophisticated implementation, this could be based on
    // device interface index or other unique identifier
    if (phy && phy->GetDevice())
    {
        uint32_t deviceId = GetDeviceIdFromPhy(phy);
        uint32_t frequency = static_cast<uint32_t>(phy->GetFrequency());
        return (deviceId << 16) | (frequency & 0xFFFF); // Combine device ID and frequency
    }
    return m_addCount; // Fall back to add count
}

void RemoteYansWifiChannelStub::LogSimulatedMpiMessage(const std::string& messageType,
                                                      const std::string& details) const
{
    m_messageId++;
    std::cout << "[SIMULATED_MPI_MSG #" << m_messageId << "] " << messageType << " - " << details
              << " [SimTime: " << Simulator::Now().GetSeconds() << "s]" << std::endl;
}

void RemoteYansWifiChannelStub::LogMethodCall(const std::string& method, const std::string& details)
{
    std::cout << "[RemoteYansWifiChannelStub] STUB_CALL: " << method;
    if (!details.empty())
    {
        std::cout << " - " << details;
    }
    std::cout << " [SimTime: " << Simulator::Now().GetSeconds() << "s]" << std::endl;
}

uint32_t RemoteYansWifiChannelStub::GetRemoteChannelRank() const
{
    return m_remoteChannelRank;
}

uint32_t RemoteYansWifiChannelStub::GetLocalDeviceRank() const
{
    return m_localDeviceRank;
}

} // namespace ns3
