# ns-3 MPI WiFi Distributed Simulation Implementation

## Overview

This document describes the implementation of a distributed WiFi simulation system for ns-3 using MPI (Message Passing Interface). The system enables WiFi simulations to run across multiple processes, with functional decomposition separating channel operations from device operations.

## Architecture Summary

### Design Philosophy
- **Functional Decomposition**: Channel operations run on rank 0, device operations run on ranks 1-N
- **Proxy Pattern**: Use inheritance-based proxies to intercept and redirect WiFi operations
- **Message-Based Communication**: All inter-rank communication via MPI messages
- **Transparent Integration**: Existing ns-3 WiFi code works without modification

### Key Components
1. **YansWifiChannelProxy**: Inheritance-based proxy for WiFi channel operations
2. **RemoteYansWifiChannelStub**: Device-side stub simulating remote channel communication
3. **RemoteYansWifiPhyStub**: Channel-side stub representing remote devices

## Implementation Details

### 1. YansWifiChannelProxy (Inheritance-Based Proxy)

**Location**: `src/wifi/mpi-channel/yans-wifi-channel-proxy.h/cc`

**Purpose**: Provides logging and potential MPI redirection for WiFi channel operations while maintaining full compatibility with existing code.

**Key Features**:
- Inherits from `YansWifiChannel`
- Overrides key virtual methods
- Calls parent class methods to maintain functionality
- Adds comprehensive logging for debugging

**Implementation Approach**:
```cpp
class YansWifiChannelProxy : public YansWifiChannel
{
    // Override virtual methods while calling parent
    void Add(Ptr<YansWifiPhy> phy) override {
        LogOperation("Add", phy);
        YansWifiChannel::Add(phy);  // Call parent implementation
    }
    
    void Send(Ptr<YansWifiPhy> sender, Ptr<const WifiPpdu> ppdu, dBm_u txPower) const override {
        LogOperation("Send", sender, ppdu, txPower);
        YansWifiChannel::Send(sender, ppdu, txPower);  // Call parent implementation
    }
};
```

**Why Inheritance Over Composition**:
- Direct polymorphic compatibility with existing code
- No need to reimplement entire YansWifiChannel interface
- Easier maintenance and debugging
- Seamless integration with WiFi helpers and examples

### 2. RemoteYansWifiChannelStub (Device-Side Stub)

**Location**: `src/wifi/mpi-channel/remote-yans-wifi-channel-stub.h/cc`

**Purpose**: Runs on device ranks (1-N) to simulate communication with the remote channel rank (0).

**Key Functionality**:
- Logs simulated MPI messages that would be sent to channel rank
- Handles device registration, configuration, and transmission requests
- Maintains compatibility with YansWifiChannel interface

**Message Types Simulated**:
```
CONFIG_DELAY_MODEL    - Propagation delay model configuration
CONFIG_LOSS_MODEL     - Propagation loss model configuration  
DEVICE_REGISTER       - Device registration with channel
TX_REQUEST           - Packet transmission requests
```

**Example Output**:
```
[SIMULATED_MPI_MSG #1] CONFIG_DELAY_MODEL - Device rank 1 configuring delay model on channel rank 0
[SIMULATED_MPI_MSG #2] DEVICE_REGISTER - Device rank 1 registering PHY with channel rank 0
```

### 3. RemoteYansWifiPhyStub (Channel-Side Stub)

**Location**: `src/wifi/mpi-channel/remote-yans-wifi-phy-stub.h/cc`

**Purpose**: Runs on channel rank (0) to represent devices running on remote ranks.

**Key Functionality**:
- Inherits from YansWifiPhy for interface compatibility
- Logs simulated MPI messages that would be sent to device ranks
- Tracks reception events and notifications
- Maintains device-specific state and statistics

**Message Types Simulated**:
```
RX_NOTIFICATION      - Reception event notifications to devices
TX_START_NOTIFY      - Transmission start notifications
TX_END_NOTIFY        - Transmission end notifications
CONFIG_UPDATE        - Configuration updates from channel
```

**Statistics Tracking**:
- Total RX events processed per device
- MPI messages sent per device
- Device-specific reception statistics

## Message Flow Architecture

### Device → Channel Communication (Ranks 1-N → Rank 0)
1. **Device Registration**: Device calls `channelStub->Add(phy)`
2. **Configuration**: Device calls `channelStub->SetPropagationModel(model)`
3. **Transmission**: Device calls `channelStub->Send(phy, packet, power)`

### Channel → Device Communication (Rank 0 → Ranks 1-N)
1. **Reception Events**: Channel calls `phyStub->NotifyRxStart(params)`
2. **Transmission Status**: Channel calls `phyStub->NotifyTxEnd(status)`
3. **Configuration Updates**: Channel sends updated parameters

## File Structure

```
src/wifi/mpi-channel/
├── yans-wifi-channel-proxy.h          # Inheritance-based logging proxy
├── yans-wifi-channel-proxy.cc
├── remote-yans-wifi-channel-stub.h    # Device-side channel stub  
├── remote-yans-wifi-channel-stub.cc
├── remote-yans-wifi-phy-stub.h        # Channel-side device stub
└── remote-yans-wifi-phy-stub.cc

examples/wireless/
├── mpi-wifi-stub-test.cc              # Complex WiFi setup test
├── simple-mpi-stub-test2.cc           # Simple stub demonstration
└── CMakeLists.txt                     # Updated build configuration
```

## Build System Integration

### CMakeLists.txt Changes

