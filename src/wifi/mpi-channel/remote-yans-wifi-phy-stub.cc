#include "remote-yans-wifi-phy-stub.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/wifi-psdu.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RemoteYansWifiPhyStub");

NS_OBJECT_ENSURE_REGISTERED(RemoteYansWifiPhyStub);

TypeId
RemoteYansWifiPhyStub::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RemoteYansWifiPhyStub")
                            .SetParent<YansWifiPhy>()
                            .SetGroupName("Wifi")
                            .AddConstructor<RemoteYansWifiPhyStub>();
    return tid;
}

RemoteYansWifiPhyStub::RemoteYansWifiPhyStub()
    : m_remoteDeviceRank(1),
      m_remoteDeviceId(0),
      m_rxEventCount(0),
      m_messageId(0)
{
    LogMethodCall("Constructor", "Channel-side device stub created");
}

RemoteYansWifiPhyStub::~RemoteYansWifiPhyStub()
{
    std::ostringstream oss;
    oss << "Stub destroyed. Total RX events processed: " << m_rxEventCount
        << ", MPI messages sent: " << m_messageId;
    LogMethodCall("Destructor", oss.str());
}

void
RemoteYansWifiPhyStub::SetRemoteDeviceRank(uint32_t rank)
{
    m_remoteDeviceRank = rank;
    LogMethodCall("SetRemoteDeviceRank", "Device rank: " + std::to_string(rank));
}

void
RemoteYansWifiPhyStub::SetRemoteDeviceId(uint32_t deviceId)
{
    m_remoteDeviceId = deviceId;
    LogMethodCall("SetRemoteDeviceId", "Device ID: " + std::to_string(deviceId));
}

uint32_t
RemoteYansWifiPhyStub::GetRemoteDeviceRank() const
{
    return m_remoteDeviceRank;
}

uint32_t
RemoteYansWifiPhyStub::GetRemoteDeviceId() const
{
    return m_remoteDeviceId;
}

void
RemoteYansWifiPhyStub::SimulateRxEvent(double rxPowerW, uint32_t packetSize)
{
    m_rxEventCount++;

    std::ostringstream details;
    details << "RX Event #" << m_rxEventCount << " for device " << m_remoteDeviceId
            << ", Power: " << rxPowerW << "W, Packet size: " << packetSize << " bytes";

    LogMethodCall("SimulateRxEvent", details.str());

    // Simulate MPI message to remote device
    std::ostringstream mpiDetails;
    mpiDetails << "RX_NOTIFICATION - Channel rank 0 sending RX event to device rank "
               << m_remoteDeviceRank << ", " << details.str();
    LogSimulatedMpiMessage("RX_NOTIFICATION", mpiDetails.str());
}

void
RemoteYansWifiPhyStub::LogSimulatedMpiMessage(const std::string& messageType,
                                              const std::string& details) const
{
    m_messageId++;
    std::cout << "[SIMULATED_MPI_MSG #" << m_messageId << "] " << messageType << " - " << details
              << " [SimTime: " << Simulator::Now().GetSeconds() << "s]" << std::endl;
}

void
RemoteYansWifiPhyStub::LogMethodCall(const std::string& method, const std::string& details)
{
    std::cout << "[RemoteYansWifiPhyStub] STUB_CALL: " << method;
    if (!details.empty())
    {
        std::cout << " - " << details;
    }
    std::cout << " [SimTime: " << Simulator::Now().GetSeconds() << "s]" << std::endl;
}

} // namespace ns3
