/**
 * Simple Local WiFi PCAP Test
 * This creates local WiFi traffic that should generate PCAP files
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LocalWifiPcapTest");

int
main(int argc, char* argv[])
{
    LogComponentEnable("LocalWifiPcapTest", LOG_LEVEL_INFO);

    // Create 2 nodes for simple test
    NodeContainer nodes;
    nodes.Create(2);

    // Set up mobility
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(5.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Create standard WiFi channel (NOT MPI)
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    // Configure WiFi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager");

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    // Enable PCAP capture
    phy.EnablePcap("local-wifi-test", devices);

    NS_LOG_INFO("PCAP capture enabled for local WiFi test");
    NS_LOG_INFO("Expected files: local-wifi-test-0-0.pcap, local-wifi-test-1-0.pcap");

    // Install IP stack
    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Create UDP traffic
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(nodes.Get(1));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    UdpEchoClientHelper echoClient(interfaces.GetAddress(1), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(100));
    echoClient.SetAttribute("Interval", TimeValue(MilliSeconds(100)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(nodes.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(9.0));

    NS_LOG_INFO("Starting local WiFi simulation with UDP traffic");

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    // Check for PCAP files
    NS_LOG_INFO("=== Checking for PCAP files ===");

    std::vector<std::string> expectedFiles = {"local-wifi-test-0-0.pcap",
                                              "local-wifi-test-1-0.pcap"};

    for (const auto& filename : expectedFiles)
    {
        std::ifstream file(filename.c_str());
        if (file.good())
        {
            file.seekg(0, std::ios::end);
            std::streampos fileSize = file.tellg();
            NS_LOG_INFO("✅ Found PCAP: " << filename << " (Size: " << fileSize << " bytes)");
            file.close();
        }
        else
        {
            NS_LOG_ERROR("❌ Missing PCAP: " << filename);
        }
    }

    Simulator::Destroy();

    NS_LOG_INFO("Local WiFi PCAP test completed");
    return 0;
}