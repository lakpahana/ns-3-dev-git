#ifndef REMOTE_YANS_WIFI_CHANNEL_STUB_H
#define REMOTE_YANS_WIFI_CHANNEL_STUB_H

#include "ns3/channel.h"
#include "ns3/net-device.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/propagation-delay-model.h"
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
 * \brief A logging stub that simulates communication with a remote channel
 *
 * This stub runs on device ranks and logs what would be MPI messages
 * to a remote channel rank. Perfect for testing the architecture before
 * implementing actual MPI communication.
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
     * \brief Get the simulated remote channel rank
     * \return The MPI rank where channel would run
     */
    uint32_t GetRemoteChannelRank() const;

    /**
     * \brief Get this device's simulated rank
     * \return The MPI rank this device would run on
     */
    uint32_t GetLocalDeviceRank() const;

    // Override key methods to log instead of executing
    void Add(Ptr<YansWifiPhy> phy) override;
    void Send(Ptr<YansWifiPhy> sender, Ptr<const WifiPpdu> ppdu, dBm_u txPower) const override;
    void SetPropagationLossModel(const Ptr<PropagationLossModel> loss) override;
    void SetPropagationDelayModel(const Ptr<PropagationDelayModel> delay) override;

  private:
    uint32_t m_remoteChannelRank; //!< Simulated channel rank
    uint32_t m_localDeviceRank;   //!< Simulated device rank
    uint32_t m_sendCount;         //!< Count of send operations
    uint32_t m_addCount;          //!< Count of device additions

    mutable uint32_t m_messageId; //!< Unique message ID for logging

    /**
     * \brief Log a simulated MPI message
     * \param messageType Type of message being "sent"
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

#endif /* REMOTE_YANS_WIFI_CHANNEL_STUB_H */
