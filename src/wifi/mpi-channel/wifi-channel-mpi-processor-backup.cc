#include "wifi-channel-mpi-processor.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/string.h"

// Debug: Check if NS3_MPI is defined during compilation
#ifdef NS3_MPI
#pragma message("COMPILING WITH NS3_MPI DEFINED")
#include "wifi-mpi-interface.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/wifi-ppdu.h"
#include <algorithm>
#include <cmath>
#else
#pragma message("COMPILING WITHOUT NS3_MPI - STUB VERSION")
#endif

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiChannelMpiProcessor");

// Always register the class, but with different implementations
NS_OBJECT_ENSURE_REGISTERED(WifiChannelMpiProcessor);

TypeId
WifiChannelMpiProcessor::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WifiChannelMpiProcessor")
                            .SetParent<Object>()
                            .SetGroupName("Wifi")
                            .AddConstructor<WifiChannelMpiProcessor>();
                            
#ifdef NS3_MPI
    tid.AddAttribute("ReceptionThreshold",
                     "Minimum reception power threshold in dBm",
                     DoubleValue(-85.0),
                     MakeDoubleAccessor(&WifiChannelMpiProcessor::m_receptionThresholdDbm),
                     MakeDoubleChecker<double>(-200.0, 0.0));
#endif
    
    return tid;
}

#ifdef NS3_MPI

// Full MPI implementation

WifiChannelMpiProcessor::WifiChannelMpiProcessor()
    : m_initialized(false),
      m_processing(false),
      m_receptionThresholdDbm(-85.0),
      m_nextMessageId(1),
      m_totalMessages(0),
      m_processedTransmissions(0),
      m_sentNotifications(0),
      m_registeredDevices(0)
{
    std::cout << "[DEBUG] MPI WifiChannelMpiProcessor constructor called" << std::endl;
    NS_LOG_FUNCTION(this);
    LogActivity("Constructor", "WiFi Channel MPI Processor created");
}

WifiChannelMpiProcessor::~WifiChannelMpiProcessor()
{
    NS_LOG_FUNCTION(this);
    if (m_processing)
    {
        StopProcessing();
    }
    LogActivity("Destructor", "WiFi Channel MPI Processor destroyed");
}

void
WifiChannelMpiProcessor::DoDispose()
{
    NS_LOG_FUNCTION(this);
    
    StopProcessing();
    
    m_mpiReceiver = nullptr;
    m_channelProxy = nullptr;
    m_remoteDevices.clear();
    m_devicesByRank.clear();
    
    Object::DoDispose();
}

bool
WifiChannelMpiProcessor::Initialize()
{
    std::cout << "[DEBUG] Initialize() called" << std::endl;
    NS_LOG_FUNCTION(this);
    
    if (m_initialized)
    {
        std::cout << "[DEBUG] Already initialized" << std::endl;
        NS_LOG_WARN("Processor already initialized");
        return true;
    }
    
    std::cout << "[DEBUG] Checking MPI status" << std::endl;
    
    // Check if MPI is enabled
    if (!MpiInterface::IsEnabled())
    {
        NS_LOG_ERROR("MPI is not enabled - cannot initialize processor");
        LogActivity("Initialize", "Failed - MPI not enabled");
        return false;
    }
    
    // Verify we're on rank 0 (channel rank)
    uint32_t rank = MpiInterface::GetSystemId();
    if (rank != 0)
    {
        NS_LOG_ERROR("Channel MPI Processor should only run on rank 0, current rank: " << rank);
        LogActivity("Initialize", "Failed - wrong rank: " + std::to_string(rank));
        return false;
    }
    
    // For now, skip MpiReceiver creation - we'll add message handling later
    // TODO: Implement proper message reception mechanism
    LogActivity("Initialize", "MPI receiver setup skipped for initial implementation");
    
    m_initialized = true;
    
    LogActivity("Initialize", "Successfully initialized on rank " + std::to_string(rank) + 
                             ", MPI size: " + std::to_string(MpiInterface::GetSize()));
    
    return true;
}

