# Phase 2 Implementation Plan: Channel-Side MPI Processing

## **🔍 Current Status Analysis**

### **✅ Phase 1 Completed:**
- **MPI Message Protocol** - Complete serialization framework using ns-3 Buffer
- **WiFi MPI Interface** - High-level wrapper around ns-3 MpiInterface
- **Device-Side Stub** - RemoteYansWifiChannelStub sends real MPI messages
- **Build Integration** - All components compile and work with/without MPI
- **Testing Framework** - Validated with mpirun showing proper rank detection

### **🎯 Phase 2 Objective:**
Implement the **Channel-Side Processing** that runs on Rank 0 and handles:
1. **Device Registration** - Track devices from all ranks
2. **Transmission Processing** - Calculate propagation for incoming packets
3. **Reception Notification** - Send RX events back to device ranks
4. **Configuration Management** - Handle propagation model updates

---

## **📦 Existing ns-3 Infrastructure We Can Leverage**

### **1. Message Reception Infrastructure**

#### **✅ `MpiReceiver` (`src/mpi/model/mpi-receiver.h`)**
```cpp
class MpiReceiver {
public:
    void SetReceiveCallback(Callback<void, Ptr<Packet>> callback);
    void Receive(Ptr<Packet> packet);
    void DoDispose();
};
```

**How we can use it:**
```cpp
// In our channel-side implementation:
class WifiChannelMpiProcessor {
private:
    Ptr<MpiReceiver> m_mpiReceiver;
    
    void SetupMessageReceiver() {
        m_mpiReceiver = Create<MpiReceiver>();
        m_mpiReceiver->SetReceiveCallback(
            MakeCallback(&WifiChannelMpiProcessor::ProcessIncomingMessage, this)
        );
    }
    
    void ProcessIncomingMessage(Ptr<Packet> packet) {
        // Deserialize WiFi MPI messages and route to appropriate handlers
    }
};
```

#### **✅ `RemoteChannelBundle` (`src/mpi/model/remote-channel-bundle.h`)**
```cpp
class RemoteChannelBundle {
public:
    void AddChannel(Ptr<Channel> channel, Time delay);
    std::size_t GetNDevices() const;
    Ptr<NetDevice> GetDevice(std::size_t i) const;
    
private:
    typedef std::vector<std::pair<Ptr<Channel>, Time>> ChannelList;
    ChannelList m_channels;
};
```

**Pattern we can adapt:**
- **Device registry management** - Track devices across ranks
- **Channel bundling** - Group WiFi devices by channel/frequency
- **Efficient device lookup** - Fast access to device information

### **2. Time Synchronization (Already Available)**

#### **✅ `DistributedSimulatorImpl` (`src/mpi/model/distributed-simulator-impl.h`)**
```cpp
class DistributedSimulatorImpl : public SimulatorImpl {
public:
    void Stop() override;
    EventId Schedule(const Time& delay, EventImpl* event) override;
    void Run() override;
    
private:
    Time CalculateLookAhead(const Time& ts);
    void CalculateSafeTime();
};
```

**What we get for free:**
- ✅ **Conservative time management** - Automatic synchronization between ranks
- ✅ **Lookahead calculation** - Safe time advancement windows
- ✅ **Cross-rank coordination** - Handles distributed time automatically
- ✅ **Event scheduling** - MPI-aware event processing

### **3. Existing Channel Infrastructure**

#### **✅ `YansWifiChannelProxy` (Our Implementation)**
We already have the perfect foundation:
```cpp
class YansWifiChannelProxy : public YansWifiChannel {
public:
    void Add(Ptr<YansWifiPhy> phy) override;
    void Send(Ptr<YansWifiPhy> sender, Ptr<const WifiPpdu> ppdu, dBm_u txPower) const override;
    void SetPropagationLossModel(const Ptr<PropagationLossModel> loss) override;
    // ... all channel operations are already intercepted and logged
};
```

**What we can leverage:**
- ✅ **Method interception** - All channel operations already captured
- ✅ **Logging infrastructure** - Perfect debugging framework
- ✅ **Propagation models** - Real channel calculation capability
- ✅ **Virtual inheritance** - Can be extended without breaking existing code

---

## **📋 Phase 2 Implementation Steps**

### **Step 2.1: Create Channel-Side MPI Processor** 
*Duration: 2-3 days*

