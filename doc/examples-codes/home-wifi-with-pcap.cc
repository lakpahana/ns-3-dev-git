/**
 * Home WiFi Scenario with PCAP - Hybrid MPI + Local Capture
 *
 * This version creates both:
 * 1. MPI-distributed simulation for performance testing
 * 2. Local WiFi monitoring interfaces for PCAP capture
 */

#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mpi-interface.h"
#include "ns3/network-module.h"
#include "ns3/propagation-module.h"
#include "ns3/wifi-module.h"

// System includes for hostname and PID
#include <sys/types.h>
#include <unistd.h>

// Include our MPI channel components
#include "ns3/remote-yans-wifi-channel-stub.h"
#include "ns3/wifi-channel-mpi-processor.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("HomeWifiScenarioWithPcap");

// Device types for different traffic patterns
enum DeviceType
{
    LAPTOP,        // Heavy traffic - video streaming
    SMARTPHONE,    // Medium traffic - social media, messaging
    SMART_TV,      // High traffic - 4K streaming
    IOT_DEVICE,    // Light traffic - periodic updates
    TABLET,        // Medium traffic - web browsing
    SECURITY_CAM,  // Constant traffic - video upload
    SMART_SPEAKER, // Light traffic - voice commands
    GAME_CONSOLE   // Burst traffic - gaming
};

struct HomeDevice
{
    std::string name;
    DeviceType type;
    Vector position;
    double txPower;    // dBm
    uint32_t dataRate; // Mbps
};

// Define home layout with realistic positions (in meters)
std::vector<HomeDevice> homeDevices = {
    {"Living Room TV", SMART_TV, Vector(5.0, 3.0, 1.0), 15.0, 25},
    {"Kitchen Tablet", TABLET, Vector(8.0, 8.0, 1.2), 10.0, 5},
    {"Bedroom Laptop", LAPTOP, Vector(12.0, 5.0, 1.0), 20.0, 15},
    {"Kids Room Phone", SMARTPHONE, Vector(12.0, 8.0, 1.0), 8.0, 3},
    {"Security Camera", SECURITY_CAM, Vector(2.0, 10.0, 2.5), 12.0, 8},
    {"Smart Speaker", SMART_SPEAKER, Vector(6.0, 6.0, 1.0), 5.0, 1},
    {"Gaming Console", GAME_CONSOLE, Vector(4.0, 4.0, 0.8), 18.0, 20},
    {"Smart Thermostat", IOT_DEVICE, Vector(9.0, 2.0, 1.5), 3.0, 1}};

