/*
 * Copyright (c) 2024
 *
 * SPDX-Li    static TypeId GetTypeId();

    YansWifiChannelProxy();
    ~YansWifiChannelProxy() override;

    // Override Channel methods
    std::size_t GetNDevices() const override;
    Ptr<NetDevice> GetDevice(std::size_t i) const override;

    // Override YansWifiChannel methods
    void Add(Ptr<YansWifiPhy> phy) override;
    void SetPropagationLossModel(const Ptr<PropagationLossModel> loss) override;
    void SetPropagationDelayModel(const Ptr<PropagationDelayModel> delay) override;
    void Send(Ptr<YansWifiPhy> sender, Ptr<const WifiPpdu> ppdu, dBm_u txPower) const override;
    int64_t AssignStreams(int64_t stream) override;ifier: GPL-2.0-only
 *
 * Author: MPI Channel Development Team
 */

#ifndef YANS_WIFI_CHANNEL_PROXY_H
#define YANS_WIFI_CHANNEL_PROXY_H

#include "ns3/log.h"
#include "ns3/yans-wifi-channel.h"

namespace ns3
{

class YansWifiPhy;
class WifiPpdu;
class PropagationLossModel;
class PropagationDelayModel;

/**
 * @brief A proxy for YansWifiChannel that logs method invocations.
 * @ingroup wifi
 *
 * This class inherits from YansWifiChannel and overrides its virtual methods
 * to log all method calls before forwarding them to the base implementation.
 * This will be useful for debugging and understanding the communication patterns
 * before implementing MPI functionality.
 */
class YansWifiChannelProxy : public YansWifiChannel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    YansWifiChannelProxy();
    ~YansWifiChannelProxy() override;

    // Override Channel methods
    std::size_t GetNDevices() const override;
    Ptr<NetDevice> GetDevice(std::size_t i) const override;

    // Override YansWifiChannel virtual methods
    void Add(Ptr<YansWifiPhy> phy) override;
    void SetPropagationLossModel(const Ptr<PropagationLossModel> loss) override;
    void SetPropagationDelayModel(const Ptr<PropagationDelayModel> delay) override;
    void Send(Ptr<YansWifiPhy> sender, Ptr<const WifiPpdu> ppdu, dBm_u txPower) const override;
    int64_t AssignStreams(int64_t stream) override;

  private:
    /**
     * @brief Counter for tracking number of method calls
     */
    mutable uint32_t m_sendCallCount;
    mutable uint32_t m_addCallCount;
    mutable uint32_t m_getDeviceCallCount;
    mutable uint32_t m_getNDevicesCallCount;

    /**
     * @brief Helper method to log method invocation with details
     * @param methodName Name of the method being called
     * @param details Additional details about the call
     */
    void LogMethodCall(const std::string& methodName, const std::string& details = "") const;
};

} // namespace ns3

#endif /* YANS_WIFI_CHANNEL_PROXY_H */