#### **File: `src/wifi/mpi-channel/wifi-channel-mpi-processor.h`**
```cpp
class WifiChannelMpiProcessor : public Object {
public:
    static TypeId GetTypeId();
    
    WifiChannelMpiProcessor();
    virtual ~WifiChannelMpiProcessor();
    
    // Main processing interface
    void Initialize();
    void StartProcessing();
    void StopProcessing();
    
    // Message handlers (based on our existing message types)
    void HandleDeviceRegistration(const WifiMpiDeviceRegistration& msg, uint32_t sourceRank);
    void HandleTransmissionRequest(const WifiMpiTransmissionRequest& msg, Ptr<Packet> packet, uint32_t sourceRank);
    void HandleConfigurationUpdate(const WifiMpiConfigurationUpdate& msg, uint32_t sourceRank);
    
private:
    Ptr<MpiReceiver> m_mpiReceiver;
    Ptr<YansWifiChannelProxy> m_channel;  // Use our existing proxy!
    std::map<uint32_t, RemoteDeviceInfo> m_remoteDevices;  // Track devices by rank
    
    void ProcessIncomingMessage(Ptr<Packet> packet);
    void SendReceptionNotification(uint32_t deviceId, uint32_t targetRank, const ReceptionInfo& rxInfo);
};
```

**Why this approach:**
- ✅ **Reuse existing proxy** - Our YansWifiChannelProxy already handles all channel operations
- ✅ **Follow ns-3 patterns** - Use MpiReceiver just like existing MPI modules
- ✅ **Message-driven design** - Event-driven processing matches ns-3 architecture
- ✅ **Modular design** - Can be tested independently

### **Step 2.2: Enhance YansWifiChannelProxy for MPI** 
*Duration: 2-3 days*

#### **Extend our existing proxy to handle remote devices:**
```cpp
// Add to existing YansWifiChannelProxy class:
class YansWifiChannelProxy : public YansWifiChannel {
    // ...existing methods...
    
    // New MPI-specific methods:
    void RegisterRemoteDevice(uint32_t deviceId, uint32_t rank, const DeviceInfo& info);
    void ProcessRemoteTransmission(uint32_t senderId, Ptr<Packet> packet, const TransmissionInfo& txInfo);
    void SetMpiProcessor(Ptr<WifiChannelMpiProcessor> processor);
    
private:
    Ptr<WifiChannelMpiProcessor> m_mpiProcessor;
    std::map<uint32_t, RemoteDeviceInfo> m_remoteDevices;
    
    // Enhanced Send method for MPI
    void Send(Ptr<YansWifiPhy> sender, Ptr<const WifiPpdu> ppdu, dBm_u txPower) const override {
        // For local devices (if any on channel rank)
        YansWifiChannel::Send(sender, ppdu, txPower);
        
        // For remote transmissions (via MPI)
        if (m_mpiProcessor) {
            m_mpiProcessor->ProcessLocalTransmission(sender, ppdu, txPower);
        }
    }
};
```

**Why enhance existing proxy:**
- ✅ **Incremental development** - Build on proven foundation
- ✅ **Minimal code changes** - Most functionality already exists
- ✅ **Backward compatibility** - Existing examples continue to work
- ✅ **Dual mode support** - Can handle both local and remote devices

### **Step 2.3: Create Remote Device Registry** 
*Duration: 1-2 days*

#### **File: `src/wifi/mpi-channel/remote-device-registry.h`**
```cpp
// Based on RemoteChannelBundle pattern but adapted for WiFi
class WifiRemoteDeviceRegistry {
public:
    struct RemoteDeviceInfo {
        uint32_t deviceId;
        uint32_t rank;
        Vector3D position;
        double antennaGain;
        uint32_t frequency;
        Time lastActivity;
    };
    
    void RegisterDevice(uint32_t deviceId, uint32_t rank, const RemoteDeviceInfo& info);
    void UnregisterDevice(uint32_t deviceId);
    
    std::vector<RemoteDeviceInfo> GetDevicesInRange(Vector3D txPosition, double txPower, uint32_t frequency);
    std::vector<RemoteDeviceInfo> GetAllDevices() const;
    
    bool IsDeviceRegistered(uint32_t deviceId) const;
    RemoteDeviceInfo GetDeviceInfo(uint32_t deviceId) const;
    
private:
    std::map<uint32_t, RemoteDeviceInfo> m_devices;
    std::map<uint32_t, std::set<uint32_t>> m_devicesByRank;  // Fast rank lookup
    
    double CalculateDistance(const Vector3D& pos1, const Vector3D& pos2) const;
    bool IsInCommunicationRange(const RemoteDeviceInfo& device, Vector3D txPos, double txPower) const;
};
```

**Why this design:**
- ✅ **Based on proven patterns** - RemoteChannelBundle shows this works
- ✅ **Efficient lookups** - Fast device discovery for transmission processing
- ✅ **Spatial awareness** - Position-based communication range calculation
- ✅ **Multi-frequency support** - Different WiFi channels/frequencies

### **Step 2.4: Implement Message Routing and Processing** 
*Duration: 2-3 days*