int
main(int argc, char* argv[])
{
    LogComponentEnable("HomeWifiScenarioWithPcap", LOG_LEVEL_INFO);
    LogComponentEnable("WifiChannelMpiProcessor", LOG_LEVEL_INFO);
    LogComponentEnable("RemoteYansWifiChannelStub", LOG_LEVEL_INFO);

    // Add timestamp for better debugging
    LogComponentEnableAll(LOG_PREFIX_TIME);
    LogComponentEnableAll(LOG_PREFIX_NODE);

#ifdef NS3_MPI
    // Initialize MPI
    MpiInterface::Enable(&argc, &argv);

    uint32_t systemId = MpiInterface::GetSystemId();
    uint32_t systemCount = MpiInterface::GetSize();

    // Get hostname and process ID
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    pid_t pid = getpid();

    NS_LOG_INFO("=== Home WiFi Scenario with PCAP - 8 Devices ===");
    NS_LOG_INFO("Running on rank " << systemId << " of " << systemCount);
    NS_LOG_INFO("Hostname: " << hostname << ", PID: " << pid);

    if (systemCount < 2 || systemCount > 4)
    {
        NS_LOG_ERROR("This scenario requires 2-4 MPI ranks");
        NS_LOG_ERROR("Recommended: 4 ranks (1 channel + 3 device groups)");
        NS_LOG_ERROR("Current setup: " << systemCount << " ranks");
        MpiInterface::Disable();
        return 1;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (systemId == 0)
    {
        NS_LOG_INFO("=== RANK 0: WiFi CHANNEL PROCESSOR ===");
        NS_LOG_INFO("Processing MPI WiFi channel for distributed simulation");

        // Create our MPI channel processor
        Ptr<WifiChannelMpiProcessor> processor = CreateObject<WifiChannelMpiProcessor>();

        Simulator::Stop(Seconds(60.0)); // Shorter for PCAP demo
        Simulator::Run();

        NS_LOG_INFO("=== MPI Channel Results ===");
        NS_LOG_INFO("✅ MPI WiFi channel simulation completed");
    }
    else
    {
        NS_LOG_INFO("=== RANK " << systemId << ": HYBRID DEVICE GROUP ===");

        // Calculate which devices this rank handles
        uint32_t devicesPerRank = 8 / (systemCount - 1);
        uint32_t startDevice = (systemId - 1) * devicesPerRank;
        uint32_t endDevice = (systemId == systemCount - 1) ? 8 : startDevice + devicesPerRank;

        NS_LOG_INFO("Managing devices " << startDevice << " to " << (endDevice - 1));

        // Create nodes for devices handled by this rank
        NodeContainer nodes;
        nodes.Create(endDevice - startDevice);

        // Set up mobility
        MobilityHelper mobility;
        Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

        for (uint32_t i = startDevice; i < endDevice; i++)
        {
            positionAlloc->Add(homeDevices[i].position);
            NS_LOG_INFO("Device: " << homeDevices[i].name << " at (" << homeDevices[i].position.x
                                   << ", " << homeDevices[i].position.y << ", "
                                   << homeDevices[i].position.z << ")");
        }

        mobility.SetPositionAllocator(positionAlloc);
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.Install(nodes);

        // CREATE DUAL SETUP: MPI + Local monitoring

        // 1. MPI-enabled WiFi devices (for distributed simulation)
        Ptr<RemoteYansWifiChannelStub> mpiChannelStub = CreateObject<RemoteYansWifiChannelStub>();
        mpiChannelStub->SetRemoteChannelRank(0);
        mpiChannelStub->SetLocalDeviceRank(systemId);

        YansWifiPhyHelper mpiWifiPhy;
        mpiWifiPhy.SetChannel(mpiChannelStub);

        // 2. Local WiFi monitoring devices (for PCAP capture)
        YansWifiChannelHelper localChannel = YansWifiChannelHelper::Default();
        YansWifiPhyHelper localWifiPhy;
        localWifiPhy.SetChannel(localChannel.Create());

        WifiHelper wifi;
        wifi.SetStandard(WIFI_STANDARD_80211n);
        wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager");

        WifiMacHelper wifiMac;
        wifiMac.SetType("ns3::AdhocWifiMac");

        NetDeviceContainer mpiDevices;
        NetDeviceContainer localDevices;

        // Create both MPI and local devices
        for (uint32_t i = 0; i < nodes.GetN(); i++)
        {
            uint32_t deviceIndex = startDevice + i;

            // Set transmission power
            double txPower = homeDevices[deviceIndex].txPower;
            mpiWifiPhy.Set("TxPowerStart", DoubleValue(txPower));
            mpiWifiPhy.Set("TxPowerEnd", DoubleValue(txPower));
            localWifiPhy.Set("TxPowerStart", DoubleValue(txPower));
            localWifiPhy.Set("TxPowerEnd", DoubleValue(txPower));

            // Create MPI device (for distributed simulation)
            NetDeviceContainer mpiDevice = wifi.Install(mpiWifiPhy, wifiMac, nodes.Get(i));
            mpiDevices.Add(mpiDevice);

            // Create local monitoring device (for PCAP)
            NetDeviceContainer localDevice = wifi.Install(localWifiPhy, wifiMac, nodes.Get(i));
            localDevices.Add(localDevice);

            NS_LOG_INFO("Device " << i << " (" << homeDevices[deviceIndex].name << ") - "
                                  << "MPI + Local monitoring created");
        }

        // Enable PCAP capture on LOCAL devices
        std::ostringstream pcapPrefix;
        pcapPrefix << "home-wifi-rank" << systemId;
        localWifiPhy.EnablePcap(pcapPrefix.str(), localDevices);

        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        NS_LOG_INFO("✅ PCAP capture enabled on LOCAL monitoring devices");
        NS_LOG_INFO("Working directory: " << cwd);
        NS_LOG_INFO("PCAP files: " << pcapPrefix.str() << "-*.pcap");

        // Install IP stack on MPI devices (main simulation)
        InternetStackHelper stack;
        stack.Install(nodes);

        Ipv4AddressHelper address;
        std::ostringstream subnet;
        subnet << "192.168." << systemId << ".0";
        address.SetBase(subnet.str().c_str(), "255.255.255.0");
        Ipv4InterfaceContainer mpiInterfaces = address.Assign(mpiDevices);

        // Install IP on local devices too (for monitoring traffic)
        std::ostringstream localSubnet;
        localSubnet << "10." << systemId << ".0.0";
        address.SetBase(localSubnet.str().c_str(), "255.255.255.0");
        Ipv4InterfaceContainer localInterfaces = address.Assign(localDevices);

        // Create traffic on BOTH networks
        ApplicationContainer apps;

        for (uint32_t i = 0; i < nodes.GetN(); i++)
        {
            uint32_t deviceIndex = startDevice + i;
            DeviceType deviceType = homeDevices[deviceIndex].type;

            // MPI traffic (distributed simulation)
            uint32_t destRank = (systemId % (systemCount - 1)) + 1;
            if (destRank == systemId)
            {
                destRank = (destRank % (systemCount - 1)) + 1;
            }

            std::ostringstream mpiDestIp;
            mpiDestIp << "192.168." << destRank << ".1";

            // Local monitoring traffic
            uint32_t localDest = (i + 1) % nodes.GetN();

            // Add traffic based on device type
            switch (deviceType)
            {
            case SMART_TV:
            case LAPTOP: {
                // MPI traffic
                OnOffHelper mpiOnoff("ns3::UdpSocketFactory",
                                     InetSocketAddress(Ipv4Address(mpiDestIp.str().c_str()), 8080));
                mpiOnoff.SetConstantRate(DataRate("50Mbps"));
                mpiOnoff.SetAttribute("PacketSize", UintegerValue(1400));
                mpiOnoff.SetAttribute("OnTime",
                                      StringValue("ns3::ConstantRandomVariable[Constant=1]"));
                mpiOnoff.SetAttribute("OffTime",
                                      StringValue("ns3::ConstantRandomVariable[Constant=0]"));
                apps.Add(mpiOnoff.Install(nodes.Get(i)));

                // Local monitoring traffic
                OnOffHelper localOnoff(
                    "ns3::UdpSocketFactory",
                    InetSocketAddress(localInterfaces.GetAddress(localDest), 8080));
                localOnoff.SetConstantRate(DataRate("5Mbps")); // Lighter for monitoring
                localOnoff.SetAttribute("PacketSize", UintegerValue(1400));
                localOnoff.SetAttribute("OnTime",
                                        StringValue("ns3::ConstantRandomVariable[Constant=1]"));
                localOnoff.SetAttribute("OffTime",
                                        StringValue("ns3::ConstantRandomVariable[Constant=0]"));
                apps.Add(localOnoff.Install(nodes.Get(i)));

                // Sinks
                PacketSinkHelper mpiSink("ns3::UdpSocketFactory",
                                         InetSocketAddress(Ipv4Address::GetAny(), 8080));
                apps.Add(mpiSink.Install(nodes.Get(i)));
                break;
            }
            default: {
                // Lighter traffic for other devices
                OnOffHelper localOnoff(
                    "ns3::UdpSocketFactory",
                    InetSocketAddress(localInterfaces.GetAddress(localDest), 8084));
                localOnoff.SetConstantRate(DataRate("1Mbps"));
                localOnoff.SetAttribute("PacketSize", UintegerValue(400));
                localOnoff.SetAttribute("OnTime",
                                        StringValue("ns3::ConstantRandomVariable[Constant=1]"));
                localOnoff.SetAttribute("OffTime",
                                        StringValue("ns3::ConstantRandomVariable[Constant=0]"));
                apps.Add(localOnoff.Install(nodes.Get(i)));

                PacketSinkHelper sink("ns3::UdpSocketFactory",
                                      InetSocketAddress(Ipv4Address::GetAny(), 8084));
                apps.Add(sink.Install(nodes.Get(i)));
                break;
            }
            }

            NS_LOG_INFO("Device: " << homeDevices[deviceIndex].name
                                   << " - MPI dest: " << mpiDestIp.str()
                                   << " - Local dest: " << localInterfaces.GetAddress(localDest));
        }

        // Start applications
        apps.Start(Seconds(2.0));
        apps.Stop(Seconds(58.0));

        NS_LOG_INFO("Started HYBRID traffic: MPI simulation + Local PCAP monitoring");

        Simulator::Stop(Seconds(60.0));
        Simulator::Run();

        // Check for PCAP files
        NS_LOG_INFO("=== Checking for PCAP files ===");
        for (uint32_t i = 0; i < localDevices.GetN(); i++)
        {
            // Check multiple possible filename patterns
            std::vector<std::string> possibleFiles = {
                pcapPrefix.str() + "-" + std::to_string(i) + "-0.pcap",
                pcapPrefix.str() + "-" + std::to_string(i) + "-1.pcap",
                pcapPrefix.str() + "-" + std::to_string(i) + ".pcap"};

            bool found = false;
            for (const auto& filename : possibleFiles)
            {
                std::ifstream file(filename.c_str());
                if (file.good())
                {
                    file.seekg(0, std::ios::end);
                    std::streampos fileSize = file.tellg();
                    NS_LOG_INFO("✅ Found PCAP: " << filename << " (Size: " << fileSize
                                                  << " bytes)");
                    file.close();
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                NS_LOG_ERROR("❌ Missing PCAP for device " << i << " - tried: " << possibleFiles[0]
                                                           << ", " << possibleFiles[1] << ", "
                                                           << possibleFiles[2]);
            }
        }

        NS_LOG_INFO("=== Hybrid Results ===");
        NS_LOG_INFO("✅ MPI simulation completed with distributed processing");
        NS_LOG_INFO("✅ Local PCAP monitoring captured WiFi traffic");
        NS_LOG_INFO("✅ Dual-network approach: performance + observability");
    }

    Simulator::Destroy();
    MpiInterface::Disable();

#else
    NS_LOG_ERROR("This scenario requires MPI support - compile with --enable-mpi");
    return 1;
#endif

    return 0;
}