void
WifiChannelMpiProcessor::StartProcessing()
{
    NS_LOG_FUNCTION(this);
    
    if (!m_initialized)
    {
        NS_LOG_ERROR("Cannot start processing - processor not initialized");
        return;
    }
    
    if (m_processing)
    {
        NS_LOG_WARN("Processor already processing messages");
        return;
    }
    
    m_processing = true;
    LogActivity("StartProcessing", "Started message processing");
}

void
WifiChannelMpiProcessor::StopProcessing()
{
    NS_LOG_FUNCTION(this);
    
    if (!m_processing)
    {
        return;
    }
    
    m_processing = false;
    LogActivity("StopProcessing", "Stopped message processing");
}

bool
WifiChannelMpiProcessor::IsProcessing() const
{
    return m_processing;
}

void
WifiChannelMpiProcessor::SetChannelProxy(Ptr<YansWifiChannelProxy> channel)
{
    NS_LOG_FUNCTION(this << channel);
    m_channelProxy = channel;
    LogActivity("SetChannelProxy", "Channel proxy configured");
}

Ptr<YansWifiChannelProxy>
WifiChannelMpiProcessor::GetChannelProxy() const
{
    return m_channelProxy;
}

void
WifiChannelMpiProcessor::SetReceptionThreshold(double thresholdDbm)
{
    NS_LOG_FUNCTION(this << thresholdDbm);
    m_receptionThresholdDbm = thresholdDbm;
    LogActivity("SetReceptionThreshold", "Threshold set to " + std::to_string(thresholdDbm) + " dBm");
}

double
WifiChannelMpiProcessor::GetReceptionThreshold() const
{
    return m_receptionThresholdDbm;
}

uint32_t
WifiChannelMpiProcessor::GetNumRegisteredDevices() const
{
    return m_remoteDevices.size();
}

uint32_t
WifiChannelMpiProcessor::GetNumDevicesOnRank(uint32_t rank) const
{
    auto it = m_devicesByRank.find(rank);
    return (it != m_devicesByRank.end()) ? it->second.size() : 0;
}

std::vector<uint32_t>
WifiChannelMpiProcessor::GetRegisteredDeviceIds() const
{
    std::vector<uint32_t> ids;
    ids.reserve(m_remoteDevices.size());
    
    for (const auto& device : m_remoteDevices)
    {
        ids.push_back(device.first);
    }
    
    return ids;
}

bool
WifiChannelMpiProcessor::IsDeviceRegistered(uint32_t deviceId) const
{
    return m_remoteDevices.find(deviceId) != m_remoteDevices.end();
}

RemoteDeviceInfo
WifiChannelMpiProcessor::GetDeviceInfo(uint32_t deviceId) const
{
    auto it = m_remoteDevices.find(deviceId);
    if (it != m_remoteDevices.end())
    {
        return it->second;
    }
    return RemoteDeviceInfo(); // Return default-constructed info
}

uint64_t
WifiChannelMpiProcessor::GetTotalProcessedMessages() const
{
    return m_totalMessages;
}

uint64_t
WifiChannelMpiProcessor::GetProcessedTransmissions() const
{
    return m_processedTransmissions;
}

uint64_t
WifiChannelMpiProcessor::GetSentReceptionNotifications() const
{
    return m_sentNotifications;
}

void
WifiChannelMpiProcessor::PrintStatistics() const
{
    std::cout << "\n=== WiFi Channel MPI Processor Statistics ===" << std::endl;
    std::cout << "Total processed messages: " << m_totalMessages << std::endl;
    std::cout << "Processed transmissions: " << m_processedTransmissions << std::endl;
    std::cout << "Sent reception notifications: " << m_sentNotifications << std::endl;
    std::cout << "Registered devices: " << m_remoteDevices.size() << std::endl;
    std::cout << "Active ranks: " << m_devicesByRank.size() << std::endl;
    
    for (const auto& rankDevices : m_devicesByRank)
    {
        std::cout << "  Rank " << rankDevices.first << ": " << rankDevices.second.size() << " devices" << std::endl;
    }
    
    std::cout << "Reception threshold: " << m_receptionThresholdDbm << " dBm" << std::endl;
    std::cout << "=============================================" << std::endl;
}