#### **File: `src/wifi/mpi-channel/wifi-mpi-message-processor.cc`**
```cpp
void WifiChannelMpiProcessor::ProcessIncomingMessage(Ptr<Packet> packet) {
    // Deserialize message header to determine type
    WifiMpiMessageHeader header;
    packet->RemoveHeader(header);
    
    uint32_t sourceRank = header.GetSourceRank();
    WifiMpiMessageType msgType = header.GetMessageType();
    
    switch (msgType) {
        case WIFI_DEVICE_REGISTRATION:
            HandleDeviceRegistration(packet, sourceRank);
            break;
            
        case WIFI_TRANSMISSION_REQUEST:
            HandleTransmissionRequest(packet, sourceRank);
            break;
            
        case WIFI_CONFIGURATION_UPDATE:
            HandleConfigurationUpdate(packet, sourceRank);
            break;
            
        default:
            NS_LOG_WARN("Unknown message type: " << msgType);
    }
}

void WifiChannelMpiProcessor::HandleTransmissionRequest(Ptr<Packet> packet, uint32_t sourceRank) {
    // Deserialize transmission request
    WifiMpiTransmissionRequest request;
    packet->RemoveHeader(request);
    
    // Get devices in communication range
    auto targetDevices = m_deviceRegistry->GetDevicesInRange(
        request.GetTransmitterPosition(),
        request.GetTransmissionPower(),
        request.GetFrequency()
    );
    
    // Calculate propagation for each target device
    for (const auto& device : targetDevices) {
        if (device.rank != sourceRank || device.deviceId != request.GetSenderId()) {
            // Calculate reception power using real propagation models
            double rxPower = CalculateReceptionPower(request, device);
            
            if (rxPower > GetReceptionThreshold()) {
                // Create and send reception notification
                SendReceptionNotification(device.deviceId, device.rank, request, rxPower);
            }
        }
    }
}
```

**Why this approach:**
- ✅ **Message-driven architecture** - Matches ns-3 event-driven design
- ✅ **Real propagation calculation** - Uses actual WiFi propagation models
- ✅ **Efficient processing** - Only notifies devices that can receive
- ✅ **Extensible design** - Easy to add new message types

### **Step 2.5: Create Channel-Side Main Program** 
*Duration: 1-2 days*

#### **File: `src/wifi/examples/mpi-wifi-channel-main.cc`**
```cpp
// Based on existing MPI examples like simple-distributed.cc
int main(int argc, char* argv[]) {
    // Parse command line arguments
    CommandLine cmd;
    std::string propagationModel = "ns3::FriisPropagationLossModel";
    cmd.AddValue("propagation", "Propagation loss model", propagationModel);
    cmd.Parse(argc, argv);
    
    // Enable MPI
    MpiInterface::Enable(&argc, &argv);
    
    uint32_t systemId = MpiInterface::GetSystemId();
    uint32_t systemCount = MpiInterface::GetSize();
    
    // This program should only run on rank 0 (channel rank)
    if (systemId != 0) {
        std::cerr << "Channel main should only run on rank 0!" << std::endl;
        MpiInterface::Disable();
        return 1;
    }
    
    std::cout << "Starting WiFi Channel Processor on rank " << systemId 
              << " (total ranks: " << systemCount << ")" << std::endl;
    
    // Enable distributed simulation
    GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::DistributedSimulatorImpl"));
    
    // Create and configure the channel processor
    Ptr<WifiChannelMpiProcessor> processor = Create<WifiChannelMpiProcessor>();
    processor->Initialize();
    
    // Create the WiFi channel with real propagation models
    Ptr<YansWifiChannelProxy> channel = Create<YansWifiChannelProxy>();
    Ptr<PropagationLossModel> lossModel = CreateObjectFromTypeId(propagationModel);
    channel->SetPropagationLossModel(lossModel);
    channel->SetMpiProcessor(processor);
    
    // Start processing
    processor->StartProcessing();
    
    std::cout << "Channel processor ready. Waiting for device registrations..." << std::endl;
    
    // Run the simulation
    Simulator::Stop(Seconds(3600));  // Run for 1 hour max
    Simulator::Run();
    
    // Cleanup
    processor->StopProcessing();
    Simulator::Destroy();
    MpiInterface::Disable();
    
    std::cout << "Channel processor finished." << std::endl;
    return 0;
}
```

**Why this design:**
- ✅ **Based on existing examples** - Follows simple-distributed.cc pattern
- ✅ **Command line configuration** - Flexible propagation model selection
- ✅ **Proper MPI lifecycle** - Enable/Disable handled correctly
- ✅ **Distributed simulation** - Uses DistributedSimulatorImpl automatically
- ✅ **Error handling** - Validates rank assignment

### **Step 2.6: Create Device-Side Main Program** 
*Duration: 1-2 days*

