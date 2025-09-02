#include "remote-yans-wifi-channel-stub.h"

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/wifi-net-device.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-phy.h"

#ifdef NS3_MPI
#include "wifi-mpi-interface.h"
#include "wifi-mpi-message.h"
#endif

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

    // Try to initialize MPI connection
#ifdef NS3_MPI
    if (WifiMpi::IsEnabled())
    {
        m_mpiInitialized = true;
        m_loggingFallback = false;
        LogMethodCall("Constructor", "MPI enabled - using real MPI communication");
    }
    else
    {
        m_mpiInitialized = false;
        m_loggingFallback = true;
        LogMethodCall("Constructor", "MPI not available - using logging fallback");
    }
#else
    m_mpiInitialized = false;
    m_loggingFallback = true;
    LogMethodCall("Constructor", "Built without MPI support - using logging fallback");
#endif
}

RemoteYansWifiChannelStub::~RemoteYansWifiChannelStub()
{
    NS_LOG_FUNCTION(this);
    LogMethodCall("Destructor", "Destroying RemoteYansWifiChannelStub");
}

bool
RemoteYansWifiChannelStub::InitializeMpi()
{
    NS_LOG_FUNCTION(this);

#ifdef NS3_MPI
    if (WifiMpi::IsEnabled())
    {
        m_mpiInitialized = true;
        m_loggingFallback = false;
        LogMethodCall("InitializeMpi", "MPI successfully initialized for WiFi channel stub");
        return true;
    }
    else
    {
        m_mpiInitialized = false;
        m_loggingFallback = true;
        LogMethodCall("InitializeMpi", "MPI not enabled - using logging fallback");
        return false;
    }
#else
    m_mpiInitialized = false;
    m_loggingFallback = true;
    LogMethodCall("InitializeMpi", "Built without MPI support - using logging fallback");
    return false;
#endif
}

void
RemoteYansWifiChannelStub::Add(Ptr<YansWifiPhy> phy)
{
    NS_LOG_FUNCTION(this << phy);

    m_addCount++;
    uint32_t deviceId = GetDeviceIdFromPhy(phy);

    // Call parent to maintain local state for compatibility
    YansWifiChannel::Add(phy);

    // Register device with remote channel via MPI
#ifdef NS3_MPI
    if (m_mpiInitialized)
    {
        try
        {
            WifiMpi::SendDeviceRegistration(m_remoteChannelRank, deviceId, m_localDeviceRank);
            LogMethodCall("Add",
                          "Device " + std::to_string(deviceId) +
                              " registered via MPI, Total: " + std::to_string(m_addCount));
        }
        catch (const std::exception& e)
        {
            LogMethodCall("Add",
                          "MPI registration failed for device " + std::to_string(deviceId) + ": " +
                              e.what());
            // Fall back to logging mode
            m_loggingFallback = true;
        }
    }
    else
#endif
    {
        LogMethodCall("Add",
                      "Device " + std::to_string(deviceId) +
                          " (logging fallback), Total: " + std::to_string(m_addCount));
    }
}

void
RemoteYansWifiChannelStub::Send(Ptr<YansWifiPhy> sender,
                                Ptr<const WifiPpdu> ppdu,
                                dBm_u txPower) const
{
    NS_LOG_FUNCTION(this << sender << ppdu << txPower);

    uint32_t deviceId = GetDeviceIdFromPhy(sender);

    // Send transmission request to remote channel via MPI
#ifdef NS3_MPI
    if (m_mpiInitialized && !m_loggingFallback)
    {
        try
        {
            WifiMpi::SendTransmissionRequest(m_remoteChannelRank, deviceId, ppdu, txPower);
            LogMethodCall("Send",
                          "Device " + std::to_string(deviceId) +
                              " transmitted via MPI, Power: " + std::to_string(txPower) + " dBm");
        }
        catch (const std::exception& e)
        {
            LogMethodCall("Send",
                          "MPI transmission failed for device " + std::to_string(deviceId) + ": " +
                              e.what());
            // Don't call parent Send() - just log the failure
        }
    }
    else
#endif
    {
        LogMethodCall("Send",
                      "Device " + std::to_string(deviceId) +
                          " (logging fallback), Power: " + std::to_string(txPower) + " dBm");
        // In distributed mode, we DON'T call parent Send() because the actual
        // channel processing should happen on the remote channel rank
        // YansWifiChannel::Send(sender, ppdu, txPower);  // Commented out for distributed mode
    }
}

