/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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

NS_LOG_COMPONENT_DEFINE("WifiChannelProxyAdvancedTest");

/**
 * Custom WiFi helper that allows us to use our proxy channel
 */
class ProxyWifiHelper
{
  public:
    static NetDeviceContainer InstallWithProxy(Ptr<YansWifiChannelProxy> proxyChannel,
                                               const WifiPhyHelper& phyHelper,
                                               const WifiMacHelper& macHelper,
                                               NodeContainer nodes)
    {
        NetDeviceContainer devices;

        for (uint32_t i = 0; i < nodes.GetN(); ++i)
        {
            Ptr<Node> node = nodes.Get(i);

            // Create WifiNetDevice
            Ptr<WifiNetDevice> device = CreateObject<WifiNetDevice>();

            // Create and configure PHY
            Ptr<YansWifiPhy> phy = CreateObject<YansWifiPhy>();

            // Set up basic PHY parameters
            phy->SetDevice(device);
            phy->SetMobility(node->GetObject<MobilityModel>());

            // Add the PHY to our proxy channel
            proxyChannel->Add(phy);

            // Configure the device
            device->SetNode(node);
            device->SetPhy(phy);

            // Install on node
            node->AddDevice(device);
            devices.Add(device);
        }

        return devices;
    }
};

int
main(int argc, char* argv[])
{
    // Enable logging
    LogComponentEnable("YansWifiChannelProxy", LOG_LEVEL_ALL);
    LogComponentEnable("WifiChannelProxyAdvancedTest", LOG_LEVEL_INFO);

    NS_LOG_INFO("=== Advanced WiFi Channel Proxy Test ===");

    // Create nodes
    NodeContainer wifiNodes;
    wifiNodes.Create(3); // Create 3 nodes for more interesting interactions

    // Configure mobility
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));  // Node 0
    positionAlloc->Add(Vector(10.0, 0.0, 0.0)); // Node 1
    positionAlloc->Add(Vector(20.0, 0.0, 0.0)); // Node 2
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiNodes);

    // Create our proxy channel
    Ptr<YansWifiChannelProxy> proxyChannel = CreateObject<YansWifiChannelProxy>();

    // Set up propagation models
    Ptr<LogDistancePropagationLossModel> lossModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();

    proxyChannel->SetPropagationLossModel(lossModel);
    proxyChannel->SetPropagationDelayModel(delayModel);

    // Create a traditional setup for comparison and to demonstrate proxy usage
    YansWifiChannelHelper channelHelper = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phyHelper;
    phyHelper.SetChannel(channelHelper.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac"); // Use adhoc mode for simplicity

    // Install traditional WiFi setup
    NetDeviceContainer devices = wifi.Install(phyHelper, mac, wifiNodes);

    // Now manually add the PHYs to our proxy channel to observe the logging
    NS_LOG_INFO("Adding PHYs to proxy channel for logging demonstration...");

    for (uint32_t i = 0; i < devices.GetN(); ++i)
    {
        Ptr<WifiNetDevice> wifiDevice = DynamicCast<WifiNetDevice>(devices.Get(i));
        Ptr<YansWifiPhy> phy = DynamicCast<YansWifiPhy>(wifiDevice->GetPhy());

        if (phy)
        {
            proxyChannel->Add(phy);
        }
    }

    // Install Internet stack
    InternetStackHelper stack;
    stack.Install(wifiNodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Create applications to generate traffic (which will trigger Send method calls)
    uint16_t port = 9;

    // Install UDP echo server on node 0
    UdpEchoServerHelper echoServer(port);
    ApplicationContainer serverApp = echoServer.Install(wifiNodes.Get(0));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    // Install UDP echo client on node 1 (sending to node 0)
    UdpEchoClientHelper echoClient(interfaces.GetAddress(0), port);
    echoClient.SetAttribute("MaxPackets", UintegerValue(5));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApp1 = echoClient.Install(wifiNodes.Get(1));
    clientApp1.Start(Seconds(2.0));
    clientApp1.Stop(Seconds(7.0));

    // Install another client on node 2 (also sending to node 0)
    ApplicationContainer clientApp2 = echoClient.Install(wifiNodes.Get(2));
    clientApp2.Start(Seconds(3.0));
    clientApp2.Stop(Seconds(8.0));

    // Enable packet tracing to see our proxy in action
    NS_LOG_INFO("Starting simulation to observe proxy channel method calls...");

    // Run simulation
    Simulator::Stop(Seconds(11.0));
    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_INFO("=== Simulation completed ===");
    NS_LOG_INFO("Check the output above to see all the proxy method calls that were logged!");

    return 0;
}
