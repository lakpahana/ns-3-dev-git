# Technical Implementation Guide: MPI WiFi Proxy and Stub Classes

## Class Hierarchy and Relationships

```
YansWifiChannel (ns-3 base class)
├── YansWifiChannelProxy (logging proxy - inherits from base)
└── RemoteYansWifiChannelStub (device-side stub - inherits from base)

YansWifiPhy (ns-3 base class)  
└── RemoteYansWifiPhyStub (channel-side stub - inherits from base)
```

## Detailed Class Implementation

### 1. YansWifiChannelProxy Class

**Header**: `src/wifi/mpi-channel/yans-wifi-channel-proxy.h`

```cpp
class YansWifiChannelProxy : public YansWifiChannel
{
public:
    static TypeId GetTypeId();
    YansWifiChannelProxy();
    virtual ~YansWifiChannelProxy();

    // Override virtual methods from YansWifiChannel
    void Add(Ptr<YansWifiPhy> phy) override;
    void Send(Ptr<YansWifiPhy> sender, Ptr<const WifiPpdu> ppdu, dBm_u txPower) const override;
    void SetPropagationLossModel(const Ptr<PropagationLossModel> loss) override;
    void SetPropagationDelayModel(const Ptr<PropagationDelayModel> delay) override;

private:
    uint32_t m_addCount;        //!< Number of PHYs added
    mutable uint32_t m_sendCount; //!< Number of packets sent
    
    void LogOperation(const std::string& operation, const std::string& details = "") const;
};
```

**Key Implementation Details**:
- **Inheritance Strategy**: Inherits from `YansWifiChannel` and overrides virtual methods
- **Functionality Preservation**: Always calls parent class methods to maintain original behavior
- **Logging Integration**: Adds comprehensive logging without affecting performance
- **Thread Safety**: Uses mutable members for const method compatibility

**Method Override Pattern**:
```cpp
void YansWifiChannelProxy::Add(Ptr<YansWifiPhy> phy)
{
    m_addCount++;
    LogOperation("Add", "PHY added to channel proxy");
    
    // CRITICAL: Call parent to maintain functionality
    YansWifiChannel::Add(phy);
}
```

### 2. RemoteYansWifiChannelStub Class

**Header**: `src/wifi/mpi-channel/remote-yans-wifi-channel-stub.h`

```cpp
class RemoteYansWifiChannelStub : public YansWifiChannel
{
public:
    static TypeId GetTypeId();
    RemoteYansWifiChannelStub();
    virtual ~RemoteYansWifiChannelStub();

    // MPI rank configuration
    void SetRemoteChannelRank(uint32_t rank);
    void SetLocalDeviceRank(uint32_t rank);
    uint32_t GetRemoteChannelRank() const;
    uint32_t GetLocalDeviceRank() const;

    // Override channel operations to redirect to MPI
    void Add(Ptr<YansWifiPhy> phy) override;
    void Send(Ptr<YansWifiPhy> sender, Ptr<const WifiPpdu> ppdu, dBm_u txPower) const override;
    void SetPropagationLossModel(const Ptr<PropagationLossModel> loss) override;
    void SetPropagationDelayModel(const Ptr<PropagationDelayModel> delay) override;

private:
    uint32_t m_remoteChannelRank; //!< Rank where channel runs
    uint32_t m_localDeviceRank;   //!< Rank where this device runs
    uint32_t m_sendCount;         //!< Count of send operations
    uint32_t m_addCount;          //!< Count of device additions
    mutable uint32_t m_messageId; //!< Unique message ID

    void LogSimulatedMpiMessage(const std::string& messageType, const std::string& details) const;
    void LogMethodCall(const std::string& method, const std::string& details = "");
};
```

**MPI Message Simulation**:
```cpp
void RemoteYansWifiChannelStub::Send(Ptr<YansWifiPhy> sender, 
                                    Ptr<const WifiPpdu> ppdu, 
                                    dBm_u txPower) const
{
    m_sendCount++;
    
    // In real implementation, this would be:
    // MPI_Send(messageData, messageSize, MPI_BYTE, m_remoteChannelRank, TAG_TX_REQUEST, MPI_COMM_WORLD);
    
    std::ostringstream mpiDetails;
    mpiDetails << "TX_REQUEST - Device rank " << m_localDeviceRank 
               << " sending packet to channel rank " << m_remoteChannelRank
               << ", Power: " << txPower << ", Size: " << ppdu->GetSize() << " bytes";
    
    LogSimulatedMpiMessage("TX_REQUEST", mpiDetails.str());
    
    // Still call parent for local state consistency
    YansWifiChannel::Send(sender, ppdu, txPower);
}
```

### 3. RemoteYansWifiPhyStub Class

