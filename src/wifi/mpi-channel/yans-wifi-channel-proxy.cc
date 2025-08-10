/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: MPI Channel Development Team
 */

#include "yans-wifi-channel-proxy.h"

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/simulator.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-ppdu.h"
#include "ns3/yans-wifi-phy.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("YansWifiChannelProxy");

NS_OBJECT_ENSURE_REGISTERED(YansWifiChannelProxy);

TypeId
YansWifiChannelProxy::GetTypeId()
{
    static TypeId tid = TypeId("ns3::YansWifiChannelProxy")
                            .SetParent<Channel>()
                            .SetGroupName("Wifi")
                            .AddConstructor<YansWifiChannelProxy>();
    return tid;
}

YansWifiChannelProxy::YansWifiChannelProxy()
    : m_sendCallCount(0),
      m_addCallCount(0),
      m_getDeviceCallCount(0),
      m_getNDevicesCallCount(0)
{
    NS_LOG_FUNCTION(this);
    LogMethodCall("Constructor");

    // Create the real channel instance that will do the actual work
    m_realChannel = CreateObject<YansWifiChannel>();
}

YansWifiChannelProxy::~YansWifiChannelProxy()
{
    NS_LOG_FUNCTION(this);
    LogMethodCall("Destructor",
                  "Total calls - Send: " + std::to_string(m_sendCallCount) +
                      ", Add: " + std::to_string(m_addCallCount) +
                      ", GetDevice: " + std::to_string(m_getDeviceCallCount) +
                      ", GetNDevices: " + std::to_string(m_getNDevicesCallCount));
}

void
YansWifiChannelProxy::LogMethodCall(const std::string& methodName, const std::string& details) const
{
    std::string logMessage = "PROXY_CALL: " + methodName;
    if (!details.empty())
    {
        logMessage += " - " + details;
    }
    logMessage += " [SimTime: " + std::to_string(Simulator::Now().GetSeconds()) + "s]";

    NS_LOG_INFO(logMessage);
    // Also output to console for easy tracking
    std::cout << "[YansWifiChannelProxy] " << logMessage << std::endl;
}

std::size_t
YansWifiChannelProxy::GetNDevices() const
{
    m_getNDevicesCallCount++;
    LogMethodCall("GetNDevices", "Call #" + std::to_string(m_getNDevicesCallCount));

    std::size_t result = m_realChannel->GetNDevices();
    LogMethodCall("GetNDevices", "Returning: " + std::to_string(result));
    return result;
}

Ptr<NetDevice>
YansWifiChannelProxy::GetDevice(std::size_t i) const
{
    m_getDeviceCallCount++;
    LogMethodCall("GetDevice",
                  "Call #" + std::to_string(m_getDeviceCallCount) +
                      ", Index: " + std::to_string(i));

    Ptr<NetDevice> result = m_realChannel->GetDevice(i);

    std::string deviceInfo = "NULL";
    if (result)
    {
        deviceInfo = "NodeId: " + std::to_string(result->GetNode()->GetId()) +
                     ", DeviceId: " + std::to_string(result->GetIfIndex());
    }
    LogMethodCall("GetDevice", "Returning device: " + deviceInfo);
    return result;
}

void
YansWifiChannelProxy::Add(Ptr<YansWifiPhy> phy)
{
    m_addCallCount++;

    std::string phyInfo = "NULL";
    if (phy)
    {
        auto device = phy->GetDevice();
        if (device && device->GetNode())
        {
            phyInfo = "NodeId: " + std::to_string(device->GetNode()->GetId()) +
                      ", Channel: " + std::to_string(phy->GetChannelNumber()) +
                      ", Frequency: " + std::to_string(phy->GetFrequency());
        }
        else
        {
            phyInfo = "PHY with no device/node info";
        }
    }

    LogMethodCall("Add", "Call #" + std::to_string(m_addCallCount) + ", PHY: " + phyInfo);

    // Forward to real channel
    m_realChannel->Add(phy);

    LogMethodCall("Add",
                  "PHY added successfully. Total devices: " +
                      std::to_string(m_realChannel->GetNDevices()));
}

void
YansWifiChannelProxy::SetPropagationLossModel(const Ptr<PropagationLossModel> loss)
{
    std::string lossInfo = loss ? loss->GetTypeId().GetName() : "NULL";
    LogMethodCall("SetPropagationLossModel", "Model: " + lossInfo);

    // Forward to real channel
    m_realChannel->SetPropagationLossModel(loss);
}

void
YansWifiChannelProxy::SetPropagationDelayModel(const Ptr<PropagationDelayModel> delay)
{
    std::string delayInfo = delay ? delay->GetTypeId().GetName() : "NULL";
    LogMethodCall("SetPropagationDelayModel", "Model: " + delayInfo);

    // Forward to real channel
    m_realChannel->SetPropagationDelayModel(delay);
}

void
YansWifiChannelProxy::Send(Ptr<YansWifiPhy> sender, Ptr<const WifiPpdu> ppdu, dBm_u txPower) const
{
    m_sendCallCount++;

    std::string senderInfo = "NULL";
    std::string ppduInfo = "NULL";

    if (sender)
    {
        auto device = sender->GetDevice();
        if (device && device->GetNode())
        {
            senderInfo = "NodeId: " + std::to_string(device->GetNode()->GetId()) +
                         ", Channel: " + std::to_string(sender->GetChannelNumber());
        }
        else
        {
            senderInfo = "PHY with no device/node info";
        }
    }

    if (ppdu)
    {
        ppduInfo = "Duration: " + std::to_string(ppdu->GetTxDuration().GetMicroSeconds()) + "us" +
                   ", ChannelWidth: " + std::to_string(ppdu->GetTxChannelWidth()) + " MHz";
    }

    LogMethodCall(
        "Send",
        "Call #" + std::to_string(m_sendCallCount) + ", Sender: " + senderInfo +
            ", TxPower: " + std::to_string(txPower) + " dBm" + ", PPDU: " + ppduInfo +
            ", Total devices on channel: " + std::to_string(m_realChannel->GetNDevices()));

    // Forward to real channel
    m_realChannel->Send(sender, ppdu, txPower);

    LogMethodCall("Send", "Transmission forwarded to real channel");
}

int64_t
YansWifiChannelProxy::AssignStreams(int64_t stream)
{
    LogMethodCall("AssignStreams", "Starting stream: " + std::to_string(stream));

    int64_t result = m_realChannel->AssignStreams(stream);

    LogMethodCall("AssignStreams", "Assigned " + std::to_string(result) + " streams");
    return result;
}

} // namespace ns3