void
WifiChannelMpiProcessor::SetDeviceRegistrationCallback(Callback<void, uint32_t, uint32_t> callback)
{
    m_deviceRegistrationCallback = callback;
}

void
WifiChannelMpiProcessor::SetTransmissionProcessingCallback(Callback<void, uint32_t, uint32_t> callback)
{
    m_transmissionProcessingCallback = callback;
}

void
WifiChannelMpiProcessor::ProcessIncomingMessage(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);
    
    if (!m_processing)
    {
        NS_LOG_WARN("Received message while not processing - ignoring");
        return;
    }
    
    m_totalMessages++;
    
    try
    {
        // Extract message header
        WifiMpiMessageHeader header;
        Ptr<Packet> messageCopy = packet->Copy();
        messageCopy->RemoveHeader(header);
        
        uint32_t sourceRank = header.GetSourceRank();
        
        LogActivity("ProcessIncomingMessage", 
                   "Message " + std::to_string(m_totalMessages) + 
                   " from rank " + std::to_string(sourceRank) +
                   ", type: " + std::to_string(static_cast<int>(header.GetMessageType())));
        
        // Route to appropriate handler
        RouteMessage(header, messageCopy, sourceRank);
    }
    catch (const std::exception& e)
    {
        NS_LOG_ERROR("Error processing message: " << e.what());
        LogActivity("ProcessIncomingMessage", "Error: " + std::string(e.what()));
    }
}

void
WifiChannelMpiProcessor::RouteMessage(const WifiMpiMessageHeader& header, 
                                     Ptr<Packet> packet, 
                                     uint32_t sourceRank)
{
    NS_LOG_FUNCTION(this << &header << packet << sourceRank);
    
    WifiMpiMessageType msgType = header.GetMessageType();
    
    switch (msgType)
    {
        case WIFI_DEVICE_REGISTRATION:
            HandleDeviceRegistration(packet, sourceRank);
            break;
            
        case WIFI_TRANSMISSION_REQUEST:
            HandleTransmissionRequest(packet, sourceRank);
            break;
            
        case WIFI_CONFIGURATION_UPDATE:
            HandleConfigurationUpdate(packet, sourceRank);
            break;
            
        case WIFI_DEVICE_UNREGISTRATION:
            HandleDeviceUnregistration(packet, sourceRank);
            break;
            
        default:
            NS_LOG_WARN("Unknown message type: " << static_cast<int>(msgType));
            LogActivity("RouteMessage", "Unknown message type: " + std::to_string(static_cast<int>(msgType)));
    }
}

void
WifiChannelMpiProcessor::HandleDeviceRegistration(Ptr<Packet> packet, uint32_t sourceRank)
{
    NS_LOG_FUNCTION(this << packet << sourceRank);
    
    try
    {
        // Deserialize device registration message
        WifiMpiDeviceRegistration regMsg;
        packet->RemoveHeader(regMsg);
        
        uint32_t deviceId = regMsg.GetDeviceId();
        Vector3D position = regMsg.GetPosition();
        
        // Create device info
        RemoteDeviceInfo deviceInfo(deviceId, sourceRank, position);
        deviceInfo.antennaGainDb = regMsg.GetAntennaGain();
        deviceInfo.frequency = regMsg.GetFrequency();
        deviceInfo.lastActivity = Simulator::Now();
        
        // Register the device
        m_remoteDevices[deviceId] = deviceInfo;
        m_devicesByRank[sourceRank].insert(deviceId);
        m_registeredDevices++;
        
        LogActivity("HandleDeviceRegistration", 
                   "Registered device " + std::to_string(deviceId) + 
                   " from rank " + std::to_string(sourceRank) +
                   " at position (" + std::to_string(position.x) + ", " + 
                   std::to_string(position.y) + ", " + std::to_string(position.z) + ")");
        
        // Trigger callback if set
        if (!m_deviceRegistrationCallback.IsNull())
        {
            m_deviceRegistrationCallback(deviceId, sourceRank);
        }
    }
    catch (const std::exception& e)
    {
        NS_LOG_ERROR("Error handling device registration: " << e.what());
        LogActivity("HandleDeviceRegistration", "Error: " + std::string(e.what()));
    }
}

