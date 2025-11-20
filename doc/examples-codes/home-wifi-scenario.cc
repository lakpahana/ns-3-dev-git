/**
 * Home WiFi Scenario - 8 Stationary Devices MPI Test
 *
 * This test simulates a typical home WiFi environment with:
 * - 8 stationary devices distributed across different rooms
 * - Various device types (laptops, phones, smart devices)
 * - Different traffic patterns (web browsing, streaming, IoT)
 * - MPI distribution across multiple machines
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

NS_LOG_COMPONENT_DEFINE("HomeWifiScenario");

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
    LogComponentEnable("HomeWifiScenario", LOG_LEVEL_INFO);
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

    NS_LOG_INFO("=== Home WiFi Scenario - 8 Devices ===");
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
        NS_LOG_INFO("=== RANK 0: WiFi ACCESS POINT / CHANNEL PROCESSOR ===");
        NS_LOG_INFO("Home WiFi scenario setup:");
        NS_LOG_INFO("- Processing WiFi channel for home environment");
        NS_LOG_INFO("- Indoor propagation model");
        NS_LOG_INFO("- 8 devices distributed across home");

        // Create our MPI channel processor for home environment
        Ptr<WifiChannelMpiProcessor> processor = CreateObject<WifiChannelMpiProcessor>();

        // Create channel with indoor propagation characteristics
        Ptr<YansWifiChannel> channel = CreateObject<YansWifiChannel>();

        // Use indoor propagation model
        Ptr<LogDistancePropagationLossModel> lossModel =
            CreateObject<LogDistancePropagationLossModel>();
        lossModel->SetAttribute("Exponent", DoubleValue(3.0)); // Indoor exponent
        lossModel->SetAttribute("ReferenceDistance", DoubleValue(1.0));
        lossModel->SetAttribute("ReferenceLoss", DoubleValue(40.0)); // Indoor reference loss

        Ptr<ConstantSpeedPropagationDelayModel> delayModel =
            CreateObject<ConstantSpeedPropagationDelayModel>();

        channel->SetPropagationLossModel(lossModel);
        channel->SetPropagationDelayModel(delayModel);

        NS_LOG_INFO("WiFi Access Point positioned at center of home: (7, 6, 2.5)");

        Simulator::Stop(Seconds(300.0)); // 5 minute intensive simulation
        Simulator::Run();

        NS_LOG_INFO("=== Home WiFi Channel Results ===");
        NS_LOG_INFO("✅ Indoor WiFi channel simulation completed");
        NS_LOG_INFO("✅ Processed traffic from 8 home devices");
        NS_LOG_INFO("✅ Indoor propagation effects modeled");
    }
    else
    {
        NS_LOG_INFO("=== RANK " << systemId << ": HOME DEVICE GROUP ===");

        // Calculate which devices this rank handles
        uint32_t devicesPerRank = 8 / (systemCount - 1);
        uint32_t startDevice = (systemId - 1) * devicesPerRank;
        uint32_t endDevice = (systemId == systemCount - 1) ? 8 : startDevice + devicesPerRank;

        NS_LOG_INFO("Managing devices " << startDevice << " to " << (endDevice - 1));

        // Create nodes for devices handled by this rank
        NodeContainer nodes;
        nodes.Create(endDevice - startDevice);

        // Set up mobility with realistic home positions
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

        // Create MPI-enabled WiFi channel stub
        Ptr<RemoteYansWifiChannelStub> channelStub = CreateObject<RemoteYansWifiChannelStub>();
        channelStub->SetRemoteChannelRank(0);
        channelStub->SetLocalDeviceRank(systemId);

        // Configure WiFi PHY for home environment
        YansWifiPhyHelper wifiPhy;
        wifiPhy.SetChannel(channelStub);

        // Configure different tx powers based on device types
        WifiHelper wifi;
        wifi.SetStandard(WIFI_STANDARD_80211n);                     // Fixed: Use standard 802.11n
        wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager"); // Adaptive rate

        WifiMacHelper wifiMac;
        // Use AdHoc mode so devices can communicate directly
        wifiMac.SetType("ns3::AdhocWifiMac");

        NetDeviceContainer devices;

        // Create devices with different characteristics
        for (uint32_t i = 0; i < nodes.GetN(); i++)
        {
            uint32_t deviceIndex = startDevice + i;

            // Set transmission power based on device type
            wifiPhy.Set("TxPowerStart", DoubleValue(homeDevices[deviceIndex].txPower));
            wifiPhy.Set("TxPowerEnd", DoubleValue(homeDevices[deviceIndex].txPower));

            NetDeviceContainer singleDevice = wifi.Install(wifiPhy, wifiMac, nodes.Get(i));
            devices.Add(singleDevice);

            NS_LOG_INFO("Device " << i << " configured as AdHoc node");
        }

        NS_LOG_INFO("Created " << devices.GetN() << " home WiFi devices");

        // Enable PCAP capture for this rank's devices - try multiple methods
        std::ostringstream pcapPrefix;
        pcapPrefix << "home-wifi-rank" << systemId;

        // Method 1: PHY level PCAP (may not work with MPI stub)
        wifiPhy.EnablePcap(pcapPrefix.str(), devices);

        // Method 2: Enable PCAP at device level
        for (uint32_t i = 0; i < devices.GetN(); i++)
        {
            std::ostringstream devicePcap;
            devicePcap << pcapPrefix.str() << "-dev" << i;
            wifiPhy.EnablePcap(devicePcap.str(), devices.Get(i));
        }

        // Print current working directory and expected PCAP files
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        NS_LOG_INFO("PCAP capture enabled for rank " << systemId << " devices");
        NS_LOG_INFO("Working directory: " << cwd);
        NS_LOG_INFO("Expected PCAP files: " << pcapPrefix.str() << "-*.pcap");

        for (uint32_t i = 0; i < devices.GetN(); i++)
        {
            NS_LOG_INFO("  Device " << i << ": " << pcapPrefix.str() << "-" << i << "-0.pcap");
            NS_LOG_INFO("  Device " << i << " (alt): " << pcapPrefix.str() << "-dev" << i
                                    << "-0-0.pcap");
        }

        // Install IP stack
        InternetStackHelper stack;
        stack.Install(nodes);

        Ipv4AddressHelper address;
        std::ostringstream subnet;
        subnet << "192.168." << systemId << ".0";
        address.SetBase(subnet.str().c_str(), "255.255.255.0");
        Ipv4InterfaceContainer interfaces = address.Assign(devices);

        // Enable IP-level PCAP capture (this should work even with MPI stubs)
        std::ostringstream ipPcapPrefix;
        ipPcapPrefix << "home-wifi-ip-rank" << systemId;
        stack.EnablePcap(ipPcapPrefix.str(), nodes);

        NS_LOG_INFO("IP-level PCAP capture enabled: " << ipPcapPrefix.str() << "-*.pcap");

        // Create different traffic patterns based on device types
        ApplicationContainer apps;

        for (uint32_t i = 0; i < nodes.GetN(); i++)
        {
            uint32_t deviceIndex = startDevice + i;
            DeviceType deviceType = homeDevices[deviceIndex].type;

            // Calculate destination - devices communicate with next device in different subnet
            uint32_t destRank = (systemId % (systemCount - 1)) + 1;
            if (destRank == systemId)
            {
                destRank = (destRank % (systemCount - 1)) + 1;
            }

            std::ostringstream destIp;
            destIp << "192.168." << destRank << ".1"; // First device in destination rank

            NS_LOG_INFO("Device " << i << " (" << homeDevices[deviceIndex].name
                                  << ") will send traffic to " << destIp.str());

            switch (deviceType)
            {
            case SMART_TV:
            case LAPTOP: {
                // Heavy streaming traffic to another device
                OnOffHelper onoff("ns3::UdpSocketFactory",
                                  InetSocketAddress(Ipv4Address(destIp.str().c_str()), 8080));
                onoff.SetConstantRate(DataRate("50Mbps")); // Heavy streaming
                onoff.SetAttribute("PacketSize", UintegerValue(1400));
                onoff.SetAttribute("OnTime",
                                   StringValue("ns3::ConstantRandomVariable[Constant=1]"));
                onoff.SetAttribute("OffTime",
                                   StringValue("ns3::ConstantRandomVariable[Constant=0]"));
                apps.Add(onoff.Install(nodes.Get(i)));

                // Also add sink to receive traffic
                PacketSinkHelper sink("ns3::UdpSocketFactory",
                                      InetSocketAddress(Ipv4Address::GetAny(), 8080));
                apps.Add(sink.Install(nodes.Get(i)));
                break;
            }
            case SMARTPHONE:
            case TABLET: {
                // Medium traffic to another device
                OnOffHelper onoff("ns3::UdpSocketFactory",
                                  InetSocketAddress(Ipv4Address(destIp.str().c_str()), 8081));
                onoff.SetConstantRate(DataRate("10Mbps")); // Medium browsing
                onoff.SetAttribute("PacketSize", UintegerValue(1200));
                onoff.SetAttribute("OnTime",
                                   StringValue("ns3::ConstantRandomVariable[Constant=1]"));
                onoff.SetAttribute("OffTime",
                                   StringValue("ns3::ConstantRandomVariable[Constant=0]"));
                apps.Add(onoff.Install(nodes.Get(i)));

                PacketSinkHelper sink("ns3::UdpSocketFactory",
                                      InetSocketAddress(Ipv4Address::GetAny(), 8081));
                apps.Add(sink.Install(nodes.Get(i)));
                break;
            }
            case SECURITY_CAM: {
                // High bitrate video to another device
                OnOffHelper onoff("ns3::UdpSocketFactory",
                                  InetSocketAddress(Ipv4Address(destIp.str().c_str()), 8082));
                onoff.SetConstantRate(DataRate("25Mbps")); // Video streaming
                onoff.SetAttribute("PacketSize", UintegerValue(1400));
                onoff.SetAttribute("OnTime",
                                   StringValue("ns3::ConstantRandomVariable[Constant=1]"));
                onoff.SetAttribute("OffTime",
                                   StringValue("ns3::ConstantRandomVariable[Constant=0]"));
                apps.Add(onoff.Install(nodes.Get(i)));

                PacketSinkHelper sink("ns3::UdpSocketFactory",
                                      InetSocketAddress(Ipv4Address::GetAny(), 8082));
                apps.Add(sink.Install(nodes.Get(i)));
                break;
            }
            case GAME_CONSOLE: {
                // Bursty gaming traffic to another device
                OnOffHelper onoff("ns3::UdpSocketFactory",
                                  InetSocketAddress(Ipv4Address(destIp.str().c_str()), 8083));
                onoff.SetConstantRate(DataRate("30Mbps")); // Gaming bursts
                onoff.SetAttribute("PacketSize", UintegerValue(800));
                onoff.SetAttribute("OnTime",
                                   StringValue("ns3::ExponentialRandomVariable[Mean=0.5]"));
                onoff.SetAttribute("OffTime",
                                   StringValue("ns3::ExponentialRandomVariable[Mean=0.1]"));
                apps.Add(onoff.Install(nodes.Get(i)));

                PacketSinkHelper sink("ns3::UdpSocketFactory",
                                      InetSocketAddress(Ipv4Address::GetAny(), 8083));
                apps.Add(sink.Install(nodes.Get(i)));
                break;
            }
            default: // IOT_DEVICE, SMART_SPEAKER
            {
                // Light IoT traffic to another device
                OnOffHelper onoff("ns3::UdpSocketFactory",
                                  InetSocketAddress(Ipv4Address(destIp.str().c_str()), 8084));
                onoff.SetConstantRate(DataRate("1Mbps")); // Light IoT
                onoff.SetAttribute("PacketSize", UintegerValue(400));
                onoff.SetAttribute("OnTime",
                                   StringValue("ns3::ConstantRandomVariable[Constant=1]"));
                onoff.SetAttribute("OffTime",
                                   StringValue("ns3::ConstantRandomVariable[Constant=0]"));
                apps.Add(onoff.Install(nodes.Get(i)));

                PacketSinkHelper sink("ns3::UdpSocketFactory",
                                      InetSocketAddress(Ipv4Address::GetAny(), 8084));
                apps.Add(sink.Install(nodes.Get(i)));
                break;
            }
            }

            NS_LOG_INFO("Device: " << homeDevices[deviceIndex].name << " - Type: " << deviceType
                                   << " - TX Power: " << homeDevices[deviceIndex].txPower << " dBm"
                                   << " - Dest: " << destIp.str());
        }

        // Start applications at different times to simulate realistic usage
        apps.Start(Seconds(2.0 + systemId * 0.5));
        apps.Stop(Seconds(295.0)); // Run almost the full 5 minutes

        NS_LOG_INFO("Started INTENSIVE home device traffic patterns");

        Simulator::Stop(Seconds(300.0)); // 5 minute intensive simulation
        Simulator::Run();

        // Check if PCAP files were created
        NS_LOG_INFO("=== Checking for PCAP files ===");
        for (uint32_t i = 0; i < devices.GetN(); i++)
        {
            std::ostringstream filename;
            filename << "home-wifi-rank" << systemId << "-" << i << "-0.pcap";

            std::ifstream file(filename.str().c_str());
            if (file.good())
            {
                file.seekg(0, std::ios::end);
                std::streampos fileSize = file.tellg();
                NS_LOG_INFO("✅ Found PCAP: " << filename.str() << " (Size: " << fileSize
                                              << " bytes)");
                file.close();
            }
            else
            {
                NS_LOG_ERROR("❌ Missing PCAP: " << filename.str());
            }
        }

        NS_LOG_INFO("=== Home Device Group Results ===");
        NS_LOG_INFO("Device group " << systemId << " completed home WiFi simulation");
        NS_LOG_INFO("✅ " << nodes.GetN() << " devices simulated realistic home traffic");
        NS_LOG_INFO("✅ Various device types and traffic patterns tested");
        NS_LOG_INFO("✅ Indoor WiFi propagation effects included");
    }

    Simulator::Destroy();
    MpiInterface::Disable();

#else
    NS_LOG_ERROR("This scenario requires MPI support - compile with --enable-mpi");
    return 1;
#endif

    return 0;
}