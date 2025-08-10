/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

// Our proxy header
#include "ns3/yans-wifi-channel-proxy.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiChannelProxyTest");

int
main(int argc, char* argv[])
{
    // Enable logging
    LogComponentEnable("YansWifiChannelProxy", LOG_LEVEL_ALL);
    LogComponentEnable("WifiChannelProxyTest", LOG_LEVEL_INFO);

    NS_LOG_INFO("=== WiFi Channel Proxy Test ===");

    // Create a simple test just to demonstrate proxy logging
    // Create our proxy channel and test its methods
    Ptr<YansWifiChannelProxy> proxyChannel = CreateObject<YansWifiChannelProxy>();

    NS_LOG_INFO("Testing proxy channel method calls...");

    // Test setting propagation models
    Ptr<LogDistancePropagationLossModel> lossModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();

    proxyChannel->SetPropagationLossModel(lossModel);
    proxyChannel->SetPropagationDelayModel(delayModel);

    // Test GetNDevices (should be 0 initially)
    std::size_t numDevices = proxyChannel->GetNDevices();
    NS_LOG_INFO("Number of devices on channel: " << numDevices);

    // Create a simple WiFi setup to see the proxy in action
    NodeContainer wifiStaNode;
    wifiStaNode.Create(1);
    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    // Use the standard helper to create devices, then we'll manually add them to our proxy
    YansWifiChannelHelper channelHelper = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phyHelper;
    phyHelper.SetChannel(channelHelper.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);

    WifiMacHelper mac;
    Ssid ssid = Ssid("test-ssid");

    // Install on AP
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevices = wifi.Install(phyHelper, mac, wifiApNode);

    // Install on STA
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevices = wifi.Install(phyHelper, mac, wifiStaNode);

    // Get the PHY objects and manually add them to our proxy channel
    Ptr<WifiNetDevice> apWifiDevice = DynamicCast<WifiNetDevice>(apDevices.Get(0));
    Ptr<WifiNetDevice> staWifiDevice = DynamicCast<WifiNetDevice>(staDevices.Get(0));

    Ptr<YansWifiPhy> apPhy = DynamicCast<YansWifiPhy>(apWifiDevice->GetPhy());
    Ptr<YansWifiPhy> staPhy = DynamicCast<YansWifiPhy>(staWifiDevice->GetPhy());

    NS_LOG_INFO("Adding PHY devices to proxy channel...");

    // Add PHYs to our proxy channel (this will show proxy logging)
    if (apPhy)
    {
        proxyChannel->Add(apPhy);
    }
    if (staPhy)
    {
        proxyChannel->Add(staPhy);
    }

    // Test GetNDevices again
    numDevices = proxyChannel->GetNDevices();
    NS_LOG_INFO("Number of devices on proxy channel after adding: " << numDevices);

    // Test GetDevice method
    for (std::size_t i = 0; i < numDevices; ++i)
    {
        Ptr<NetDevice> device = proxyChannel->GetDevice(i);
        NS_LOG_INFO("Retrieved device " << i << " from proxy channel");
    }

    NS_LOG_INFO("=== Proxy logging demonstration completed ===");

    return 0;
}