void
RemoteYansWifiChannelStub::SetPropagationLossModel(const Ptr<PropagationLossModel> loss)
{
    std::string modelName = loss ? loss->GetTypeId().GetName() : "nullptr";

    // Call parent to maintain local state
    YansWifiChannel::SetPropagationLossModel(loss);

    // Send configuration to remote channel via MPI
#ifdef NS3_MPI
    if (m_mpiInitialized && !m_loggingFallback)
    {
        try
        {
            WifiMpi::SendLossModelConfig(m_remoteChannelRank, loss);
            LogMethodCall("SetPropagationLossModel", "Model " + modelName + " sent via MPI");
        }
        catch (const std::exception& e)
        {
            LogMethodCall("SetPropagationLossModel",
                          "MPI config failed for " + modelName + ": " + e.what());
        }
    }
    else
#endif
    {
        LogMethodCall("SetPropagationLossModel", "Model " + modelName + " (logging fallback)");
    }
}

void
RemoteYansWifiChannelStub::SetPropagationDelayModel(const Ptr<PropagationDelayModel> delay)
{
    std::string modelName = delay ? delay->GetTypeId().GetName() : "nullptr";

    // Call parent to maintain local state
    YansWifiChannel::SetPropagationDelayModel(delay);

    // Send configuration to remote channel via MPI
#ifdef NS3_MPI
    if (m_mpiInitialized && !m_loggingFallback)
    {
        try
        {
            WifiMpi::SendDelayModelConfig(m_remoteChannelRank, delay);
            LogMethodCall("SetPropagationDelayModel", "Model " + modelName + " sent via MPI");
        }
        catch (const std::exception& e)
        {
            LogMethodCall("SetPropagationDelayModel",
                          "MPI config failed for " + modelName + ": " + e.what());
        }
    }
    else
#endif
    {
        LogMethodCall("SetPropagationDelayModel", "Model " + modelName + " (logging fallback)");
    }
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
RemoteYansWifiChannelStub::SendMpiMessageOrLog(uint32_t messageType,
                                               const std::string& details) const
{
    // Fall back to logging simulation
    std::string typeName = "WIFI_MPI_MESSAGE_" + std::to_string(messageType);
    std::ostringstream mpiDetails;
    mpiDetails << typeName << " - Device rank " << m_localDeviceRank << " to channel rank "
               << m_remoteChannelRank << ", " << details;
    LogSimulatedMpiMessage(typeName, mpiDetails.str());
}

uint32_t
RemoteYansWifiChannelStub::GetDeviceIdFromPhy(Ptr<YansWifiPhy> phy) const
{
    if (phy && phy->GetDevice() && phy->GetDevice()->GetNode())
    {
        return phy->GetDevice()->GetNode()->GetId();
    }
    return 0; // Default device ID
}

uint32_t
RemoteYansWifiChannelStub::GetPhyIdFromPhy(Ptr<YansWifiPhy> phy) const
{
    // For now, use a simple mapping based on device ID and frequency
    if (phy && phy->GetDevice())
    {
        uint32_t deviceId = GetDeviceIdFromPhy(phy);
        uint32_t frequency = static_cast<uint32_t>(phy->GetFrequency());
        return (deviceId << 16) | (frequency & 0xFFFF); // Combine device ID and frequency
    }
    return m_addCount; // Fall back to add count
}

void
RemoteYansWifiChannelStub::LogSimulatedMpiMessage(const std::string& messageType,
                                                  const std::string& details) const
{
    m_messageId++;
    std::cout << "[SIMULATED_MPI_MSG #" << m_messageId << "] " << messageType << " - " << details
              << " [SimTime: " << Simulator::Now().GetSeconds() << "s]" << std::endl;
}

void
RemoteYansWifiChannelStub::LogMethodCall(const std::string& method,
                                         const std::string& details) const
{
    std::cout << "[RemoteYansWifiChannelStub] STUB_CALL: " << method;
    if (!details.empty())
    {
        std::cout << " - " << details;
    }
    std::cout << " [SimTime: " << Simulator::Now().GetSeconds() << "s]" << std::endl;
}

uint32_t
RemoteYansWifiChannelStub::GetRemoteChannelRank() const
{
    return m_remoteChannelRank;
}

uint32_t
RemoteYansWifiChannelStub::GetLocalDeviceRank() const
{
    return m_localDeviceRank;
}

} // namespace ns3
