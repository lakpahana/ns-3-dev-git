#include "remote-yans-wifi-phy-stub.h"
#include "wifi-mpi-messages.h"

#include "ns3/core-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RemoteStubTest");

int
main(int argc, char* argv[])
{
    std::cout << "\n=== Testing RemoteYansWifiPhyStub ===" << std::endl;

    // Test 1: Create and configure a stub
    std::cout << "\n--- Test 1: Creating and configuring stub ---" << std::endl;
    Ptr<RemoteYansWifiPhyStub> stub = CreateObject<RemoteYansWifiPhyStub>();
    stub->SetRemoteRank(1);
    stub->SetRemoteDeviceId(42);

    std::cout << "Created stub for device " << stub->GetRemoteDeviceId() << " on rank "
              << stub->GetRemoteRank() << std::endl;

    // Test 2: Test position methods
    std::cout << "\n--- Test 2: Testing position methods ---" << std::endl;
    Vector position(10.0, 20.0, 5.0);
    stub->SetPosition(position);
    Vector retrievedPosition = stub->GetPosition();
    std::cout << "Position set and retrieved: (" << retrievedPosition.x << ", "
              << retrievedPosition.y << ", " << retrievedPosition.z << ")" << std::endl;

    // Test 3: Test simulation methods
    std::cout << "\n--- Test 3: Testing simulation methods ---" << std::endl;
    // Create a simple PPDU for testing (null for now, real implementation will need actual PPDU)
    Ptr<const WifiPpdu> ppdu = nullptr;            // Simplified for testing
    stub->SimulateRx(ppdu, -70.0, Seconds(0.001)); // -70 dBm, 1ms duration

    // Test 4: Test MPI message structures
    std::cout << "\n--- Test 4: Testing MPI message structures ---" << std::endl;

    // Test TX request message
    WifiMpiTxRequest txRequest;
    txRequest.senderId = 100;
    txRequest.senderRank = 2;
    txRequest.txPowerW = 0.02; // 20mW
    txRequest.txTime = Seconds(1.5);
    txRequest.packetSize = 1024;

    std::cout << "TX Request - Sender ID: " << txRequest.senderId
              << ", Rank: " << txRequest.senderRank << ", Power: " << txRequest.txPowerW << "W"
              << ", Time: " << txRequest.txTime.GetSeconds() << "s"
              << ", Size: " << txRequest.packetSize << " bytes" << std::endl;

    // Test RX notification message
    WifiMpiRxNotification rxNotification;
    rxNotification.receiverId = 200;
    rxNotification.receiverRank = 3;
    rxNotification.rxPowerW = 0.001; // 1mW
    rxNotification.snr = 15.5;
    rxNotification.rxTime = Seconds(2.0);
    rxNotification.packetSize = 1024;

    std::cout << "RX Notification - Receiver ID: " << rxNotification.receiverId
              << ", Rank: " << rxNotification.receiverRank << ", Power: " << rxNotification.rxPowerW
              << "W"
              << ", SNR: " << rxNotification.snr << " dB"
              << ", Time: " << rxNotification.rxTime.GetSeconds() << "s"
              << ", Size: " << rxNotification.packetSize << " bytes" << std::endl;

    // Test Heartbeat message
    WifiMpiHeartbeat heartbeat;
    heartbeat.currentTime = Seconds(5.0);
    heartbeat.sourceRank = 0;

    std::cout << "Heartbeat - Time: " << heartbeat.currentTime.GetSeconds() << "s"
              << ", Source Rank: " << heartbeat.sourceRank << std::endl;

    // Test 5: Test message serialization sizes
    std::cout << "\n--- Test 5: Testing message serialization ---" << std::endl;
    std::cout << "TX Request serialized size: " << txRequest.GetSerializedSize() << " bytes"
              << std::endl;
    std::cout << "RX Notification serialized size: " << rxNotification.GetSerializedSize()
              << " bytes" << std::endl;
    std::cout << "Heartbeat serialized size: " << heartbeat.GetSerializedSize() << " bytes"
              << std::endl;

    std::cout << "\n=== Stub Test Complete ===" << std::endl;

    return 0;
}
