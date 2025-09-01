#include "ns3/core-module.h"
#include "ns3/remote-yans-wifi-channel-stub.h"
#include "ns3/remote-yans-wifi-phy-stub.h"
#include "ns3/wifi-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SimpleMpiStubTest");

int
main(int argc, char* argv[])
{
    bool verbose = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "Enable verbose logging", verbose);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("SimpleMpiStubTest", LOG_LEVEL_ALL);
        LogComponentEnable("RemoteYansWifiChannelStub", LOG_LEVEL_ALL);
        LogComponentEnable("RemoteYansWifiPhyStub", LOG_LEVEL_ALL);
    }

    std::cout << "\n=================================" << std::endl;
    std::cout << "=== Simple MPI WiFi Stub Test ===" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "Testing logging-only MPI stubs without actual WiFi setup" << std::endl;
    std::cout << "This demonstrates the MPI message simulation concept" << std::endl;
    std::cout << "=================================" << std::endl;

    // Test 1: Create and configure device-side channel stub
    std::cout << "\n=== Test 1: Device-Side Channel Stub ===" << std::endl;

    Ptr<RemoteYansWifiChannelStub> deviceStub = CreateObject<RemoteYansWifiChannelStub>();
    deviceStub->SetLocalDeviceRank(1);   // Simulate we're on device rank 1
    deviceStub->SetRemoteChannelRank(0); // Channel runs on rank 0

    // Test basic configuration operations
    std::cout << "Configuring propagation models..." << std::endl;

    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    deviceStub->SetPropagationDelayModel(delayModel);

    Ptr<FixedRssLossModel> lossModel = CreateObject<FixedRssLossModel>();
    lossModel->SetRss(-80); // -80 dBm
    deviceStub->SetPropagationLossModel(lossModel);

    // Test 2: Create channel-side device stubs
    std::cout << "\n=== Test 2: Channel-Side Device Stubs ===" << std::endl;

    std::vector<Ptr<RemoteYansWifiPhyStub>> channelStubs;

    for (uint32_t i = 0; i < 3; ++i)
    {
        Ptr<RemoteYansWifiPhyStub> phyStub = CreateObject<RemoteYansWifiPhyStub>();
        phyStub->SetRemoteDeviceRank(1);     // These devices are on rank 1
        phyStub->SetRemoteDeviceId(100 + i); // Unique device IDs
        channelStubs.push_back(phyStub);

        std::cout << "Created PHY stub for device " << (100 + i) << " (simulated on rank "
                  << phyStub->GetRemoteDeviceRank() << ")" << std::endl;
    }

    // Test 3: Demonstrate stub operations without real PHY
    std::cout << "\n=== Test 3: Simulating MPI Operations (Without Real PHY) ===" << std::endl;

    // Simulate device registration (device -> channel communication)
    std::cout << "Simulating device registration..." << std::endl;
    std::cout << "Device would call: deviceStub->Add(phy) -> sends MPI message to channel"
              << std::endl;

    // Simulate transmission (device -> channel communication)
    std::cout << "Simulating packet transmission..." << std::endl;
    std::cout
        << "Device would call: deviceStub->Send(phy, packet, power) -> sends MPI message to channel"
        << std::endl;

    // Simulate reception notifications (channel -> device communication)
    std::cout << "Simulating reception notifications..." << std::endl;
    for (const auto& stub : channelStubs)
    {
        std::cout << "Channel would notify device " << stub->GetRemoteDeviceId()
                  << " of packet reception via MPI" << std::endl;

        // Simulate calling the notification methods
        stub->SimulateRxEvent(1e-6, 1024); // 1 µW, 1024 bytes
    }

    // Test callback setup on channel-side stubs
    std::cout << "Simulating callback configuration..." << std::endl;
    for (auto& stub : channelStubs)
    {
        std::cout << "  - Callbacks would be configured for device " << stub->GetRemoteDeviceId()
                  << std::endl;
    }

    // Test 4: Summary
    std::cout << "\n=== Test 4: Summary ===" << std::endl;
    std::cout << "Device stub configured for rank " << deviceStub->GetRemoteChannelRank()
              << std::endl;
    std::cout << "Created " << channelStubs.size() << " PHY stubs on the channel side" << std::endl;

    // Show what each stub represents
    for (const auto& stub : channelStubs)
    {
        std::cout << "  - PHY stub for device " << stub->GetRemoteDeviceId() << " on rank "
                  << stub->GetRemoteDeviceRank() << std::endl;
    }

    std::cout << "\n=== All Tests Complete ===" << std::endl;
    std::cout << "Check the logs above to see simulated MPI message flows" << std::endl;
    std::cout << "Each SIMULATED_MPI_MSG represents what would be an actual MPI call" << std::endl;
    std::cout << "\nThis demonstrates the foundation for distributed WiFi simulation:" << std::endl;
    std::cout << "- Device operations -> Channel operations (via MPI messages)" << std::endl;
    std::cout << "- Channel operations -> Device notifications (via MPI messages)" << std::endl;
    std::cout << "- Clean separation between device logic and channel logic" << std::endl;

    return 0;
}
