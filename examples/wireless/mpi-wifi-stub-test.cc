#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/remote-yans-wifi-channel-stub.h"
#include "ns3/remote-yans-wifi-phy-stub.h"
#include "ns3/wifi-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MpiWifiStubTest");

int
main(int argc, char* argv[])
{
    bool verbose = false;
    uint32_t numDevices = 2;
    Time simulationTime = Seconds(5.0);

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "Enable verbose logging", verbose);
    cmd.AddValue("numDevices", "Number of WiFi devices", numDevices);
    cmd.AddValue("simulationTime", "Simulation time", simulationTime);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("MpiWifiStubTest", LOG_LEVEL_ALL);
        LogComponentEnable("RemoteYansWifiChannelStub", LOG_LEVEL_ALL);
        LogComponentEnable("RemoteYansWifiPhyStub", LOG_LEVEL_ALL);
    }

    std::cout << "\n=================================" << std::endl;
    std::cout << "=== MPI WiFi Stub Test ====" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "Testing logging-only MPI stubs without actual MPI" << std::endl;
    std::cout << "This simulates device rank <-> channel rank communication" << std::endl;
    std::cout << "=================================" << std::endl;

    // Create nodes
    NodeContainer wifiNodes;
    wifiNodes.Create(numDevices);

    // Configure WiFi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("OfdmRate6Mbps"),
                                 "ControlMode",
                                 StringValue("OfdmRate6Mbps"));

    // Create our MPI stub channel instead of regular channel
    std::cout << "\n=== Creating MPI Stub Channel (simulating device rank 1) ===" << std::endl;

    Ptr<RemoteYansWifiChannelStub> stubChannel = CreateObject<RemoteYansWifiChannelStub>();
    stubChannel->SetLocalDeviceRank(1);   // Simulate we're on device rank 1
    stubChannel->SetRemoteChannelRank(0); // Channel runs on rank 0

    // Set up PHY with our stub channel
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(stubChannel);
    wifiPhy.Set("TxPowerStart", DoubleValue(20.0));
    wifiPhy.Set("TxPowerEnd", DoubleValue(20.0));

    // Configure MAC
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");

    // Install WiFi devices
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, wifiNodes);

    // Set up mobility (stationary)
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    for (uint32_t i = 0; i < numDevices; ++i)
    {
        positionAlloc->Add(Vector(i * 10.0, 0.0, 0.0)); // Space devices 10m apart
    }

    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiNodes);

    // Install Internet stack
    InternetStackHelper stack;
    stack.Install(wifiNodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Set up a simple UDP application to generate traffic
    if (numDevices >= 2)
    {
        std::cout << "\n=== Setting up UDP traffic between nodes ===" << std::endl;

        uint16_t port = 9; // Discard port (RFC 863)

        // UDP server on node 1
        UdpEchoServerHelper echoServer(port);
        ApplicationContainer serverApps = echoServer.Install(wifiNodes.Get(1));
        serverApps.Start(Seconds(1.0));
        serverApps.Stop(simulationTime);

        // UDP client on node 0
        UdpEchoClientHelper echoClient(interfaces.GetAddress(1), port);
        echoClient.SetAttribute("MaxPackets", UintegerValue(5));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
        echoClient.SetAttribute("PacketSize", UintegerValue(1024));

        ApplicationContainer clientApps = echoClient.Install(wifiNodes.Get(0));
        clientApps.Start(Seconds(2.0));
        clientApps.Stop(simulationTime);
    }

    // Test creating channel-side device stubs (simulating what would run on channel rank)
    std::cout << "\n=== Creating Channel-Side Device Stubs (simulating channel rank 0) ==="
              << std::endl;

    for (uint32_t i = 0; i < numDevices; ++i)
    {
        Ptr<RemoteYansWifiPhyStub> deviceStub = CreateObject<RemoteYansWifiPhyStub>();
        deviceStub->SetRemoteDeviceRank(1); // These devices are on rank 1
        deviceStub->SetRemoteDeviceId(i);   // Unique device ID

        std::cout << "Created stub for device " << i << " on rank 1" << std::endl;
    }

    // Schedule some events to show the stub in action
    Simulator::Schedule(Seconds(1.5), []() {
        std::cout << "\n=== @ t=1.5s: About to start packet transmission ===" << std::endl;
    });

    Simulator::Schedule(Seconds(3.0), []() {
        std::cout << "\n=== @ t=3.0s: Mid-simulation status ===" << std::endl;
    });

    // Run simulation
    std::cout << "\n=== Starting Simulation for " << simulationTime.GetSeconds()
              << " seconds ===" << std::endl;

    Simulator::Stop(simulationTime);
    Simulator::Run();

    std::cout << "\n=== Simulation Complete ===" << std::endl;
    std::cout << "Check the logs above to see simulated MPI message flows" << std::endl;
    std::cout << "Each SIMULATED_MPI_MSG represents what would be an actual MPI call" << std::endl;

    Simulator::Destroy();

    std::cout << "\n=== Test Finished Successfully ===" << std::endl;

    return 0;
}