**Header**: `src/wifi/mpi-channel/remote-yans-wifi-phy-stub.h`

```cpp
class RemoteYansWifiPhyStub : public YansWifiPhy
{
public:
    static TypeId GetTypeId();
    RemoteYansWifiPhyStub();
    virtual ~RemoteYansWifiPhyStub();

    // Remote device configuration
    void SetRemoteDeviceRank(uint32_t rank);
    void SetRemoteDeviceId(uint32_t deviceId);
    uint32_t GetRemoteDeviceRank() const;
    uint32_t GetRemoteDeviceId() const;

    // Simulation methods for testing
    void SimulateRxEvent(double rxPowerW, uint32_t packetSize);

    // Override notification methods that would trigger MPI messages
    void NotifyRxStart(Time duration, dBm_u rxPower, WifiTxVector txVector) override;
    void NotifyRxEnd(Time duration) override;

private:
    uint32_t m_remoteDeviceRank;    //!< Rank where device runs
    uint32_t m_remoteDeviceId;      //!< Unique device identifier
    uint32_t m_rxEventCount;        //!< Count of RX events
    mutable uint32_t m_messageCount; //!< Count of MPI messages

    void LogSimulatedMpiMessage(const std::string& messageType, const std::string& details) const;
    void LogMethodCall(const std::string& method, const std::string& details = "");
};
```

**Reception Event Simulation**:
```cpp
void RemoteYansWifiPhyStub::SimulateRxEvent(double rxPowerW, uint32_t packetSize)
{
    m_rxEventCount++;
    
    std::ostringstream details;
    details << "RX Event #" << m_rxEventCount 
            << " for device " << m_remoteDeviceId
            << ", Power: " << rxPowerW << "W"
            << ", Packet size: " << packetSize << " bytes";
            
    LogMethodCall("SimulateRxEvent", details.str());
    
    // In real implementation, this would be:
    // MPI_Send(rxData, sizeof(RxEventData), MPI_BYTE, m_remoteDeviceRank, TAG_RX_NOTIFY, MPI_COMM_WORLD);
    
    std::ostringstream mpiDetails;
    mpiDetails << "RX_NOTIFICATION - Channel rank 0 sending RX event to device rank " 
               << m_remoteDeviceRank << ", " << details.str();
               
    LogSimulatedMpiMessage("RX_NOTIFICATION", mpiDetails.str());
}
```

## Message Protocol Design

### Message Types and Tags
```cpp
enum MpiMessageType {
    TAG_CONFIG_DELAY_MODEL = 100,
    TAG_CONFIG_LOSS_MODEL  = 101,
    TAG_DEVICE_REGISTER    = 102,
    TAG_TX_REQUEST         = 103,
    TAG_RX_NOTIFICATION    = 200,
    TAG_TX_START_NOTIFY    = 201,
    TAG_TX_END_NOTIFY      = 202,
    TAG_CONFIG_UPDATE      = 203
};
```

### Message Structure Design
```cpp
struct MpiMessageHeader {
    uint32_t messageType;    // Message type tag
    uint32_t messageSize;    // Total message size
    uint32_t sourceRank;     // Sender rank
    uint32_t targetRank;     // Receiver rank  
    uint64_t timestamp;      // Simulation timestamp
    uint32_t sequenceNumber; // Message sequence
};

struct DeviceRegisterMessage {
    MpiMessageHeader header;
    uint32_t deviceId;
    uint32_t phyType;
    // Additional PHY configuration data
};

struct TxRequestMessage {
    MpiMessageHeader header;
    uint32_t senderId;
    double txPowerW;
    uint32_t packetSize;
    // Serialized packet data follows
};
```

## Build System Integration Details

### WiFi Module CMakeLists.txt Changes
**File**: `src/wifi/CMakeLists.txt`

**Added Source Files**:
```cmake
${CMAKE_CURRENT_SOURCE_DIR}/model/yans-wifi-channel.cc
${CMAKE_CURRENT_SOURCE_DIR}/model/yans-wifi-phy.cc
# ... existing files ...

# NEW: MPI Channel Implementation
${CMAKE_CURRENT_SOURCE_DIR}/mpi-channel/yans-wifi-channel-proxy.cc
${CMAKE_CURRENT_SOURCE_DIR}/mpi-channel/remote-yans-wifi-channel-stub.cc
${CMAKE_CURRENT_SOURCE_DIR}/mpi-channel/remote-yans-wifi-phy-stub.cc
```

