#ifndef REMOTE_YANS_WIFI_CHANNEL_STUB_H
#define REMOTE_YANS_WIFI_CHANNEL_STUB_H

#include "ns3/channel.h"
#include "ns3/net-device.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/propagation-delay-model.h"

#ifdef NS3_MPI
#include "wifi-mpi-interface.h"
#endif
#include "ns3/propagation-loss-model.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-phy.h"

namespace ns3
{

/**
 * \brief MPI-enabled device-side channel stub for distributed WiFi simulation
 *
 * This stub runs on device ranks (1-N) and uses real MPI communication
 * to send WiFi operations to the remote channel rank (0). It replaces
 * direct channel operations with MPI messages, enabling distributed
 * WiFi simulation with functional decomposition.
 */
class RemoteYansWifiChannelStub : public YansWifiChannel
{
  public:
    static TypeId GetTypeId();

    RemoteYansWifiChannelStub();
    virtual ~RemoteYansWifiChannelStub();

    /**
     * \brief Set the simulated remote channel rank
     * \param rank The MPI rank where channel would run (for logging)
     */
    void SetRemoteChannelRank(uint32_t rank);

    /**
     * \brief Set this device's simulated rank
     * \param rank The MPI rank this device would run on (for logging)
     */
    void SetLocalDeviceRank(uint32_t rank);

    /**
     * \brief Get the remote channel rank
     * \return The MPI rank where channel runs (should be 0)
     */
    uint32_t GetRemoteChannelRank() const;

    /**
     * \brief Get this device's rank
     * \return The MPI rank this device runs on
     */
    uint32_t GetLocalDeviceRank() const;

    /**
     * \brief Initialize MPI communication
     * \return True if MPI initialization successful
     */
    bool InitializeMpi();

    /**
     * \brief Enable/disable fallback to logging when MPI is not available
     * \param enable True to enable logging fallback
     */
    void SetLoggingFallback(bool enable);

    // Override key methods to send MPI messages instead of direct execution
    void Add(Ptr<YansWifiPhy> phy) override;
    void Send(Ptr<YansWifiPhy> sender, Ptr<const WifiPpdu> ppdu, dBm_u txPower) const override;
    void SetPropagationLossModel(const Ptr<PropagationLossModel> loss) override;
    void SetPropagationDelayModel(const Ptr<PropagationDelayModel> delay) override;

  private:
    uint32_t m_remoteChannelRank; //!< Channel rank (should be 0)
    uint32_t m_localDeviceRank;   //!< This device's rank
    uint32_t m_sendCount;         //!< Count of send operations
    uint32_t m_addCount;          //!< Count of device additions
    bool m_mpiInitialized;        //!< Whether MPI is initialized
    bool m_loggingFallback;       //!< Whether to fall back to logging

    mutable uint32_t m_messageId; //!< Unique message ID for logging

    /**
     * \brief Send an MPI message or log if MPI not available
     * \param messageType Type of MPI message to send
     * \param details Message details for logging
     */
    void SendMpiMessageOrLog(uint32_t messageType, const std::string& details) const;

    /**
     * \brief Get device ID from PHY
     * \param phy The PHY object
     * \return Device ID (node ID)
     */
    uint32_t GetDeviceIdFromPhy(Ptr<YansWifiPhy> phy) const;

    /**
     * \brief Get PHY ID within device
     * \param phy The PHY object
     * \return PHY ID within the device
     */
    uint32_t GetPhyIdFromPhy(Ptr<YansWifiPhy> phy) const;

    /**
     * \brief Log a simulated MPI message (fallback mode)
     * \param messageType Type of message being "sent"
     * \param details Message details
     */
    void LogSimulatedMpiMessage(const std::string& messageType, const std::string& details) const;

    /**
     * \brief Log method calls
     * \param method Method name
     * \param details Additional details
     */
    void LogMethodCall(const std::string& method, const std::string& details = "") const;
};

} // namespace ns3

#endif /* REMOTE_YANS_WIFI_CHANNEL_STUB_H */