void
WifiChannelMpiProcessor::HandleTransmissionRequest(Ptr<Packet> packet, uint32_t sourceRank)
{
    NS_LOG_FUNCTION(this << packet << sourceRank);
    
    try
    {
        // Deserialize transmission request
        WifiMpiTransmissionRequest txMsg;
        packet->RemoveHeader(txMsg);
        
        uint32_t senderId = txMsg.GetSenderId();
        double txPowerDbm = txMsg.GetTxPowerDbm();
        uint32_t frequency = txMsg.GetFrequency();
        
        m_processedTransmissions++;
        
        // Find sender device info
        auto senderIt = m_remoteDevices.find(senderId);
        if (senderIt == m_remoteDevices.end())
        {
            NS_LOG_WARN("Transmission from unregistered device: " << senderId);
            LogActivity("HandleTransmissionRequest", "Unregistered sender: " + std::to_string(senderId));
            return;
        }
        
        RemoteDeviceInfo& senderInfo = senderIt->second;
        senderInfo.lastActivity = Simulator::Now();
        
        // Find devices in communication range
        auto targetDevices = GetDevicesInRange(senderInfo, txPowerDbm, frequency);
        
        LogActivity("HandleTransmissionRequest", 
                   "Processing transmission from device " + std::to_string(senderId) +
                   ", power: " + std::to_string(txPowerDbm) + " dBm, targets: " + std::to_string(targetDevices.size()));
        
        // Send reception notifications to target devices
        for (const auto& rxDevice : targetDevices)
        {
            if (rxDevice.deviceId != senderId) // Don't send to self
            {
                double rxPowerDbm = CalculateReceptionPower(senderInfo, rxDevice, txPowerDbm);
                
                if (rxPowerDbm >= m_receptionThresholdDbm)
                {
                    ReceptionInfo rxInfo;
                    rxInfo.transmitterId = senderId;
                    rxInfo.receiverId = rxDevice.deviceId;
                    rxInfo.rxPowerDbm = rxPowerDbm;
                    rxInfo.txPowerDbm = txPowerDbm;
                    rxInfo.propagationDelay = CalculatePropagationDelay(senderInfo, rxDevice);
                    rxInfo.duration = txMsg.GetDuration();
                    rxInfo.packetSize = txMsg.GetPacketSize();
                    
                    SendReceptionNotification(rxDevice, rxInfo);
                }
            }
        }
        
        // Trigger callback if set
        if (!m_transmissionProcessingCallback.IsNull())
        {
            m_transmissionProcessingCallback(senderId, targetDevices.size());
        }
    }
    catch (const std::exception& e)
    {
        NS_LOG_ERROR("Error handling transmission request: " << e.what());
        LogActivity("HandleTransmissionRequest", "Error: " + std::string(e.what()));
    }
}

void
WifiChannelMpiProcessor::HandleConfigurationUpdate(Ptr<Packet> packet, uint32_t sourceRank)
{
    NS_LOG_FUNCTION(this << packet << sourceRank);
    
    // TODO: Implement configuration update handling
    // This would handle propagation model changes, channel parameters, etc.
    LogActivity("HandleConfigurationUpdate", "Configuration update from rank " + std::to_string(sourceRank));
}

