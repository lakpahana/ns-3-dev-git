#include "remote-yans-wifi-channel-stub.h"

#include "ns3/log.h"
#include "ns3/node.h"
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
      m_messageId(0)
{
    LogMethodCall("Constructor", "Device-side channel stub created");
}

RemoteYansWifiChannelStub::~RemoteYansWifiChannelStub()
{
    std::ostringstream oss;
    oss << "Stub destroyed. Total: " << m_addCount << " devices added, " << m_sendCount
        << " packets sent, " << m_messageId << " MPI messages simulated";
    LogMethodCall("Destructor", oss.str());
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
    m_addCount++;

    std::ostringstream details;
    details << "PHY #" << m_addCount;
    if (phy && phy->GetDevice() && phy->GetDevice()->GetNode())
    {
        details << ", NodeId: " << phy->GetDevice()->GetNode()->GetId();
    }
    details << ", Frequency: " << (phy ? phy->GetFrequency() : 0) << " MHz";

    LogMethodCall("Add", details.str());

    // Simulate MPI message to remote channel
    std::ostringstream mpiDetails;
    mpiDetails << "DEVICE_REGISTER - Device rank " << m_localDeviceRank
               << " registering PHY with channel rank " << m_remoteChannelRank
               << ", PHY details: " << details.str();
    LogSimulatedMpiMessage("DEVICE_REGISTER", mpiDetails.str());

    // Still call parent to maintain local state for compatibility
    YansWifiChannel::Add(phy);
}

void
RemoteYansWifiChannelStub::Send(Ptr<YansWifiPhy> sender,
                                Ptr<const WifiPpdu> ppdu,
                                dBm_u txPower) const
{
    const_cast<RemoteYansWifiChannelStub*>(this)->m_sendCount++;

    std::ostringstream details;
    details << "Send #" << m_sendCount << ", Power: " << txPower;

    if (ppdu && ppdu->GetPsdu())
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
        details << ", Sender NodeId: " << sender->GetDevice()->GetNode()->GetId();
    }

    const_cast<RemoteYansWifiChannelStub*>(this)->LogMethodCall("Send", details.str());

    // Simulate MPI message to remote channel
    std::ostringstream mpiDetails;
    mpiDetails << "TX_REQUEST - Device rank " << m_localDeviceRank << " sending to channel rank "
               << m_remoteChannelRank << ", " << details.str();
    LogSimulatedMpiMessage("TX_REQUEST", mpiDetails.str());

    // Simulate receiving an RX notification back from channel
    std::string detailsStr = details.str();
    uint32_t localRank = m_localDeviceRank;
    uint32_t remoteRank = m_remoteChannelRank;

    Simulator::Schedule(MicroSeconds(10), [detailsStr, localRank, remoteRank]() {
        std::ostringstream rxDetails;
        rxDetails << "RX_NOTIFICATION - Channel rank " << remoteRank << " notifying device rank "
                  << localRank << " about reception, " << detailsStr;
        std::cout << "[SIMULATED_MPI_MSG] " << rxDetails.str()
                  << " [SimTime: " << Simulator::Now().GetSeconds() << "s]" << std::endl;
    });

    // DON'T call parent Send() - we're simulating remote processing
    // YansWifiChannel::Send(sender, ppdu, txPower);  // Commented out
}

void
RemoteYansWifiChannelStub::SetPropagationLossModel(const Ptr<PropagationLossModel> loss)
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

void
RemoteYansWifiChannelStub::SetPropagationDelayModel(const Ptr<PropagationDelayModel> delay)
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

void
RemoteYansWifiChannelStub::LogSimulatedMpiMessage(const std::string& messageType,
                                                  const std::string& details) const
{
    m_messageId++;
    std::cout << "[SIMULATED_MPI_MSG #" << m_messageId << "] " << messageType << " - " << details
              << " [SimTime: " << Simulator::Now().GetSeconds() << "s]" << std::endl;
}

void
RemoteYansWifiChannelStub::LogMethodCall(const std::string& method, const std::string& details)
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