**Added Header Files**:
```cmake
${CMAKE_CURRENT_SOURCE_DIR}/model/yans-wifi-channel.h
${CMAKE_CURRENT_SOURCE_DIR}/model/yans-wifi-phy.h
# ... existing files ...

# NEW: MPI Channel Headers  
${CMAKE_CURRENT_SOURCE_DIR}/mpi-channel/yans-wifi-channel-proxy.h
${CMAKE_CURRENT_SOURCE_DIR}/mpi-channel/remote-yans-wifi-channel-stub.h
${CMAKE_CURRENT_SOURCE_DIR}/mpi-channel/remote-yans-wifi-phy-stub.h
```

### Example Build Configuration
**File**: `examples/wireless/CMakeLists.txt`

**Added Test Examples**:
```cmake
build_example(
  NAME mpi-wifi-stub-test
  SOURCE_FILES mpi-wifi-stub-test.cc
  LIBRARIES_TO_LINK ${libinternet}
                    ${libwifi}
                    ${libmobility}
                    ${libapplications}
)

build_example(
  NAME simple-mpi-stub-test2
  SOURCE_FILES simple-mpi-stub-test2.cc
  LIBRARIES_TO_LINK ${libinternet}
                    ${libwifi}
                    ${libmobility}
                    ${libapplications}
)
```

## Debugging and Logging Framework

### Log Component Registration
```cpp
// In each .cc file
NS_LOG_COMPONENT_DEFINE("YansWifiChannelProxy");
NS_LOG_COMPONENT_DEFINE("RemoteYansWifiChannelStub");
NS_LOG_COMPONENT_DEFINE("RemoteYansWifiPhyStub");
```

### Structured Logging Format
```cpp
void RemoteYansWifiChannelStub::LogSimulatedMpiMessage(const std::string& messageType, 
                                                      const std::string& details) const
{
    ++m_messageId;
    std::cout << "[SIMULATED_MPI_MSG #" << m_messageId << "] " 
              << messageType << " - " << details
              << " [SimTime: " << Simulator::Now().GetSeconds() << "s]" 
              << std::endl;
}

void RemoteYansWifiChannelStub::LogMethodCall(const std::string& method, 
                                             const std::string& details)
{
    std::cout << "[RemoteYansWifiChannelStub] STUB_CALL: " << method;
    if (!details.empty()) {
        std::cout << " - " << details;
    }
    std::cout << " [SimTime: " << Simulator::Now().GetSeconds() << "s]" << std::endl;
}
```

## Testing Strategy

### Unit Test Structure
```cpp
// Test 1: Verify proxy maintains functionality
Ptr<YansWifiChannelProxy> proxy = CreateObject<YansWifiChannelProxy>();
Ptr<YansWifiPhy> phy = CreateObject<YansWifiPhy>();
proxy->Add(phy);  // Should work exactly like YansWifiChannel

// Test 2: Verify stub message generation
Ptr<RemoteYansWifiChannelStub> stub = CreateObject<RemoteYansWifiChannelStub>();
stub->SetLocalDeviceRank(1);
stub->SetRemoteChannelRank(0);
stub->Add(phy);  // Should generate DEVICE_REGISTER message

// Test 3: Verify channel-side stub behavior
Ptr<RemoteYansWifiPhyStub> channelStub = CreateObject<RemoteYansWifiPhyStub>();
channelStub->SetRemoteDeviceRank(1);
channelStub->SimulateRxEvent(1e-6, 1024);  // Should generate RX_NOTIFICATION
```

### Integration Test Scenarios
1. **Single Device Registration**: Device stub → Channel communication
2. **Packet Transmission**: Complete transmission workflow
3. **Reception Notification**: Channel → Device communication
4. **Configuration Updates**: Propagation model changes
5. **Multi-Device Simulation**: Multiple device stubs on channel

## Performance Optimization Considerations

### Memory Efficiency
- Use object pools for frequently created/destroyed message objects
- Implement efficient serialization for WiFi data structures
- Minimize memory allocations in critical paths

### Message Batching Strategy
```cpp
class MessageBatcher {
    std::vector<MpiMessage> m_pendingMessages;
    Timer m_batchTimer;
    static const size_t MAX_BATCH_SIZE = 100;
    static const Time MAX_BATCH_DELAY = MilliSeconds(1);
    
    void FlushBatch();  // Send all pending messages
    void AddMessage(const MpiMessage& msg);
};
```

### Non-Blocking MPI Implementation
```cpp
void SendAsyncMpiMessage(const MpiMessage& msg, uint32_t targetRank) {
    MPI_Request request;
    MPI_Isend(msg.data(), msg.size(), MPI_BYTE, targetRank, msg.tag(), MPI_COMM_WORLD, &request);
    
    // Store request for later completion check
    m_pendingRequests.push_back(request);
}
```

This technical implementation provides the foundation for transitioning from logging-only stubs to a full MPI-based distributed WiFi simulation system in ns-3.