void
WifiChannelMpiProcessor::HandleDeviceUnregistration(Ptr<Packet> packet, uint32_t sourceRank)
{
    NS_LOG_FUNCTION(this << packet << sourceRank);
    
    try
    {
        // Deserialize device unregistration message
        WifiMpiDeviceUnregistration unregMsg;
        packet->RemoveHeader(unregMsg);
        
        uint32_t deviceId = unregMsg.GetDeviceId();
        
        // Remove device from registry
        auto deviceIt = m_remoteDevices.find(deviceId);
        if (deviceIt != m_remoteDevices.end())
        {
            uint32_t deviceRank = deviceIt->second.rank;
            m_remoteDevices.erase(deviceIt);
            
            // Remove from rank mapping
            auto rankIt = m_devicesByRank.find(deviceRank);
            if (rankIt != m_devicesByRank.end())
            {
                rankIt->second.erase(deviceId);
                if (rankIt->second.empty())
                {
                    m_devicesByRank.erase(rankIt);
                }
            }
            
            LogActivity("HandleDeviceUnregistration", 
                       "Unregistered device " + std::to_string(deviceId) + 
                       " from rank " + std::to_string(sourceRank));
        }
        else
        {
            NS_LOG_WARN("Unregistration for unknown device: " << deviceId);
            LogActivity("HandleDeviceUnregistration", "Unknown device: " + std::to_string(deviceId));
        }
    }
    catch (const std::exception& e)
    {
        NS_LOG_ERROR("Error handling device unregistration: " << e.what());
        LogActivity("HandleDeviceUnregistration", "Error: " + std::string(e.what()));
    }
}

std::vector<RemoteDeviceInfo>
WifiChannelMpiProcessor::GetDevicesInRange(const RemoteDeviceInfo& txDevice, 
                                          double txPowerDbm, 
                                          uint32_t frequency)
{
    NS_LOG_FUNCTION(this << &txDevice << txPowerDbm << frequency);
    
    std::vector<RemoteDeviceInfo> devicesInRange;
    
    for (const auto& devicePair : m_remoteDevices)
    {
        const RemoteDeviceInfo& rxDevice = devicePair.second;
        
        // Skip inactive devices and same device
        if (!rxDevice.isActive || rxDevice.deviceId == txDevice.deviceId)
        {
            continue;
        }
        
        // Check frequency compatibility (simplified)
        if (rxDevice.frequency != frequency)
        {
            continue;
        }
        
        // Check if in communication range
        if (IsInCommunicationRange(txDevice, rxDevice, txPowerDbm))
        {
            devicesInRange.push_back(rxDevice);
        }
    }
    
    return devicesInRange;
}

double
WifiChannelMpiProcessor::CalculateReceptionPower(const RemoteDeviceInfo& txDevice, 
                                                 const RemoteDeviceInfo& rxDevice, 
                                                 double txPowerDbm)
{
    NS_LOG_FUNCTION(this << &txDevice << &rxDevice << txPowerDbm);
    
    // Calculate distance
    double distance = CalculateDistance(txDevice.position, rxDevice.position);
    
    // Basic Friis formula for free space path loss
    // TODO: Use actual propagation models from channel proxy when available
    double frequency = txDevice.frequency; // Hz
    double lambda = 3e8 / frequency; // wavelength in meters
    double pathLossDb = 20 * log10(4 * M_PI * distance / lambda);
    
    // Include antenna gains
    double rxPowerDbm = txPowerDbm - pathLossDb + txDevice.antennaGainDb + rxDevice.antennaGainDb;
    
    return rxPowerDbm;
}

Time
WifiChannelMpiProcessor::CalculatePropagationDelay(const RemoteDeviceInfo& txDevice, 
                                                   const RemoteDeviceInfo& rxDevice)
{
    NS_LOG_FUNCTION(this << &txDevice << &rxDevice);
    
    // Calculate distance and propagation delay
    double distance = CalculateDistance(txDevice.position, rxDevice.position);
    double delaySeconds = distance / 3e8; // Speed of light
    
    return Seconds(delaySeconds);
}