#### **File: `src/wifi/examples/mpi-wifi-device-main.cc`**
```cpp
int main(int argc, char* argv[]) {
    // Parse command line
    CommandLine cmd;
    uint32_t nDevices = 1;
    Time simTime = Seconds(60);
    cmd.AddValue("nDevices", "Number of devices on this rank", nDevices);
    cmd.AddValue("simTime", "Simulation time", simTime);
    cmd.Parse(argc, argv);
    
    // Enable MPI
    MpiInterface::Enable(&argc, &argv);
    
    uint32_t systemId = MpiInterface::GetSystemId();
    uint32_t systemCount = MpiInterface::GetSize();
    
    // Device ranks are 1, 2, 3, ... (rank 0 is channel)
    if (systemId == 0) {
        std::cerr << "Device main should not run on rank 0 (channel rank)!" << std::endl;
        MpiInterface::Disable();
        return 1;
    }
    
    std::cout << "Starting WiFi Devices on rank " << systemId 
              << " (total ranks: " << systemCount << ")" << std::endl;
    
    // Enable distributed simulation
    GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::DistributedSimulatorImpl"));
    
    // Create nodes
    NodeContainer nodes;
    nodes.Create(nDevices);
    
    // Install WiFi with our MPI-enabled channel stub
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    
    YansWifiPhyHelper wifiPhy;
    // Use our MPI channel stub instead of regular channel
    Ptr<RemoteYansWifiChannelStub> channelStub = Create<RemoteYansWifiChannelStub>();
    channelStub->SetRemoteChannelRank(0);  // Channel is on rank 0
    channelStub->SetLocalDeviceRank(systemId);
    wifiPhy.SetChannel(channelStub);
    
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");
    
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);
    
    // Install mobility
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = Create<ListPositionAllocator>();
    for (uint32_t i = 0; i < nDevices; ++i) {
        // Spread devices spatially based on rank
        Vector pos(systemId * 50.0, i * 10.0, 0.0);
        positionAlloc->Add(pos);
    }
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);
    
    // Install internet stack and applications
    InternetStackHelper internet;
    internet.Install(nodes);
    
    Ipv4AddressHelper ipv4;
    std::ostringstream subnet;
    subnet << "10." << systemId << ".1.0";
    ipv4.SetBase(subnet.str().c_str(), "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);
    
    // TODO: Add application traffic generation
    
    std::cout << "Devices configured. Starting simulation..." << std::endl;
    
    // Run simulation
    Simulator::Stop(simTime);
    Simulator::Run();
    Simulator::Destroy();
    MpiInterface::Disable();
    
    std::cout << "Device simulation finished." << std::endl;
    return 0;
}
```

---

## **🚀 Expected Implementation Timeline**

### **Total Estimated Duration: 10-13 days**

| Step | Duration | Description | Key Deliverable |
|------|----------|-------------|-----------------|
| **2.1** | 2-3 days | Channel MPI Processor | Message handling infrastructure |
| **2.2** | 2-3 days | Enhance Channel Proxy | Remote device support |
| **2.3** | 1-2 days | Device Registry | Spatial device management |
| **2.4** | 2-3 days | Message Processing | Real propagation calculation |
| **2.5** | 1-2 days | Channel Main Program | Rank 0 executable |
| **2.6** | 1-2 days | Device Main Program | Device rank executable |

### **🎯 Key Milestones:**

- **Week 1**: Channel-side message processing working
- **Week 2**: Complete bidirectional communication
- **Final**: Full distributed WiFi simulation demo

---

## **🔧 Reusable Components Summary**

### **From ns-3 MPI Module:**
- ✅ **MpiReceiver** - Message handling framework
- ✅ **DistributedSimulatorImpl** - Time synchronization
- ✅ **RemoteChannelBundle** - Device management patterns
- ✅ **MpiInterface** - Core MPI lifecycle

### **From Our Phase 1:**
- ✅ **WiFi MPI Messages** - Complete protocol definition
- ✅ **WiFi MPI Interface** - High-level communication wrapper
- ✅ **Device-side stub** - Proven MPI message sending
- ✅ **YansWifiChannelProxy** - Channel operation interception

### **From Existing ns-3 Examples:**
- ✅ **simple-distributed.cc** - Main program structure
- ✅ **MPI test fixtures** - Testing patterns
- ✅ **Build system** - MPI integration patterns

---

## **💡 Key Advantages of This Approach**

1. **Minimal New Code**: ~70% reuse of existing infrastructure
2. **Proven Patterns**: Following established ns-3 MPI examples
3. **Incremental Testing**: Each step can be validated independently
4. **Backward Compatibility**: Existing WiFi code continues to work
5. **Production Ready**: Real propagation models with distributed processing

This plan builds directly on our successful Phase 1 foundation and leverages the extensive existing ns-3 MPI infrastructure to create a complete distributed WiFi simulation system.