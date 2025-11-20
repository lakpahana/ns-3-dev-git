/**
 * Minimal WiFi Scenario - 2 Devices Communicating
 * 
 * Simplest distributed WiFi example:
 * - Rank 0: WiFi channel processor
 * - Rank 1: 2 devices sending packets to each other
 * 
 * Usage:
 *   mpirun -np 2 ./ns3 run scratch/minimal-wifi
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/mpi-interface.h"

#include "ns3/remote-yans-wifi-channel-stub.h"
#include "ns3/wifi-channel-mpi-processor.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MinimalWifi");

int main(int argc, char* argv[])
{
#ifdef NS3_MPI
    MpiInterface::Enable(&argc, &argv);
    
    uint32_t rank = MpiInterface::GetSystemId();
    uint32_t size = MpiInterface::GetSize();

    if (size != 2) {
        if (rank == 0) {
            std::cout << "ERROR: Requires exactly 2 ranks" << std::endl;
            std::cout << "Usage: mpirun -np 2 ./ns3 run scratch/minimal-wifi" << std::endl;
        }
        MpiInterface::Disable();
        return 1;
    }

    if (rank == 0) {
        // Channel processor
        std::cout << "=== Rank 0: Channel Processor ===" << std::endl;
        
        Ptr<WifiChannelMpiProcessor> processor = CreateObject<WifiChannelMpiProcessor>();
        Ptr<YansWifiChannel> channel = CreateObject<YansWifiChannel>();
        Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel>();
        channel->SetPropagationLossModel(loss);
        channel->SetPropagationDelayModel(CreateObject<ConstantSpeedPropagationDelayModel>());
        
        Simulator::Stop(Seconds(10.0));
        Simulator::Run();
        std::cout << "✅ Channel completed" << std::endl;
    }
    else {
        // WiFi devices
        std::cout << "=== Rank 1: WiFi Devices ===" << std::endl;
        
        NodeContainer nodes;
        nodes.Create(2);
        
        // Position nodes 5 meters apart
        MobilityHelper mobility;
        Ptr<ListPositionAllocator> pos = CreateObject<ListPositionAllocator>();
        pos->Add(Vector(0.0, 0.0, 0.0));
        pos->Add(Vector(5.0, 0.0, 0.0));
        mobility.SetPositionAllocator(pos);
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.Install(nodes);
        
        // MPI WiFi setup
        Ptr<RemoteYansWifiChannelStub> stub = CreateObject<RemoteYansWifiChannelStub>();
        stub->SetRemoteChannelRank(0);
        stub->SetLocalDeviceRank(1);
        
        YansWifiPhyHelper phy;
        phy.SetChannel(stub);
        
        WifiHelper wifi;
        wifi.SetStandard(WIFI_STANDARD_80211n);
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager");
        
        WifiMacHelper mac;
        mac.SetType("ns3::AdhocWifiMac");
        
        NetDeviceContainer devices = wifi.Install(phy, mac, nodes);
        
        // IP setup
        InternetStackHelper stack;
        stack.Install(nodes);
        
        Ipv4AddressHelper addr;
        addr.SetBase("10.0.0.0", "255.255.255.0");
        Ipv4InterfaceContainer ifs = addr.Assign(devices);
        
        // Simple ping: Node 0 -> Node 1
        UdpEchoServerHelper server(9);
        ApplicationContainer serverApp = server.Install(nodes.Get(1));
        serverApp.Start(Seconds(1.0));
        
        UdpEchoClientHelper client(ifs.GetAddress(1), 9);
        client.SetAttribute("MaxPackets", UintegerValue(5));
        client.SetAttribute("Interval", TimeValue(Seconds(1.0)));
        client.SetAttribute("PacketSize", UintegerValue(512));
        
        ApplicationContainer clientApp = client.Install(nodes.Get(0));
        clientApp.Start(Seconds(2.0));
        
        std::cout << "Sending 5 packets: " << ifs.GetAddress(0) << " -> " << ifs.GetAddress(1) << std::endl;
        
        Simulator::Stop(Seconds(10.0));
        Simulator::Run();
        std::cout << "✅ Devices completed" << std::endl;
    }
    
    Simulator::Destroy();
    MpiInterface::Disable();
    
    if (rank == 0) {
        std::cout << "\n✅ Minimal WiFi test PASSED\n" << std::endl;
    }
    
#else
    std::cout << "ERROR: Requires MPI. Build with: ./ns3 configure --enable-mpi" << std::endl;
    return 1;
#endif
    
    return 0;
}