void
WifiChannelMpiProcessor::SendReceptionNotification(const RemoteDeviceInfo& rxDevice, 
                                                   const ReceptionInfo& rxInfo)
{
    NS_LOG_FUNCTION(this << &rxDevice << &rxInfo);
    
    try
    {
        // Create reception notification message
        WifiMpiReceptionNotification notification;
        notification.SetReceiverId(rxInfo.receiverId);
        notification.SetTransmitterId(rxInfo.transmitterId);
        notification.SetRxPowerDbm(rxInfo.rxPowerDbm);
        notification.SetPropagationDelay(rxInfo.propagationDelay);
        notification.SetDuration(rxInfo.duration);
        notification.SetPacketSize(rxInfo.packetSize);
        
        // Send via WiFi MPI interface
        WifiMpi::SendReceptionNotification(rxDevice.rank, notification);
        
        m_sentNotifications++;
        
        LogActivity("SendReceptionNotification", 
                   "Sent to device " + std::to_string(rxInfo.receiverId) + 
                   " on rank " + std::to_string(rxDevice.rank) +
                   ", RX power: " + std::to_string(rxInfo.rxPowerDbm) + " dBm");
    }
    catch (const std::exception& e)
    {
        NS_LOG_ERROR("Error sending reception notification: " << e.what());
        LogActivity("SendReceptionNotification", "Error: " + std::string(e.what()));
    }
}

double
WifiChannelMpiProcessor::CalculateDistance(const Vector3D& pos1, const Vector3D& pos2) const
{
    double dx = pos1.x - pos2.x;
    double dy = pos1.y - pos2.y;
    double dz = pos1.z - pos2.z;
    return sqrt(dx*dx + dy*dy + dz*dz);
}

bool
WifiChannelMpiProcessor::IsInCommunicationRange(const RemoteDeviceInfo& txDevice, 
                                                const RemoteDeviceInfo& rxDevice, 
                                                double txPowerDbm) const
{
    NS_LOG_FUNCTION(this << &txDevice << &rxDevice << txPowerDbm);
    
    // Calculate estimated reception power
    double distance = CalculateDistance(txDevice.position, rxDevice.position);
    
    // Simple range check (could be made more sophisticated)
    // For now, use a fixed maximum range of 1000 meters
    const double maxRange = 1000.0; // meters
    
    return distance <= maxRange;
}

uint64_t
WifiChannelMpiProcessor::GenerateMessageId()
{
    return m_nextMessageId++;
}

void
WifiChannelMpiProcessor::LogActivity(const std::string& method, const std::string& details) const
{
    std::cout << "[WifiChannelMpiProcessor] " << method;
    if (!details.empty())
    {
        std::cout << " - " << details;
    }
    
    // Safe simulator time access
    if (Simulator::IsFinished() || !Simulator::IsExpired(Seconds(0)))
    {
        std::cout << " [SimTime: " << Simulator::Now().GetSeconds() << "s]";
    }
    else
    {
        std::cout << " [SimTime: not started]";
    }
    
    std::cout << std::endl;
}

#else // NS3_MPI not defined

// Stub implementation when MPI is not available

WifiChannelMpiProcessor::WifiChannelMpiProcessor()
{
    std::cout << "[DEBUG] STUB WifiChannelMpiProcessor constructor called" << std::endl;
    NS_LOG_FUNCTION(this);
}

WifiChannelMpiProcessor::~WifiChannelMpiProcessor()
{
    NS_LOG_FUNCTION(this);
}

bool
WifiChannelMpiProcessor::Initialize()
{
    return false;
}

void
WifiChannelMpiProcessor::StartProcessing()
{
}

void
WifiChannelMpiProcessor::StopProcessing()
{
}

bool
WifiChannelMpiProcessor::IsProcessing() const
{
    return false;
}

uint32_t
WifiChannelMpiProcessor::GetNumRegisteredDevices() const
{
    return 0;
}

uint64_t
WifiChannelMpiProcessor::GetTotalProcessedMessages() const
{
    return 0;
}

uint64_t
WifiChannelMpiProcessor::GetProcessedTransmissions() const
{
    return 0;
}

uint64_t
WifiChannelMpiProcessor::GetSentReceptionNotifications() const
{
    return 0;
}

double
WifiChannelMpiProcessor::GetReceptionThreshold() const
{
    return -85.0;
}

void
WifiChannelMpiProcessor::PrintStatistics() const
{
    std::cout << "[WifiChannelMpiProcessor] MPI not available - no statistics" << std::endl;
}

#endif // NS3_MPI

} // namespace ns3
