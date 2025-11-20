/*
 * Multi-Rank WiFi MPI Communication Test
 *
 * Run with: mpirun -np 2 ./build/scratch/ns3-dev-wifi-mpi-demo-default
 *
 * Rank 0: Channel processor
 * Rank 1: Device with enhanced MPI communication
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

#ifdef NS3_MPI
#include "ns3/mpi-interface.h"
#include "ns3/remote-yans-wifi-channel-stub.h"
#include "ns3/wifi-channel-mpi-processor.h"
#endif

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiMpiDemo");

int
main(int argc, char* argv[])
{
    CommandLine cmd;
    cmd.Parse(argc, argv);

    std::cout << "=== WiFi MPI Multi-Rank Demo ===" << std::endl;

#ifdef NS3_MPI
    // Enable MPI
    MpiInterface::Enable(&argc, &argv);

    uint32_t rank = MpiInterface::GetSystemId();
    uint32_t size = MpiInterface::GetSize();

    std::cout << "[Rank " << rank << "] Starting - Total ranks: " << size << std::endl;

    if (rank == 0)
    {
        std::cout << "[Rank 0] === CHANNEL RANK === Setting up MPI Processor" << std::endl;

        // Create and initialize channel processor
        Ptr<WifiChannelMpiProcessor> processor = Create<WifiChannelMpiProcessor>();
        bool initSuccess = processor->Initialize();

        std::cout << "[Rank 0] Channel processor initialization: "
                  << (initSuccess ? "SUCCESS" : "FAILED") << std::endl;

        if (initSuccess)
        {
            std::cout << "[Rank 0] Ready to receive device registrations and transmissions"
                      << std::endl;

            // Run for a while to receive messages from device ranks
            Simulator::Stop(Seconds(5.0));
            Simulator::Run();

            std::cout << "[Rank 0] Final device count: " << processor->GetDeviceCount()
                      << std::endl;
        }
    }
    else
    {
        std::cout << "[Rank " << rank << "] === DEVICE RANK === Setting up Enhanced Channel Stub"
                  << std::endl;

        // Create nodes
        NodeContainer nodes;
        nodes.Create(1);

        // Create enhanced channel stub that will send real MPI messages
        Ptr<RemoteYansWifiChannelStub> channelStub = Create<RemoteYansWifiChannelStub>();
        channelStub->SetRemoteChannelRank(0); // Channel is on rank 0
        channelStub->SetLocalDeviceRank(rank);

        // Initialize MPI for this stub
        bool mpiInit = channelStub->InitializeMpi();
        std::cout << "[Rank " << rank
                  << "] Channel stub MPI initialization: " << (mpiInit ? "SUCCESS" : "FAILED")
                  << std::endl;

        // Create WiFi devices
        YansWifiPhyHelper wifiPhy;
        wifiPhy.SetChannel(channelStub);

        WifiHelper wifi;
        wifi.SetStandard(WIFI_STANDARD_80211n);

        WifiMacHelper mac;
        mac.SetType("ns3::StaWifiMac");

        NetDeviceContainer devices = wifi.Install(wifiPhy, mac, nodes);

        std::cout << "[Rank " << rank << "] Created " << devices.GetN()
                  << " WiFi devices with enhanced MPI channel" << std::endl;

        // Simulate some network activity to trigger our enhanced Send method
        Ptr<WifiNetDevice> wifiDevice = DynamicCast<WifiNetDevice>(devices.Get(0));
        if (wifiDevice)
        {
            // Create test packets
            for (int i = 0; i < 3; i++)
            {
                Ptr<Packet> packet = Create<Packet>(1000 + i * 100);

                // Schedule transmissions at different times
                Time sendTime = Seconds(0.5 + i * 1.0);
                Simulator::Schedule(sendTime, [wifiDevice, packet, rank, i]() {
                    std::cout << "[Rank " << rank << "] Sending test packet " << (i + 1)
                              << " (size: " << packet->GetSize() << " bytes)" << std::endl;

                    // This will trigger our enhanced Send method with real MPI!
                    wifiDevice->Send(packet, Mac48Address("00:00:00:00:00:02"), 0x0800);
                });
            }

            std::cout << "[Rank " << rank << "] Scheduled 3 test transmissions" << std::endl;
        }

        // Run simulation
        Simulator::Stop(Seconds(5.0));
        Simulator::Run();

        std::cout << "[Rank " << rank << "] Device simulation completed" << std::endl;
    }

    std::cout << "[Rank " << rank << "] Cleaning up..." << std::endl;
    Simulator::Destroy();
    MpiInterface::Disable();

#else
    std::cout << "ERROR: This demo requires MPI support" << std::endl;
    std::cout << "Please build ns-3 with --enable-mpi" << std::endl;
    return 1;
#endif

    std::cout << "[Rank " << rank << "] === Demo Complete ===" << std::endl;
    return 0;
}
