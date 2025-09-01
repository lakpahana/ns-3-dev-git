#ifndef REMOTE_YANS_WIFI_PHY_STUB_H
#define REMOTE_YANS_WIFI_PHY_STUB_H

#include "ns3/yans-wifi-phy.h"

namespace ns3
{

/**
 * \brief A logging stub representing a remote device on the channel rank
 *
 * This stub runs on the channel rank and logs what would be responses
 * to remote devices. Perfect for testing before implementing MPI.
 */
class RemoteYansWifiPhyStub : public YansWifiPhy
{
  public:
    static TypeId GetTypeId();

    RemoteYansWifiPhyStub();
    virtual ~RemoteYansWifiPhyStub();

    /**
     * \brief Set the simulated remote device rank
     * \param rank The MPI rank where device would run (for logging)
     */
    void SetRemoteDeviceRank(uint32_t rank);

    /**
     * \brief Set the simulated device ID
     * \param deviceId Unique device identifier
     */
    void SetRemoteDeviceId(uint32_t deviceId);

    /**
     * \brief Get the remote device rank
     * \return The simulated device rank
     */
    uint32_t GetRemoteDeviceRank() const;

    /**
     * \brief Get the remote device ID
     * \return The simulated device ID
     */
    uint32_t GetRemoteDeviceId() const;

    /**
     * \brief Simulate an RX event for the remote device
     * \param rxPowerW The received power in Watts
     * \param packetSize The packet size in bytes
     */
    void SimulateRxEvent(double rxPowerW, uint32_t packetSize);

  private:
    uint32_t m_remoteDeviceRank;  //!< Simulated device rank
    uint32_t m_remoteDeviceId;    //!< Simulated device ID
    uint32_t m_rxEventCount;      //!< Count of RX events
    mutable uint32_t m_messageId; //!< Message ID for logging

    /**
     * \brief Log a simulated MPI message to remote device
     * \param messageType Type of message
     * \param details Message details
     */
    void LogSimulatedMpiMessage(const std::string& messageType, const std::string& details) const;

    /**
     * \brief Log method calls
     * \param method Method name
     * \param details Additional details
     */
    void LogMethodCall(const std::string& method, const std::string& details = "");
};

} // namespace ns3

#endif /* REMOTE_YANS_WIFI_PHY_STUB_H */
