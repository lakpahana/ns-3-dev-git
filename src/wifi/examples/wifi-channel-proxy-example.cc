/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This example demonstrates the YansWifiChannelProxy functionality
 * by logging all method invocations.
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

// Include our proxy
#include "ns3/yans-wifi-channel-proxy.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiChannelProxyExample");

int
main(int argc, char* argv[])
{
    // Enable logging
    LogComponentEnable("YansWifiChannelProxy", LOG_LEVEL_ALL);
    LogComponentEnable("WifiChannelProxyExample", LOG_LEVEL_INFO);

    Time::SetResolution(Time::NS);

    // Create nodes
    NodeContainer nodes;
    nodes.Create(3);

    // Set up mobility
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(5.0),
                                  "DeltaY",
                                  DoubleValue(10.0),
                                  "GridWidth",
                                  UintegerValue(3),
                                  "LayoutType",
                                  StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Create WiFi setup with our proxy channel
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);

    // Set up the physical layer
    YansWifiPhyHelper wifiPhy;

    // Create our proxy channel instead of regular YansWifiChannel
    Ptr<YansWifiChannelProxy> proxyChannel = CreateObject<YansWifiChannelProxy>();

    // Set propagation models on the proxy (which will forward to real channel)
    Ptr<LogDistancePropagationLossModel> lossModel =
        CreateObject<LogDistancePropagationLossModel>();
    proxyChannel->SetPropagationLossModel(lossModel);

    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    proxyChannel->SetPropagationDelayModel(delayModel);

    wifiPhy.SetChannel(proxyChannel);

    // Set up MAC layer
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");

    // Install WiFi on nodes
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);

    // Set up Internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Set up applications to generate some traffic
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(nodes.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    UdpEchoClientHelper echoClient(interfaces.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(5));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(nodes.Get(1));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(10.0));

    // Another client from node 2
    UdpEchoClientHelper echoClient2(interfaces.GetAddress(0), 9);
    echoClient2.SetAttribute("MaxPackets", UintegerValue(3));
    echoClient2.SetAttribute("Interval", TimeValue(Seconds(1.5)));
    echoClient2.SetAttribute("PacketSize", UintegerValue(512));

    ApplicationContainer clientApps2 = echoClient2.Install(nodes.Get(2));
    clientApps2.Start(Seconds(3.0));
    clientApps2.Stop(Seconds(10.0));

    NS_LOG_INFO("Starting simulation...");
    std::cout << std::endl << "=== WiFi Channel Proxy Example ===" << std::endl;
    std::cout << "Watch for proxy method call logs below:" << std::endl << std::endl;

    Simulator::Stop(Seconds(11.0));
    Simulator::Run();

    std::cout << std::endl << "=== Simulation Complete ===" << std::endl;
    std::cout << "Check the logs above to see all the proxy method calls" << std::endl;

    Simulator::Destroy();
    return 0;
}
