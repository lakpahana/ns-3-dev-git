/*
 * Distributed Third Script Example using MPI WiFi Implementation
 * 
 * This demonstrates our distributed WiFi architecture by converting the classic
 * third.cc example to run across multiple MPI ranks with mixed node distribution.
 *
 * Network Topology (Distributed):
 * 
 * Rank 0 (Channel + Some Nodes):    Rank 1 (Remaining Nodes):
 *   WiFi Channel Processor            WiFi devices via MPI stub
 *   WiFi AP                          Some WiFi STAs
 *   P2P node 0                       P2P node 1 + CSMA nodes
 *   Some WiFi STAs                   Some WiFi STAs
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mpi-interface.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"

// Include our MPI WiFi components
#ifdef NS3_MPI
#include "ns3/remote-yans-wifi-channel-stub.h"
#include "ns3/wifi-channel-mpi-processor.h"
#endif

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("DistributedThirdExample");

// Helper function to create MPI-aware WiFi channel
Ptr<YansWifiChannel> CreateMpiWifiChannel()
{
#ifdef NS3_MPI
    if (MpiInterface::IsEnabled()) {
        uint32_t rank = MpiInterface::GetSystemId();
        
        if (rank == 0) {
            // Channel rank - create real channel with MPI processor
            YansWifiChannelHelper channelHelper = YansWifiChannelHelper::Default();
            Ptr<YansWifiChannel> channel = channelHelper.Create();
            
            // In a full implementation, we would attach our WifiChannelMpiProcessor here
            // For now, use the regular channel as baseline
            return channel;
        } else {
            // Device rank - create MPI stub
            Ptr<RemoteYansWifiChannelStub> stub = CreateObject<RemoteYansWifiChannelStub>();
            stub->SetRemoteChannelRank(0);
            stub->SetLocalDeviceRank(rank);
            return stub;
        }
    }
#endif
    
    // No MPI - create regular channel
    YansWifiChannelHelper channelHelper = YansWifiChannelHelper::Default();
    return channelHelper.Create();
}

int
main(int argc, char* argv[])
{
    bool verbose = true;
    uint32_t nCsma = 3;
    uint32_t nWifi = 6;  // Increased for better distribution
    bool tracing = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
    cmd.AddValue("nWifi", "Number of wifi STA devices", nWifi);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);

    cmd.Parse(argc, argv);

#ifdef NS3_MPI
    // Enable MPI
    MpiInterface::Enable(&argc, &argv);
    
    uint32_t rank = MpiInterface::GetSystemId();
    uint32_t size = MpiInterface::GetSize();
    
    std::cout << "=== Distributed Third Example ===" << std::endl;
    std::cout << "Running on rank " << rank << " of " << size << std::endl;
    
    // Use distributed simulator for MPI
    GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::DistributedSimulatorImpl"));
#else
    uint32_t rank = 0;
    std::cout << "=== Single-Rank Third Example ===" << std::endl;
#endif

    if (nWifi > 18) {
        std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box" << std::endl;
        return 1;
    }

    if (verbose) {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    // Create nodes based on rank assignment
    NodeContainer allP2pNodes;
    NodeContainer allCsmaNodes; 
    NodeContainer allWifiStaNodes;
    NodeContainer allWifiApNodes;
    
    NodeContainer localP2pNodes;
    NodeContainer localCsmaNodes;
    NodeContainer localWifiStaNodes; 
    NodeContainer localWifiApNodes;

    if (rank == 0) {
        std::cout << "=== RANK 0: Channel + AP + Half the nodes ===" << std::endl;
        
        // Rank 0 gets: P2P node 0, WiFi AP, first half of WiFi STAs
        allP2pNodes.Create(2);
        localP2pNodes.Add(allP2pNodes.Get(0));  // P2P node 0 on rank 0
        
        allWifiApNodes.Add(allP2pNodes.Get(0));  // AP is P2P node 0
        localWifiApNodes.Add(allP2pNodes.Get(0));
        
        // First half of WiFi STAs on rank 0
        allWifiStaNodes.Create(nWifi);
        uint32_t rank0StaCount = nWifi / 2;
        for (uint32_t i = 0; i < rank0StaCount; ++i) {
            localWifiStaNodes.Add(allWifiStaNodes.Get(i));
        }
        
        std::cout << "Rank 0 managing: P2P node 0, WiFi AP, " << rank0StaCount << " WiFi STAs" << std::endl;
        
    } else if (rank == 1) {
        std::cout << "=== RANK 1: P2P + CSMA + Remaining WiFi STAs ===" << std::endl;
        
        // Rank 1 gets: P2P node 1, all CSMA nodes, second half of WiFi STAs
        allP2pNodes.Create(2);
        localP2pNodes.Add(allP2pNodes.Get(1));  // P2P node 1 on rank 1
        
        // All CSMA nodes on rank 1
        allCsmaNodes.Add(allP2pNodes.Get(1));
        localCsmaNodes.Add(allP2pNodes.Get(1));
        
        NodeContainer extraCsmaNodes;
        extraCsmaNodes.Create(nCsma);
        allCsmaNodes.Add(extraCsmaNodes);
        localCsmaNodes.Add(extraCsmaNodes);
        
        // Second half of WiFi STAs on rank 1
        allWifiStaNodes.Create(nWifi);
        uint32_t rank0StaCount = nWifi / 2;
        for (uint32_t i = rank0StaCount; i < nWifi; ++i) {
            localWifiStaNodes.Add(allWifiStaNodes.Get(i));
        }
        
        std::cout << "Rank 1 managing: P2P node 1, " << (nCsma + 1) << " CSMA nodes, " 
                  << (nWifi - rank0StaCount) << " WiFi STAs" << std::endl;
    }

    // P2P connection (only create on both ranks for connectivity)
    NetDeviceContainer p2pDevices;
    if (rank == 0 || rank == 1) {
        PointToPointHelper pointToPoint;
        pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
        pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));
        
        if (!localP2pNodes.GetN() == 0) {
            // Create P2P devices for local nodes only
            p2pDevices = pointToPoint.Install(localP2pNodes);
        }
    }

    // CSMA network (only on rank 1)
    NetDeviceContainer csmaDevices;
    if (rank == 1 && localCsmaNodes.GetN() > 0) {
        CsmaHelper csma;
        csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
        csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
        csmaDevices = csma.Install(localCsmaNodes);
    }

    // WiFi network setup with MPI-aware channel
    NetDeviceContainer localStaDevices;
    NetDeviceContainer localApDevices;
    
    if (localWifiStaNodes.GetN() > 0 || localWifiApNodes.GetN() > 0) {
        // Create MPI-aware WiFi channel
        Ptr<YansWifiChannel> wifiChannel = CreateMpiWifiChannel();
        
        YansWifiPhyHelper phy;
        phy.SetChannel(wifiChannel);

        WifiMacHelper mac;
        Ssid ssid = Ssid("ns-3-ssid");
        WifiHelper wifi;

        // Configure for 802.11g to avoid HT issues
        wifi.SetStandard(WIFI_STANDARD_80211g);
        wifi.SetRemoteStationManager("ns3::ArfWifiManager");

        // Install STA devices
        if (localWifiStaNodes.GetN() > 0) {
            mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
            localStaDevices = wifi.Install(phy, mac, localWifiStaNodes);
            std::cout << "Rank " << rank << ": Installed " << localStaDevices.GetN() << " STA devices" << std::endl;
        }

        // Install AP devices (only on rank 0)
        if (localWifiApNodes.GetN() > 0) {
            mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
            localApDevices = wifi.Install(phy, mac, localWifiApNodes);
            std::cout << "Rank " << rank << ": Installed " << localApDevices.GetN() << " AP devices" << std::endl;
        }
    }

    // Mobility setup
    if (localWifiStaNodes.GetN() > 0) {
        MobilityHelper mobility;

        mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                      "MinX", DoubleValue(0.0),
                                      "MinY", DoubleValue(0.0), 
                                      "DeltaX", DoubleValue(5.0),
                                      "DeltaY", DoubleValue(10.0),
                                      "GridWidth", UintegerValue(3),
                                      "LayoutType", StringValue("RowFirst"));

        // Mobile STAs with position offset per rank for distribution
        mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                                  "Bounds", RectangleValue(Rectangle(-50 + rank * 50, 50 + rank * 50, -50, 50)));
        mobility.Install(localWifiStaNodes);
        
        std::cout << "Rank " << rank << ": Configured mobility for " << localWifiStaNodes.GetN() << " WiFi STAs" << std::endl;
    }

    if (localWifiApNodes.GetN() > 0) {
        MobilityHelper mobility;
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.Install(localWifiApNodes);
        
        std::cout << "Rank " << rank << ": Configured static mobility for WiFi AP" << std::endl;
    }

    // Internet stack installation
    InternetStackHelper stack;
    
    if (localCsmaNodes.GetN() > 0) {
        stack.Install(localCsmaNodes);
    }
    if (localWifiApNodes.GetN() > 0) {
        stack.Install(localWifiApNodes);
    }
    if (localWifiStaNodes.GetN() > 0) {
        stack.Install(localWifiStaNodes);
    }

    // IP address assignment
    Ipv4AddressHelper address;

    // P2P network
    Ipv4InterfaceContainer p2pInterfaces;
    if (p2pDevices.GetN() > 0) {
        address.SetBase("10.1.1.0", "255.255.255.0");
        p2pInterfaces = address.Assign(p2pDevices);
    }

    // CSMA network
    Ipv4InterfaceContainer csmaInterfaces;
    if (csmaDevices.GetN() > 0) {
        address.SetBase("10.1.2.0", "255.255.255.0");
        csmaInterfaces = address.Assign(csmaDevices);
    }

    // WiFi network
    if (localStaDevices.GetN() > 0 || localApDevices.GetN() > 0) {
        address.SetBase("10.1.3.0", "255.255.255.0");
        if (localStaDevices.GetN() > 0) {
            address.Assign(localStaDevices);
        }
        if (localApDevices.GetN() > 0) {
            address.Assign(localApDevices);
        }
    }

    // Applications setup
    if (rank == 1 && localCsmaNodes.GetN() > 0) {
        // UDP Echo Server on last CSMA node (rank 1)
        UdpEchoServerHelper echoServer(9);
        ApplicationContainer serverApps = echoServer.Install(localCsmaNodes.Get(localCsmaNodes.GetN() - 1));
        serverApps.Start(Seconds(1));
        serverApps.Stop(Seconds(10));
        
        std::cout << "Rank 1: Started UDP Echo Server" << std::endl;
    }

    if ((rank == 0 || rank == 1) && localWifiStaNodes.GetN() > 0) {
        // UDP Echo Client on last WiFi STA of each rank
        if (rank == 1 && csmaInterfaces.GetN() > 0) {
            UdpEchoClientHelper echoClient(csmaInterfaces.GetAddress(csmaInterfaces.GetN() - 1), 9);
            echoClient.SetAttribute("MaxPackets", UintegerValue(3));
            echoClient.SetAttribute("Interval", TimeValue(Seconds(1)));
            echoClient.SetAttribute("PacketSize", UintegerValue(1024));

            uint32_t clientStaIndex = localWifiStaNodes.GetN() - 1;
            ApplicationContainer clientApps = echoClient.Install(localWifiStaNodes.Get(clientStaIndex));
            clientApps.Start(Seconds(2));
            clientApps.Stop(Seconds(10));
            
            std::cout << "Rank " << rank << ": Started UDP Echo Client on STA " << clientStaIndex << std::endl;
        }
    }

    // Routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Simulation
    Simulator::Stop(Seconds(10));

    std::cout << "Rank " << rank << ": Starting simulation..." << std::endl;
    
    Simulator::Run();
    Simulator::Destroy();

    std::cout << "Rank " << rank << ": Simulation completed successfully!" << std::endl;

#ifdef NS3_MPI
    MpiInterface::Disable();
#endif

    return 0;
}