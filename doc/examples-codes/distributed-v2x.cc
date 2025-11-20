/**
 * Distributed Vehicular WiFi Heavy Scenario
 * 
 * Converted from single-machine to distributed MPI implementation
 * - Rank 0: WiFi channel processor
 * - Other ranks: Vehicle groups with mobility
 * 
 * Features:
 * - Multi-lane highway with moving vehicles
 * - Periodic beacon broadcast (V2X safety messages)
 * - Performance monitoring with progress bars
 * - Optional PCAP capture on monitoring network
 * - Flow statistics tracking
 * 
 * Usage:
 *   mpirun -np 4 --hostfile ~/hosts.txt ./ns3 run "scratch/distributed-v2x \
 *     --lanes=6 --vehPerLane=300 --simTime=900"
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/mpi-interface.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdint>
#include <cmath>
#include <sys/types.h>
#include <unistd.h>

// MPI components
#include "ns3/remote-yans-wifi-channel-stub.h"
#include "ns3/wifi-channel-mpi-processor.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("DistributedVehicularWifi");

// -------- Helpers for status formatting --------
static uint64_t g_txCount = 0;
static uint64_t g_rxCount = 0;
static uint64_t g_expectedTx = 0;
static uint32_t g_payloadBytes = 0;
static uint32_t g_nodes = 0;
static double   g_beaconHz = 0.0;
static const double kStatusEvery = 5.0; // seconds

static std::string HumanBytes(double bpsOrBytesPerSec, bool perSec)
{
  const char* units[] = {"B", "KB", "MB", "GB", "TB"};
  int i = 0;
  double n = bpsOrBytesPerSec;
  while (n >= 1024.0 && i < 4) { n /= 1024.0; ++i; }
  std::ostringstream os;
  os << std::fixed << std::setprecision(n >= 100 ? 0 : (n >= 10 ? 1 : 2))
     << n << " " << units[i] << (perSec ? "/s" : "");
  return os.str();
}

static std::string HumanCount(uint64_t v)
{
  const char* suffix[] = {"", "K", "M", "B", "T"};
  int i = 0;
  double n = static_cast<double>(v);
  while (n >= 1000.0 && i < 4) { n /= 1000.0; ++i; }
  std::ostringstream os;
  os << std::fixed << std::setprecision(n >= 100 ? 0 : (n >= 10 ? 1 : 2))
     << n << suffix[i];
  return os.str();
}

static size_t GetVmRSSKb()
{
  std::ifstream f("/proc/self/status");
  if (!f) return 0;
  std::string line;
  while (std::getline(f, line))
  {
    if (line.rfind("VmRSS:", 0) == 0)
    {
      std::istringstream iss(line);
      std::string key, unit;
      size_t valKb = 0;
      iss >> key >> valKb >> unit;
      return valKb;
    }
  }
  return 0;
}

static void StatusTick(double simTime)
{
  double now = Simulator::Now().GetSeconds();
  double pct = simTime > 0 ? (now / simTime) * 100.0 : 0.0;
  double eta = std::max(0.0, simTime - now);
  double txRate = now > 0 ? static_cast<double>(g_txCount) / now : 0.0;
  double rxRate = now > 0 ? static_cast<double>(g_rxCount) / now : 0.0;
  double bytesPerSec = txRate * static_cast<double>(g_payloadBytes);
  size_t rssKb = GetVmRSSKb();

  std::ostringstream bar;
  int width = 30;
  int fill = static_cast<int>(std::round((pct / 100.0) * width));
  bar << "[";
  for (int i = 0; i < width; ++i) bar << (i < fill ? '#' : '-');
  bar << "]";

  std::cout << std::fixed << std::setprecision(1)
            << "t=" << now << "s  "
            << bar.str() << " " << std::setprecision(1) << pct << "%, "
            << "ETA " << std::setprecision(0) << eta << "s  |  "
            << "TX " << HumanCount(g_txCount) << " pkts "
            << "(" << std::setprecision(1) << txRate << "/s),  "
            << "RX " << HumanCount(g_rxCount) << " pkts "
            << "(" << std::setprecision(1) << rxRate << "/s),  "
            << "Data ~" << HumanBytes(bytesPerSec, true) << "  |  "
            << "RSS " << (rssKb / 1024) << " MB"
            << std::endl;

  if (now + kStatusEvery <= simTime)
  {
    Simulator::Schedule(Seconds(kStatusEvery), &StatusTick, simTime);
  }
}

// -------- Application --------
class PeriodicBroadcastApp : public Application
{
public:
  void Setup(uint16_t port, uint32_t payloadBytes, double rateHz)
  {
    m_port = port;
    m_payloadBytes = payloadBytes;
    m_interval = Seconds(1.0 / rateHz);
  }
private:
  void StartApplication() override
  {
    m_sock = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
    m_sock->SetAllowBroadcast(true);
    m_sock->Bind();
    m_sock->Connect(InetSocketAddress(Ipv4Address("255.255.255.255"), m_port));

    // Use constant jitter for deterministic behavior across MPI ranks
    Time jitter = Seconds(0.01 * GetNode()->GetId());
    m_event = Simulator::Schedule(jitter, &PeriodicBroadcastApp::Tick, this);
  }
  void StopApplication() override
  {
    if (m_event.IsPending()) Simulator::Cancel(m_event);
    if (m_sock) m_sock->Close();
  }
  void Tick()
  {
    m_sock->Send(Create<Packet>(m_payloadBytes));
    ++g_txCount;
    m_event = Simulator::Schedule(m_interval, &PeriodicBroadcastApp::Tick, this);
  }
private:
  Ptr<Socket> m_sock;
  uint16_t m_port{4444};
  uint32_t m_payloadBytes{300};
  Time m_interval{Seconds(0.1)};
  EventId m_event;
};

// Packet reception callback
void PacketReceived(Ptr<const Packet> packet, const Address& address) {
    ++g_rxCount;
}

// -------- Topology --------
static void
PositionVehicles(const NodeContainer& nodes, uint32_t lanes, uint32_t vehPerLane,
                 double laneSpacing, double vehSpacing, double speed, uint32_t startIdx)
{
  MobilityHelper mob;
  mob.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
  mob.Install(nodes);

  for (uint32_t i = 0; i < nodes.GetN(); ++i)
  {
    uint32_t globalIdx = startIdx + i;
    uint32_t lane = globalIdx / vehPerLane;
    uint32_t posInLane = globalIdx % vehPerLane;
    
    auto m = nodes.Get(i)->GetObject<ConstantVelocityMobilityModel>();
    double x = posInLane * vehSpacing;
    double y = lane * laneSpacing;
    m->SetPosition(Vector(x, y, 1.5));
    
    // Add slight speed variation for realism
    double speedVar = speed + (globalIdx % 10 - 5) * 0.5; // ±2.5 m/s variation
    m->SetVelocity(Vector(speedVar, 0.0, 0.0));
  }
}

int main (int argc, char *argv[])
{
  // Simulation parameters
  uint32_t lanes = 6;
  uint32_t vehPerLane = 300;
  double laneSpacing = 4.0;
  double vehSpacing  = 7.0;
  double speed = 30.0;
  double simTime = 900.0;
  uint32_t payloadBytes = 1200;
  double beaconHz = 10.0;
  bool enablePcap = false;
  bool enableFlowmon = false; // Disabled for MPI
  bool enableRtsCts = true;
  std::string dataMode = "OfdmRate6Mbps";
  double txPowerDbm = 23.0;
  uint32_t rngRun = 1;

  CommandLine cmd(__FILE__);
  cmd.AddValue("lanes", "Number of lanes", lanes);
  cmd.AddValue("vehPerLane", "Vehicles per lane", vehPerLane);
  cmd.AddValue("laneSpacing", "Lane separation (m)", laneSpacing);
  cmd.AddValue("vehSpacing", "Initial spacing per vehicle (m)", vehSpacing);
  cmd.AddValue("speed", "Vehicle speed (m/s)", speed);
  cmd.AddValue("simTime", "Simulation time (s)", simTime);
  cmd.AddValue("payloadBytes", "Beacon payload size (bytes)", payloadBytes);
  cmd.AddValue("beaconHz", "Beacon frequency (Hz)", beaconHz);
  cmd.AddValue("dataMode", "Wi-Fi DataMode", dataMode);
  cmd.AddValue("txPowerDbm", "Tx power (dBm)", txPowerDbm);
  cmd.AddValue("pcap", "Enable PCAP capture", enablePcap);
  cmd.AddValue("rtsCts", "Force RTS/CTS (threshold=0)", enableRtsCts);
  cmd.AddValue("rngRun", "RNG run", rngRun);
  cmd.Parse(argc, argv);

  RngSeedManager::SetRun(rngRun);

  uint32_t totalVehicles = lanes * vehPerLane;
  g_payloadBytes = payloadBytes;
  g_beaconHz = beaconHz;
  g_txCount = 0;
  g_rxCount = 0;

#ifdef NS3_MPI
  // Initialize MPI
  MpiInterface::Enable(&argc, &argv);

  uint32_t systemId = MpiInterface::GetSystemId();
  uint32_t systemCount = MpiInterface::GetSize();

  // Get hostname and process ID
  char hostname[256];
  gethostname(hostname, sizeof(hostname));
  pid_t pid = getpid();

  if (systemCount < 2) {
    if (systemId == 0) {
      std::cerr << "ERROR: This scenario requires at least 2 MPI ranks" << std::endl;
      std::cerr << "Recommended: 4-8 ranks for " << totalVehicles << " vehicles" << std::endl;
    }
    MpiInterface::Disable();
    return 1;
  }

  if (systemId == 0) {
    // RANK 0: Channel Processor
    std::cout << "=== Distributed Vehicular WiFi Heavy ===" << std::endl;
    std::cout << "Rank 0 on " << hostname << " (PID: " << pid << ")" << std::endl;
    std::cout << "Total vehicles: " << totalVehicles 
              << " (lanes=" << lanes << ", veh/lane=" << vehPerLane << ")" << std::endl;
    std::cout << "Beacon: " << beaconHz << " Hz, payload=" << payloadBytes << " B" << std::endl;
    std::cout << "Mobility: speed=" << speed << " m/s, spacing=" << vehSpacing << " m" << std::endl;
    std::cout << "WiFi: 802.11a, mode=" << dataMode << ", power=" << txPowerDbm << " dBm" << std::endl;
    std::cout << "MPI ranks: " << systemCount << ", RNG run: " << rngRun << std::endl;
    std::cout << "Sim time: " << simTime << " s" << std::endl;
    std::cout << "================================" << std::endl;

    g_expectedTx = static_cast<uint64_t>(std::llround(totalVehicles * beaconHz * simTime));
    std::cout << "Expected TX: ~" << HumanCount(g_expectedTx) << " packets" << std::endl;

    // Create MPI channel processor
    Ptr<WifiChannelMpiProcessor> processor = CreateObject<WifiChannelMpiProcessor>();

    // Create channel with vehicular propagation
    Ptr<YansWifiChannel> channel = CreateObject<YansWifiChannel>();
    Ptr<TwoRayGroundPropagationLossModel> lossModel = 
        CreateObject<TwoRayGroundPropagationLossModel>();
    lossModel->SetAttribute("HeightAboveZ", DoubleValue(1.5));
    
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    
    channel->SetPropagationLossModel(lossModel);
    channel->SetPropagationDelayModel(delayModel);

    std::cout << "Channel processor ready" << std::endl;

    Simulator::Stop(Seconds(simTime));
    Simulator::Schedule(Seconds(kStatusEvery), &StatusTick, simTime);
    Simulator::Run();

    std::cout << "\n=== Channel Processor Completed ===" << std::endl;
    std::cout << "Processed vehicular WiFi traffic for " << simTime << " seconds" << std::endl;
  }
  else {
    // OTHER RANKS: Vehicle Groups
    std::cout << "Rank " << systemId << " on " << hostname << " (PID: " << pid << ")" << std::endl;

    // Calculate which vehicles this rank handles
    uint32_t vehiclesPerRank = totalVehicles / (systemCount - 1);
    uint32_t startVehicle = (systemId - 1) * vehiclesPerRank;
    uint32_t endVehicle = (systemId == systemCount - 1) ? 
                          totalVehicles : startVehicle + vehiclesPerRank;
    uint32_t localVehicles = endVehicle - startVehicle;

    std::cout << "Managing vehicles " << startVehicle << " to " << (endVehicle - 1) 
              << " (" << localVehicles << " vehicles)" << std::endl;

    g_nodes = localVehicles;

    if (enableRtsCts) {
      Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(0));
    }

    // Create nodes for this rank's vehicles
    NodeContainer nodes;
    nodes.Create(localVehicles);

    // Position vehicles with mobility
    PositionVehicles(nodes, lanes, vehPerLane, laneSpacing, vehSpacing, speed, startVehicle);

    // Create MPI WiFi channel stub for distributed simulation
    Ptr<RemoteYansWifiChannelStub> mpiChannelStub = CreateObject<RemoteYansWifiChannelStub>();
    mpiChannelStub->SetRemoteChannelRank(0);
    mpiChannelStub->SetLocalDeviceRank(systemId);

    // Setup WiFi PHY for MPI
    YansWifiPhyHelper mpiPhy;
    mpiPhy.SetChannel(mpiChannelStub);
    mpiPhy.Set("TxPowerStart", DoubleValue(txPowerDbm));
    mpiPhy.Set("TxPowerEnd", DoubleValue(txPowerDbm));
    mpiPhy.Set("RxGain", DoubleValue(0.0));
    mpiPhy.Set("TxGain", DoubleValue(0.0));

    // Optional: Create local monitoring network for PCAP
    YansWifiPhyHelper localPhy;
    if (enablePcap) {
      YansWifiChannelHelper localChannelHelper = YansWifiChannelHelper::Default();
      localChannelHelper.AddPropagationLoss("ns3::TwoRayGroundPropagationLossModel",
                                           "HeightAboveZ", DoubleValue(1.5));
      Ptr<YansWifiChannel> localChannel = localChannelHelper.Create();
      localPhy.SetChannel(localChannel);
      localPhy.Set("TxPowerStart", DoubleValue(15.0)); // Lower power for monitoring
      localPhy.Set("TxPowerEnd", DoubleValue(15.0));
    }

    // Configure WiFi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue(dataMode),
                                "ControlMode", StringValue(dataMode));

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    // Create MPI devices
    NetDeviceContainer mpiDevices = wifi.Install(mpiPhy, mac, nodes);
    
    // Create local monitoring devices if PCAP enabled
    NetDeviceContainer localDevices;
    if (enablePcap) {
      localDevices = wifi.Install(localPhy, mac, nodes);
    }

    // Install Internet stack
    InternetStackHelper internet;
    internet.Install(nodes);

    // Assign IP addresses to MPI network
    Ipv4AddressHelper mpiIp;
    std::ostringstream mpiSubnet;
    mpiSubnet << "10." << systemId << ".0.0";
    mpiIp.SetBase(mpiSubnet.str().c_str(), "255.255.0.0");
    Ipv4InterfaceContainer mpiInterfaces = mpiIp.Assign(mpiDevices);

    // Assign IP addresses to local monitoring network
    if (enablePcap) {
      Ipv4AddressHelper localIp;
      std::ostringstream localSubnet;
      localSubnet << "192.168." << systemId << ".0";
      localIp.SetBase(localSubnet.str().c_str(), "255.255.255.0");
      localIp.Assign(localDevices);
    }

    std::cout << "Created " << localVehicles << " vehicles with WiFi devices" << std::endl;

    // Setup applications
    uint16_t port = 4444;
    
    // Packet sinks on all vehicles
    PacketSinkHelper sinkHelper("ns3::UdpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinks = sinkHelper.Install(nodes);
    sinks.Start(Seconds(0.0));
    sinks.Stop(Seconds(simTime));

    // Connect RX callback
    for (uint32_t i = 0; i < sinks.GetN(); ++i) {
      Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinks.Get(i));
      if (sink) {
        sink->TraceConnectWithoutContext("Rx", MakeCallback(&PacketReceived));
      }
    }

    // Broadcast applications on all vehicles
    for (uint32_t i = 0; i < nodes.GetN(); ++i) {
      Ptr<PeriodicBroadcastApp> app = CreateObject<PeriodicBroadcastApp>();
      app->Setup(port, payloadBytes, beaconHz);
      nodes.Get(i)->AddApplication(app);
      app->SetStartTime(Seconds(0.0));
      app->SetStopTime(Seconds(simTime));
    }

    // Enable PCAP on local monitoring network
    if (enablePcap) {
      std::ostringstream prefix;
      prefix << "vehicular-mpi-rank" << systemId;
      localPhy.EnablePcapAll(prefix.str(), true);
      std::cout << "PCAP enabled: " << prefix.str() << "-*.pcap" << std::endl;
    }

    // Schedule status updates (only from rank 1)
    if (systemId == 1) {
      Simulator::Schedule(Seconds(kStatusEvery), &StatusTick, simTime);
    }

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    // Final summary
    double now = Simulator::Now().GetSeconds();
    double txRate = now > 0 ? static_cast<double>(g_txCount) / now : 0.0;
    double rxRate = now > 0 ? static_cast<double>(g_rxCount) / now : 0.0;
    double bytesPerSec = txRate * static_cast<double>(g_payloadBytes);
    size_t rssKb = GetVmRSSKb();

    std::cout << "\n=== Rank " << systemId << " Completed ===" << std::endl;
    std::cout << "Vehicles: " << localVehicles << std::endl;
    std::cout << "TX total: " << g_txCount << " pkts (" << txRate << " pkt/s)" << std::endl;
    std::cout << "RX total: " << g_rxCount << " pkts (" << rxRate << " pkt/s)" << std::endl;
    std::cout << "Data rate: " << HumanBytes(bytesPerSec, true) << std::endl;
    std::cout << "Final RSS: " << (rssKb / 1024) << " MB" << std::endl;

    if (enablePcap) {
      std::cout << "\nChecking PCAP files..." << std::endl;
      for (uint32_t i = 0; i < std::min(localVehicles, 3u); ++i) {
        std::ostringstream filename;
        filename << "vehicular-mpi-rank" << systemId << "-" << i << "-1.pcap";
        std::ifstream file(filename.str());
        if (file.good()) {
          file.seekg(0, std::ios::end);
          size_t size = file.tellg();
          std::cout << "✅ " << filename.str() << " (" << HumanBytes(size, false) << ")" << std::endl;
        }
      }
    }
  }

  Simulator::Destroy();
  MpiInterface::Disable();

#else
  std::cerr << "ERROR: This scenario requires MPI support" << std::endl;
  std::cerr << "Compile NS-3 with: ./ns3 configure --enable-mpi" << std::endl;
  return 1;
#endif

  return 0;
}
