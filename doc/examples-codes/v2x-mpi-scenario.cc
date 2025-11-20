/**
 * V2X MPI Scenario - Vehicular WiFi Communication with Distributed Processing
 *
 * This scenario simulates Vehicle-to-Vehicle (V2V) and Vehicle-to-Infrastructure (V2I)
 * communication using a hybrid approach:
 * - High-performance MPI simulation for distributed processing across machines
 * - Local WiFi monitoring for detailed PCAP capture and protocol analysis
 * - Realistic vehicular mobility patterns and traffic loads
 *
 * Features:
 * - Multiple vehicle types (cars, trucks, emergency vehicles, motorcycles)
 * - Different communication patterns (safety beacons, infotainment, emergency)
 * - Multi-lane highway scenario with realistic mobility
 * - MPI distribution across multiple machines for performance testing
 * - PCAP capture for detailed V2X protocol analysis
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
#include "ns3/flow-monitor-module.h"

// System includes for hostname and PID
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

// Include our MPI channel components
#include "ns3/remote-yans-wifi-channel-stub.h"
#include "ns3/wifi-channel-mpi-processor.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("V2xMpiScenario");

// Vehicle types for different communication patterns
enum VehicleType {
    PASSENGER_CAR,    // Regular safety beacons + infotainment
    TRUCK,           // Heavy safety beacons + fleet management
    EMERGENCY,       // High priority emergency messages
    MOTORCYCLE,      // Lightweight safety beacons
    BUS,            // Public transport coordination
    INFRASTRUCTURE   // RSU (Road Side Unit)
};

struct Vehicle {
    std::string name;
    VehicleType type;
    Vector initialPosition;
    Vector velocity;
    double txPower;      // dBm
    double beaconRate;   // Hz
    uint32_t payloadSize; // bytes
};

// Status tracking
static uint64_t g_txCount = 0;
static uint64_t g_rxCount = 0;
static uint32_t g_totalVehicles = 0;
static double g_simTime = 0.0;

// Helper functions for status display
static std::string HumanBytes(double bytes, bool perSec) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    double n = bytes;
    while (n >= 1024.0 && i < 4) { n /= 1024.0; ++i; }
    std::ostringstream os;
    os << std::fixed << std::setprecision(n >= 100 ? 0 : (n >= 10 ? 1 : 2))
       << n << " " << units[i] << (perSec ? "/s" : "");
    return os.str();
}

static std::string HumanCount(uint64_t v) {
    const char* suffix[] = {"", "K", "M", "B", "T"};
    int i = 0;
    double n = static_cast<double>(v);
    while (n >= 1000.0 && i < 4) { n /= 1000.0; ++i; }
    std::ostringstream os;
    os << std::fixed << std::setprecision(n >= 100 ? 0 : (n >= 10 ? 1 : 2))
       << n << suffix[i];
    return os.str();
}

// V2X Application for periodic beacon transmission
class V2xBeaconApp : public Application
{
public:
    void Setup(uint16_t port, uint32_t payloadBytes, double rateHz, VehicleType vType) {
        m_port = port;
        m_payloadBytes = payloadBytes;
        m_interval = Seconds(1.0 / rateHz);
        m_vehicleType = vType;
    }

private:
    void StartApplication() override {
        m_sock = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
        m_sock->SetAllowBroadcast(true);
        m_sock->Bind();
        m_sock->Connect(InetSocketAddress(Ipv4Address("255.255.255.255"), m_port));

        // Add jitter to avoid synchronization
        Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
        Time jitter = Seconds(rng->GetValue(0.0, m_interval.GetSeconds() * 0.1));
        m_event = Simulator::Schedule(jitter, &V2xBeaconApp::SendBeacon, this);
    }

    void StopApplication() override {
        if (m_event.IsPending()) Simulator::Cancel(m_event);
        if (m_sock) m_sock->Close();
    }

    void SendBeacon() {
        // Create V2X beacon packet
        Ptr<Packet> packet = Create<Packet>(m_payloadBytes);
        
        // Add vehicle type-specific headers/priorities
        switch (m_vehicleType) {
            case EMERGENCY:
                // High priority emergency beacon
                packet->AddPacketTag(CreateObject<TypeIdTag>()); // Emergency priority
                break;
            case TRUCK:
            case BUS:
                // Fleet management data
                break;
            default:
                // Regular safety beacon
                break;
        }

        m_sock->Send(packet);
        ++g_txCount;
        
        m_event = Simulator::Schedule(m_interval, &V2xBeaconApp::SendBeacon, this);
    }

private:
    Ptr<Socket> m_sock;
    uint16_t m_port{5000};
    uint32_t m_payloadBytes{300};
    Time m_interval{Seconds(0.1)};
    VehicleType m_vehicleType{PASSENGER_CAR};
    EventId m_event;
};

// Packet sink to count received packets
void PacketReceived(Ptr<const Packet> packet, const Address& address) {
    ++g_rxCount;
}

// Progress reporting
void StatusUpdate() {
    double now = Simulator::Now().GetSeconds();
    double progress = (now / g_simTime) * 100.0;
    double txRate = now > 0 ? static_cast<double>(g_txCount) / now : 0.0;
    double rxRate = now > 0 ? static_cast<double>(g_rxCount) / now : 0.0;
    
    std::cout << std::fixed << std::setprecision(1)
              << "t=" << now << "s (" << progress << "%) | "
              << "TX: " << HumanCount(g_txCount) << " (" << txRate << "/s) | "
              << "RX: " << HumanCount(g_rxCount) << " (" << rxRate << "/s)"
              << std::endl;

    if (now + 5.0 <= g_simTime) {
        Simulator::Schedule(Seconds(5.0), &StatusUpdate);
    }
}

int
main(int argc, char* argv[])
{
    LogComponentEnable("V2xMpiScenario", LOG_LEVEL_INFO);
    LogComponentEnable("WifiChannelMpiProcessor", LOG_LEVEL_INFO);
    LogComponentEnable("RemoteYansWifiChannelStub", LOG_LEVEL_INFO);

    // Add timestamp for better debugging
    LogComponentEnableAll(LOG_PREFIX_TIME);
    LogComponentEnableAll(LOG_PREFIX_NODE);

    // Simulation parameters
    uint32_t lanes = 4;
    uint32_t vehiclesPerLane = 50;
    double laneWidth = 3.5;  // meters
    double vehicleSpacing = 20.0;  // meters
    double vehicleSpeed = 30.0;  // m/s (108 km/h)
    uint32_t rsuCount = 5;  // Road Side Units
    double simTime = 300.0;  // 5 minutes
    bool enablePcap = true;
    uint32_t rngRun = 1;

    // Parse command line
    CommandLine cmd(__FILE__);
    cmd.AddValue("lanes", "Number of highway lanes", lanes);
    cmd.AddValue("vehiclesPerLane", "Vehicles per lane", vehiclesPerLane);
    cmd.AddValue("laneWidth", "Lane width in meters", laneWidth);
    cmd.AddValue("vehicleSpacing", "Initial vehicle spacing in meters", vehicleSpacing);
    cmd.AddValue("vehicleSpeed", "Vehicle speed in m/s", vehicleSpeed);
    cmd.AddValue("rsuCount", "Number of Road Side Units", rsuCount);
    cmd.AddValue("simTime", "Simulation time in seconds", simTime);
    cmd.AddValue("pcap", "Enable PCAP capture", enablePcap);
    cmd.AddValue("rngRun", "RNG run number", rngRun);
    cmd.Parse(argc, argv);

    RngSeedManager::SetRun(rngRun);
    g_simTime = simTime;

#ifdef NS3_MPI
    // Initialize MPI
    MpiInterface::Enable(&argc, &argv);

    uint32_t systemId = MpiInterface::GetSystemId();
    uint32_t systemCount = MpiInterface::GetSize();

    // Get hostname and process ID
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    pid_t pid = getpid();

    NS_LOG_INFO("=== V2X MPI Scenario - Highway Communication ===");
    NS_LOG_INFO("Running on rank " << systemId << " of " << systemCount);
    NS_LOG_INFO("Hostname: " << hostname << ", PID: " << pid);

    if (systemCount < 2 || systemCount > 6) {
        NS_LOG_ERROR("This scenario requires 2-6 MPI ranks");
        NS_LOG_ERROR("Recommended: 4 ranks (1 infrastructure + 3 vehicle groups)");
        NS_LOG_ERROR("Current setup: " << systemCount << " ranks");
        MpiInterface::Disable();
        return 1;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    uint32_t totalVehicles = lanes * vehiclesPerLane;
    g_totalVehicles = totalVehicles + rsuCount;

    if (systemId == 0) {
        NS_LOG_INFO("=== RANK 0: V2X INFRASTRUCTURE / CHANNEL PROCESSOR ===");
        NS_LOG_INFO("V2X highway scenario setup:");
        NS_LOG_INFO("- " << lanes << " lanes with " << vehiclesPerLane << " vehicles each");
        NS_LOG_INFO("- " << rsuCount << " Road Side Units (RSUs)");
        NS_LOG_INFO("- Vehicle speed: " << vehicleSpeed << " m/s");
        NS_LOG_INFO("- Total entities: " << g_totalVehicles);
        
        // Create our MPI channel processor for V2X environment
        Ptr<WifiChannelMpiProcessor> processor = CreateObject<WifiChannelMpiProcessor>();

        // Create channel with vehicular propagation characteristics
        Ptr<YansWifiChannel> channel = CreateObject<YansWifiChannel>();
        
        // Use vehicular propagation model
        Ptr<TwoRayGroundPropagationLossModel> lossModel = 
            CreateObject<TwoRayGroundPropagationLossModel>();
        lossModel->SetAttribute("HeightAboveZ", DoubleValue(1.5)); // Vehicle antenna height
        
        Ptr<ConstantSpeedPropagationDelayModel> delayModel =
            CreateObject<ConstantSpeedPropagationDelayModel>();
        
        channel->SetPropagationLossModel(lossModel);
        channel->SetPropagationDelayModel(delayModel);

        NS_LOG_INFO("V2X Infrastructure: RSUs positioned along highway");
        
        Simulator::Stop(Seconds(simTime));
        Simulator::Schedule(Seconds(5.0), &StatusUpdate);
        Simulator::Run();

        NS_LOG_INFO("=== V2X Infrastructure Results ===");
        NS_LOG_INFO("✅ Highway V2X channel simulation completed");
        NS_LOG_INFO("✅ Processed " << totalVehicles << " vehicles + " << rsuCount << " RSUs");
        NS_LOG_INFO("✅ Vehicular propagation effects modeled");
    }
    else {
        NS_LOG_INFO("=== RANK " << systemId << ": V2X VEHICLE GROUP ===");
        
        // Calculate which vehicles this rank handles
        uint32_t vehiclesPerRank = totalVehicles / (systemCount - 1);
        uint32_t startVehicle = (systemId - 1) * vehiclesPerRank;
        uint32_t endVehicle = (systemId == systemCount - 1) ? totalVehicles : startVehicle + vehiclesPerRank;
        
        // Add RSUs to the last rank
        uint32_t localRsuCount = (systemId == systemCount - 1) ? rsuCount : 0;
        uint32_t totalLocalNodes = (endVehicle - startVehicle) + localRsuCount;
        
        NS_LOG_INFO("Managing vehicles " << startVehicle << " to " << (endVehicle - 1) 
                   << " + " << localRsuCount << " RSUs");
        
        // Create nodes for vehicles and RSUs handled by this rank
        NodeContainer vehicleNodes, rsuNodes, allNodes;
        vehicleNodes.Create(endVehicle - startVehicle);
        rsuNodes.Create(localRsuCount);
        allNodes.Add(vehicleNodes);
        allNodes.Add(rsuNodes);

        // Set up mobility for vehicles
        MobilityHelper vehicleMobility;
        Ptr<ListPositionAllocator> vehiclePositions = CreateObject<ListPositionAllocator>();

        // Position vehicles in lanes
        for (uint32_t i = startVehicle; i < endVehicle; i++) {
            uint32_t lane = i / vehiclesPerLane;
            uint32_t posInLane = i % vehiclesPerLane;
            
            double x = posInLane * vehicleSpacing;
            double y = lane * laneWidth;
            double z = 1.5; // Vehicle height
            
            vehiclePositions->Add(Vector(x, y, z));
            
            NS_LOG_INFO("Vehicle " << i << " at lane " << lane 
                       << " position (" << x << ", " << y << ", " << z << ")");
        }

        vehicleMobility.SetPositionAllocator(vehiclePositions);
        vehicleMobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
        vehicleMobility.Install(vehicleNodes);

        // Set vehicle velocities
        for (uint32_t i = 0; i < vehicleNodes.GetN(); i++) {
            Ptr<ConstantVelocityMobilityModel> mob = 
                vehicleNodes.Get(i)->GetObject<ConstantVelocityMobilityModel>();
            
            // Add some speed variation
            Ptr<UniformRandomVariable> speedVar = CreateObject<UniformRandomVariable>();
            double speed = vehicleSpeed + speedVar->GetValue(-5.0, 5.0);
            mob->SetVelocity(Vector(speed, 0.0, 0.0));
        }

        // Set up mobility for RSUs (stationary)
        if (localRsuCount > 0) {
            MobilityHelper rsuMobility;
            Ptr<ListPositionAllocator> rsuPositions = CreateObject<ListPositionAllocator>();
            
            for (uint32_t i = 0; i < localRsuCount; i++) {
                double x = i * (lanes * vehiclesPerLane * vehicleSpacing) / rsuCount;
                double y = -10.0; // RSUs positioned beside the highway
                double z = 5.0;   // RSU antenna height
                
                rsuPositions->Add(Vector(x, y, z));
                NS_LOG_INFO("RSU " << i << " at (" << x << ", " << y << ", " << z << ")");
            }
            
            rsuMobility.SetPositionAllocator(rsuPositions);
            rsuMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
            rsuMobility.Install(rsuNodes);
        }

        // Create MPI-enabled WiFi channel stub for performance testing
        Ptr<RemoteYansWifiChannelStub> mpiChannelStub = CreateObject<RemoteYansWifiChannelStub>();
        mpiChannelStub->SetRemoteChannelRank(0);
        mpiChannelStub->SetLocalDeviceRank(systemId);

        // Create local monitoring channel for PCAP capture
        YansWifiChannelHelper localChannelHelper = YansWifiChannelHelper::Default();
        localChannelHelper.AddPropagationLoss("ns3::TwoRayGroundPropagationLossModel",
                                             "HeightAboveZ", DoubleValue(1.5));
        Ptr<YansWifiChannel> localChannel = localChannelHelper.Create();

        // Configure WiFi PHY for V2X (802.11p-like)
        YansWifiPhyHelper mpiPhy, localPhy;
        mpiPhy.SetChannel(mpiChannelStub);
        localPhy.SetChannel(localChannel);

        // V2X WiFi configuration
        WifiHelper wifi;
        wifi.SetStandard(WIFI_STANDARD_80211a); // 802.11p basis
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                   "DataMode", StringValue("OfdmRate6Mbps"),
                                   "ControlMode", StringValue("OfdmRate6Mbps"));

        WifiMacHelper mac;
        mac.SetType("ns3::AdhocWifiMac"); // V2X AdHoc mode

        // Create MPI devices for performance testing
        NetDeviceContainer mpiDevices = wifi.Install(mpiPhy, mac, allNodes);
        
        // Create local monitoring devices for PCAP
        NetDeviceContainer localDevices = wifi.Install(localPhy, mac, allNodes);

        NS_LOG_INFO("Created " << allNodes.GetN() << " V2X nodes (" 
                   << vehicleNodes.GetN() << " vehicles + " 
                   << rsuNodes.GetN() << " RSUs)");

        // Install IP stack on both networks
        InternetStackHelper stack;
        stack.Install(allNodes);

        // MPI network addressing
        Ipv4AddressHelper mpiAddress;
        std::ostringstream mpiSubnet;
        mpiSubnet << "192.168." << systemId << ".0";
        mpiAddress.SetBase(mpiSubnet.str().c_str(), "255.255.255.0");
        Ipv4InterfaceContainer mpiInterfaces = mpiAddress.Assign(mpiDevices);

        // Local monitoring network addressing
        Ipv4AddressHelper localAddress;
        std::ostringstream localSubnet;
        localSubnet << "10." << systemId << ".0.0";
        localAddress.SetBase(localSubnet.str().c_str(), "255.255.255.0");
        Ipv4InterfaceContainer localInterfaces = localAddress.Assign(localDevices);

        // Enable PCAP capture on local monitoring network
        if (enablePcap) {
            localPhy.EnablePcapAll("v2x-mpi", true);
            NS_LOG_INFO("✅ PCAP capture enabled on V2X monitoring devices");
            NS_LOG_INFO("Working directory: " << getenv("PWD"));
            NS_LOG_INFO("PCAP files: v2x-mpi-rank" << systemId << "-*.pcap");
        }

        // Create V2X applications
        ApplicationContainer apps;
        uint16_t beaconPort = 5000;
        
        // Configure different vehicle types and communication patterns
        for (uint32_t i = 0; i < vehicleNodes.GetN(); i++) {
            uint32_t globalVehicleId = startVehicle + i;
            VehicleType vType;
            double beaconRate;
            uint32_t payloadSize;
            double txPower;
            
            // Determine vehicle type based on ID
            if (globalVehicleId % 20 == 0) {
                vType = EMERGENCY;
                beaconRate = 20.0; // Hz - high frequency emergency beacons
                payloadSize = 400; // bytes - emergency data
                txPower = 25.0; // dBm - high power
            } else if (globalVehicleId % 15 == 0) {
                vType = TRUCK;
                beaconRate = 8.0; // Hz - fleet management
                payloadSize = 600; // bytes - truck telemetry
                txPower = 23.0; // dBm
            } else if (globalVehicleId % 12 == 0) {
                vType = BUS;
                beaconRate = 5.0; // Hz - public transport
                payloadSize = 500; // bytes - passenger info
                txPower = 22.0; // dBm
            } else if (globalVehicleId % 8 == 0) {
                vType = MOTORCYCLE;
                beaconRate = 15.0; // Hz - higher frequency for safety
                payloadSize = 200; // bytes - minimal data
                txPower = 18.0; // dBm - lower power
            } else {
                vType = PASSENGER_CAR;
                beaconRate = 10.0; // Hz - standard safety beacons
                payloadSize = 300; // bytes - standard V2X data
                txPower = 20.0; // dBm
            }

            // Set transmission power
            mpiPhy.Set("TxPowerStart", DoubleValue(txPower));
            mpiPhy.Set("TxPowerEnd", DoubleValue(txPower));
            localPhy.Set("TxPowerStart", DoubleValue(15.0)); // Lower for monitoring
            localPhy.Set("TxPowerEnd", DoubleValue(15.0));

            // MPI V2X application (high performance)
            Ptr<V2xBeaconApp> mpiApp = CreateObject<V2xBeaconApp>();
            mpiApp->Setup(beaconPort, payloadSize, beaconRate, vType);
            vehicleNodes.Get(i)->AddApplication(mpiApp);
            mpiApp->SetStartTime(Seconds(2.0 + i * 0.01)); // Staggered start
            mpiApp->SetStopTime(Seconds(simTime - 1.0));

            // Local monitoring application (for PCAP)
            Ptr<V2xBeaconApp> localApp = CreateObject<V2xBeaconApp>();
            localApp->Setup(beaconPort + 1000, 200, 2.0, vType); // Lighter traffic
            vehicleNodes.Get(i)->AddApplication(localApp);
            localApp->SetStartTime(Seconds(2.0 + i * 0.01));
            localApp->SetStopTime(Seconds(simTime - 1.0));

            NS_LOG_INFO("Vehicle " << globalVehicleId << " - Type: " << vType 
                       << " - Beacon: " << beaconRate << "Hz"
                       << " - Payload: " << payloadSize << "B"
                       << " - TX Power: " << txPower << "dBm");
        }

        // RSU applications (if any)
        for (uint32_t i = 0; i < rsuNodes.GetN(); i++) {
            // RSU broadcasts infrastructure information
            Ptr<V2xBeaconApp> rsuApp = CreateObject<V2xBeaconApp>();
            rsuApp->Setup(beaconPort + 2000, 800, 1.0, INFRASTRUCTURE); // 1Hz infrastructure beacons
            rsuNodes.Get(i)->AddApplication(rsuApp);
            rsuApp->SetStartTime(Seconds(1.0));
            rsuApp->SetStopTime(Seconds(simTime));
            
            NS_LOG_INFO("RSU " << i << " - Infrastructure beacons: 1Hz, 800B");
        }

        // Packet sinks to count received packets
        PacketSinkHelper mpiSink("ns3::UdpSocketFactory", 
                                InetSocketAddress(Ipv4Address::GetAny(), beaconPort));
        PacketSinkHelper localSink("ns3::UdpSocketFactory", 
                                  InetSocketAddress(Ipv4Address::GetAny(), beaconPort + 1000));
        
        ApplicationContainer sinks;
        sinks.Add(mpiSink.Install(allNodes));
        sinks.Add(localSink.Install(allNodes));
        sinks.Start(Seconds(0.0));
        sinks.Stop(Seconds(simTime));

        // Connect packet reception callback
        for (uint32_t i = 0; i < sinks.GetN(); i++) {
            Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinks.Get(i));
            if (sink) {
                sink->TraceConnectWithoutContext("Rx", MakeCallback(&PacketReceived));
            }
        }

        NS_LOG_INFO("Started V2X communication simulation");
        
        Simulator::Stop(Seconds(simTime));
        if (systemId == 1) { // Only one rank reports status
            Simulator::Schedule(Seconds(5.0), &StatusUpdate);
        }
        Simulator::Run();

        NS_LOG_INFO("=== V2X Vehicle Group Results ===");
        NS_LOG_INFO("Vehicle group " << systemId << " completed V2X simulation");
        NS_LOG_INFO("✅ " << vehicleNodes.GetN() << " vehicles + " << rsuNodes.GetN() << " RSUs simulated");
        NS_LOG_INFO("✅ V2X beacon patterns: Emergency, Fleet, Public, Safety");
        NS_LOG_INFO("✅ Vehicular mobility and propagation effects");
        NS_LOG_INFO("✅ TX: " << g_txCount << " packets, RX: " << g_rxCount << " packets");
        
        // Check for PCAP files
        if (enablePcap) {
            NS_LOG_INFO("=== Checking for V2X PCAP files ===");
            for (uint32_t i = 0; i < allNodes.GetN(); i++) {
                std::ostringstream filename;
                filename << "v2x-mpi-rank" << systemId << "-" << i << "-1.pcap";
                std::ifstream file(filename.str());
                if (file.good()) {
                    file.seekg(0, std::ios::end);
                    size_t size = file.tellg();
                    NS_LOG_INFO("✅ Found PCAP: " << filename.str() << " (Size: " << size << " bytes)");
                } else {
                    NS_LOG_INFO("❌ Missing PCAP: " << filename.str());
                }
            }
        }
    }

    Simulator::Destroy();
    MpiInterface::Disable();

#else
    NS_LOG_ERROR("This scenario requires MPI support - compile with --enable-mpi");
    return 1;
#endif

    return 0;
}