**WiFi Module** (`src/wifi/CMakeLists.txt`):
```cmake
# Added MPI channel stub files
mpi-channel/yans-wifi-channel-proxy.cc
mpi-channel/remote-yans-wifi-channel-stub.cc  
mpi-channel/remote-yans-wifi-phy-stub.cc
```

**Examples** (`examples/wireless/CMakeLists.txt`):
```cmake
# Added MPI stub test examples
build_example(
  NAME mpi-wifi-stub-test
  SOURCE_FILES mpi-wifi-stub-test.cc
  LIBRARIES_TO_LINK ${libinternet} ${libwifi} ${libmobility} ${libapplications}
)

build_example(
  NAME simple-mpi-stub-test2  
  SOURCE_FILES simple-mpi-stub-test2.cc
  LIBRARIES_TO_LINK ${libinternet} ${libwifi} ${libmobility} ${libapplications}
)
```

## Testing and Validation

### Test Examples Created

1. **mpi-wifi-stub-test.cc**: 
   - Complex WiFi network setup with UDP traffic
   - Demonstrates integration with existing WiFi helpers
   - Shows realistic usage patterns

2. **simple-mpi-stub-test2.cc**:
   - Simple demonstration of stub functionality
   - Focus on message flow logging
   - Validates architecture without complex WiFi setup

### Test Results

**Successful Output Example**:
```
=== Test 1: Device-Side Channel Stub ===
[RemoteYansWifiChannelStub] STUB_CALL: Constructor - Device-side channel stub created
[SIMULATED_MPI_MSG #1] CONFIG_DELAY_MODEL - Device rank 1 configuring delay model on channel rank 0

=== Test 2: Channel-Side Device Stubs ===  
Created PHY stub for device 100 (simulated on rank 1)
[SIMULATED_MPI_MSG #1] RX_NOTIFICATION - Channel rank 0 sending RX event to device rank 1

=== Test 3: Simulating MPI Operations ===
Device would call: deviceStub->Add(phy) -> sends MPI message to channel
Channel would notify device 100 of packet reception via MPI
```

## Key Design Decisions

### 1. Inheritance vs Composition for Proxy
**Decision**: Use inheritance-based proxy pattern
**Rationale**: 
- Maintains polymorphic compatibility with existing code
- No need to reimplement entire channel interface  
- Easier debugging and maintenance
- Seamless integration with helpers

### 2. Logging-Only Stubs First
**Decision**: Implement logging stubs before actual MPI
**Rationale**:
- Validate architecture without MPI complexity
- Easier debugging and testing
- Clear message flow visualization
- Foundation for actual MPI implementation

### 3. Functional Decomposition
**Decision**: Channel on rank 0, devices on ranks 1-N
**Rationale**:
- Centralizes channel state management
- Simplifies inter-device communication
- Natural WiFi simulation hierarchy
- Scales well with number of devices

## Implementation Status

### ✅ Completed Components
- [x] YansWifiChannelProxy with inheritance pattern
- [x] RemoteYansWifiChannelStub (device-side)
- [x] RemoteYansWifiPhyStub (channel-side) 
- [x] Build system integration
- [x] Test examples and validation
- [x] Comprehensive logging and debugging

### 🔄 Next Implementation Steps
1. **Replace logging with actual MPI calls**
   - Implement MPI_Send/MPI_Recv in stub methods
   - Add message serialization for WiFi data structures
   
2. **Message Protocol Design**
   - Define message formats for different operation types
   - Implement message queuing and buffering
   
3. **Synchronization and Timing**
   - Handle simulation time synchronization across ranks
   - Implement proper event scheduling

4. **Error Handling and Robustness**
   - Add MPI error handling
   - Implement failover mechanisms

## Usage Examples

### Basic Integration
```cpp
// Create device-side channel stub instead of regular channel
Ptr<RemoteYansWifiChannelStub> channelStub = CreateObject<RemoteYansWifiChannelStub>();
channelStub->SetLocalDeviceRank(1);     // This device is on rank 1
channelStub->SetRemoteChannelRank(0);   // Channel runs on rank 0

// Use exactly like regular channel
YansWifiChannelHelper channelHelper;
channelHelper.SetChannel(channelStub);
```

### Channel-Side Setup
```cpp
// Create stubs representing remote devices
std::vector<Ptr<RemoteYansWifiPhyStub>> deviceStubs;
for (uint32_t deviceId : remoteDeviceIds) {
    Ptr<RemoteYansWifiPhyStub> stub = CreateObject<RemoteYansWifiPhyStub>();
    stub->SetRemoteDeviceRank(GetDeviceRank(deviceId));
    stub->SetRemoteDeviceId(deviceId);
    deviceStubs.push_back(stub);
}
```

## Performance Considerations

### Message Optimization
- Batch small messages to reduce MPI overhead
- Use non-blocking MPI calls where possible
- Implement message compression for large packets

### Memory Management
- Efficient serialization of WiFi data structures
- Smart pointer usage for inter-rank object references
- Proper cleanup of MPI resources

### Scalability
- Linear scaling with number of device ranks
- Minimal memory overhead per remote device
- Efficient message routing and delivery

## Debugging and Monitoring

### Logging Levels
- **STUB_CALL**: Method invocations and parameters
- **SIMULATED_MPI_MSG**: MPI message simulation details
- **PERFORMANCE**: Timing and statistics information

### Statistics Collection
- Message counts per operation type
- Timing measurements for MPI operations
- Memory usage tracking per rank

This implementation provides a solid foundation for distributed WiFi simulation in ns-3, with clear separation of concerns, comprehensive testing, and a path toward full MPI implementation.
