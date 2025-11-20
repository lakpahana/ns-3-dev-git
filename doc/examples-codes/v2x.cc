#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdint>
#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("VehicularWifiHeavy");

// -------- Helpers for status formatting --------
static uint64_t g_txCount = 0;
static uint64_t g_expectedTx = 0;
static uint32_t g_payloadBytes = 0;
static uint32_t g_nodes = 0;
static double   g_beaconHz = 0.0;
static const double kStatusEvery = 2.0; // seconds

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
      iss >> key >> valKb >> unit; // "VmRSS:" <kb> "kB"
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

    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    Time jitter = Seconds(rng->GetValue(0.0, m_interval.GetSeconds()));
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

// -------- Topology / main --------
static void
PositionVehicles(const NodeContainer& nodes, uint32_t lanes, uint32_t vehPerLane,
                 double laneSpacing, double vehSpacing, double speed)
{
  MobilityHelper mob;
  mob.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
  mob.Install(nodes);

  for (uint32_t i = 0; i < lanes; ++i)
  {
    for (uint32_t j = 0; j < vehPerLane; ++j)
    {
      uint32_t idx = i * vehPerLane + j;
      auto m = nodes.Get(idx)->GetObject<ConstantVelocityMobilityModel>();
      double x = j * vehSpacing;
      double y = i * laneSpacing;
      m->SetPosition(Vector(x, y, 1.5));
      m->SetVelocity(Vector(speed, 0.0, 0.0));
    }
  }
}

int main (int argc, char *argv[])
{
  uint32_t lanes = 6;
  uint32_t vehPerLane = 300;
  double laneSpacing = 4.0;
  double vehSpacing  = 7.0;
  double speed = 30.0;
  double simTime = 900.0;
  uint32_t payloadBytes = 1200;
  double beaconHz = 10.0;
  bool enablePcap = false;
  bool enableFlowmon = true;
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
  cmd.AddValue("flowmon", "Enable FlowMonitor", enableFlowmon);
  cmd.AddValue("rtsCts", "Force RTS/CTS (threshold=0)", enableRtsCts);
  cmd.AddValue("rngRun", "RNG run", rngRun);
  cmd.Parse(argc, argv);

  RngSeedManager::SetRun(rngRun);

  uint32_t n = lanes * vehPerLane;
  g_nodes = n;
  g_payloadBytes = payloadBytes;
  g_beaconHz = beaconHz;
  g_txCount = 0;
  g_expectedTx = static_cast<uint64_t>(std::llround(n * beaconHz * simTime));

  std::cout << "=== Vehicular Wi-Fi Heavy ===" << std::endl
            << "Nodes: " << n << "  (lanes=" << lanes << ", veh/lane=" << vehPerLane << ")" << std::endl
            << "Beacon: " << beaconHz << " Hz, payload=" << payloadBytes << " B" << std::endl
            << "Mobility: speed=" << speed << " m/s, laneSpacing=" << laneSpacing << " m, vehSpacing=" << vehSpacing << " m" << std::endl
            << "Wi-Fi: standard=802.11a, mode=" << dataMode << ", txPower=" << txPowerDbm << " dBm, RTS/CTS=" << (enableRtsCts?"on":"off") << std::endl
            << "Outputs: flowmon=" << (enableFlowmon?"on":"off") << ", pcap=" << (enablePcap?"on":"off") << std::endl
            << "RNG run: " << rngRun << "   Sim time: " << simTime << " s" << std::endl
            << "Expected TX: ~" << HumanCount(g_expectedTx) << " packets (nodes*rate*time)" << std::endl
            << "================================" << std::endl;

  if (enableRtsCts)
  {
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(0));
  }

  // Channel/PHY
  YansWifiChannelHelper ch = YansWifiChannelHelper::Default();
  ch.AddPropagationLoss("ns3::TwoRayGroundPropagationLossModel");
  Ptr<YansWifiChannel> channel = ch.Create();

  YansWifiPhyHelper phy;
  phy.SetChannel(channel);
  phy.Set("TxPowerStart", DoubleValue(txPowerDbm));
  phy.Set("TxPowerEnd",   DoubleValue(txPowerDbm));
  phy.Set("RxGain", DoubleValue(0.0));
  phy.Set("TxGain", DoubleValue(0.0));

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211a);
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                               "DataMode", StringValue(dataMode),
                               "ControlMode", StringValue(dataMode));

  WifiMacHelper mac;
  mac.SetType("ns3::AdhocWifiMac");

  NodeContainer nodes; nodes.Create(n);
  NetDeviceContainer devs = wifi.Install(phy, mac, nodes);

  InternetStackHelper internet;
  internet.Install(nodes);
  Ipv4AddressHelper ip;
  ip.SetBase("10.0.0.0", "255.255.0.0");
  Ipv4InterfaceContainer ifs = ip.Assign(devs);

  PositionVehicles(nodes, lanes, vehPerLane, laneSpacing, vehSpacing, speed);

  uint16_t port = 4444;
  PacketSinkHelper sinkHelper("ns3::UdpSocketFactory",
                              InetSocketAddress(Ipv4Address::GetAny(), port));
  ApplicationContainer sinks = sinkHelper.Install(nodes);
  sinks.Start(Seconds(0.0));
  sinks.Stop(Seconds(simTime));

  for (uint32_t i = 0; i < nodes.GetN(); ++i)
  {
    Ptr<PeriodicBroadcastApp> app = CreateObject<PeriodicBroadcastApp>();
    app->Setup(port, payloadBytes, beaconHz);
    nodes.Get(i)->AddApplication(app);
    app->SetStartTime(Seconds(0.0));
    app->SetStopTime(Seconds(simTime));
  }

  if (enablePcap) { phy.EnablePcapAll("vehicular-wifi-heavy", true); }

  FlowMonitorHelper fm;
  Ptr<FlowMonitor> mon;
  if (enableFlowmon) { mon = fm.InstallAll(); }

  // Schedule periodic console status
  Simulator::Schedule(Seconds(kStatusEvery), &StatusTick, simTime);

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  // Final summary
  double now = Simulator::Now().GetSeconds();
  double txRate = now > 0 ? static_cast<double>(g_txCount) / now : 0.0;
  double bytesPerSec = txRate * static_cast<double>(g_payloadBytes);
  size_t rssKb = GetVmRSSKb();

  std::cout << "=== Completed ===" << std::endl
            << "Sim time: " << now << " s" << std::endl
            << "TX total: " << g_txCount << " pkts (expected ~" << g_expectedTx << ")" << std::endl
            << "Avg TX rate: " << std::setprecision(1) << txRate << " pkt/s, "
            << "Avg data: " << HumanBytes(bytesPerSec, true) << std::endl
            << "Final RSS: " << (rssKb / 1024) << " MB" << std::endl;

  if (enableFlowmon)
  {
    mon->CheckForLostPackets();
    fm.SerializeToXmlFile("vehicular-wifi-heavy.flowmon.xml", true, true);
    std::cout << "FlowMonitor: wrote vehicular-wifi-heavy.flowmon.xml" << std::endl;
  }

  Simulator::Destroy();
  return 0;
}
