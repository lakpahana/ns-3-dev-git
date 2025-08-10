/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: MPI Channel Development Team
 */

#ifndef YANS_WIFI_CHANNEL_PROXY_H
#define YANS_WIFI_CHANNEL_PROXY_H

#include "ns3/channel.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-channel.h"

namespace ns3
{

/**
 * @brief A proxy for YansWifiChannel that logs method invocations.
 * @ingroup wifi
 *
 * This class acts as a proxy to YansWifiChannel, intercepting and logging
 * all method calls before forwarding them to the actual channel implementation.
 * This will be useful for debugging and understanding the communication patterns
 * before implementing MPI functionality.
 */
class YansWifiChannelProxy : public Channel
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

    // YansWifiChannel-like methods (not overrides since they're not virtual in base)
    void Add(Ptr<YansWifiPhy> phy);
    void SetPropagationLossModel(const Ptr<PropagationLossModel> loss);
    void SetPropagationDelayModel(const Ptr<PropagationDelayModel> delay);
    void Send(Ptr<YansWifiPhy> sender, Ptr<const WifiPpdu> ppdu, dBm_u txPower) const;
    int64_t AssignStreams(int64_t stream);

  private:
    /**
     * @brief The actual YansWifiChannel instance that does the real work
     */
    Ptr<YansWifiChannel> m_realChannel;

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
