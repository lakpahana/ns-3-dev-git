/*
 * Copyright (c) 2025 ns-3 Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * WiFi Channel MPI Processor - Simplified Working Version
 * This demonstrates successful WiFi MPI integration with conditional compilation
 *
 * Author: AI Assistant
 * Date: January 2025
 */

#include "wifi-channel-mpi-processor.h"

#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

#ifdef NS3_MPI
#pragma message("SUCCESS: WiFi MPI Processor compiling with NS3_MPI defined!")
#include "wifi-mpi-message.h"

#include "ns3/mpi-interface.h"
#include "ns3/mpi-receiver.h"

#include <mpi.h>
#else
#pragma message("WiFi MPI Processor compiling in stub mode - MPI not available")
#endif

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiChannelMpiProcessor");

NS_OBJECT_ENSURE_REGISTERED(WifiChannelMpiProcessor);

#ifdef NS3_MPI

// ============================================================================
// RemoteDeviceInfo Implementation
// ============================================================================

RemoteDeviceInfo::RemoteDeviceInfo(uint32_t id, uint32_t r, const Vector3D& pos)
    : deviceId(id),
      rank(r),
      position(pos),
      lastActivity(Simulator::Now()),
      isActive(true)
{
}

// ============================================================================
// WifiChannelMpiProcessor Implementation (MPI Version)
// ============================================================================

TypeId
WifiChannelMpiProcessor::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WifiChannelMpiProcessor")
                            .SetParent<Object>()
                            .SetGroupName("Wifi")
                            .AddConstructor<WifiChannelMpiProcessor>();
    return tid;
}

WifiChannelMpiProcessor::WifiChannelMpiProcessor()
    : m_initialized(false),
      m_systemId(MpiInterface::GetSystemId()),
      m_systemCount(MpiInterface::GetSize()),
      m_deviceCounter(0),
      m_sequenceNumber(0)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("WifiChannelMpiProcessor created on rank " << m_systemId);
}

WifiChannelMpiProcessor::~WifiChannelMpiProcessor()
{
    NS_LOG_FUNCTION(this);
}

void
WifiChannelMpiProcessor::DoDispose()
{
    NS_LOG_FUNCTION(this);

    // Clear device registry
    m_remoteDevices.clear();

    // Clear cached models
    m_lossModels.clear();
    m_delayModels.clear();

    m_initialized = false;

    Object::DoDispose();
}

bool
WifiChannelMpiProcessor::Initialize()
{
    NS_LOG_FUNCTION(this);

    if (m_initialized)
    {
        NS_LOG_WARN("WifiChannelMpiProcessor already initialized");
        return true;
    }

    // Verify we're on rank 0 (channel rank)
    if (m_systemId != 0)
    {
        NS_LOG_ERROR("WifiChannelMpiProcessor must run on rank 0, current rank: " << m_systemId);
        return false;
    }

    NS_LOG_INFO("Initializing WiFi Channel MPI Processor on rank " << m_systemId << " of "
                                                                   << m_systemCount);

    // Set up MPI message reception for WiFi transmissions
    SetupMpiReception();

    m_initialized = true;
    LogActivity("INIT", "WiFi Channel MPI Processor initialized with message reception");

    return true;
}

uint32_t
WifiChannelMpiProcessor::RegisterDevice(uint32_t sourceRank, const Vector3D& position)
{
    NS_LOG_FUNCTION(this << sourceRank << position);

    uint32_t deviceId = ++m_deviceCounter;

    RemoteDeviceInfo deviceInfo(deviceId, sourceRank, position);
    m_remoteDevices[deviceId] = deviceInfo;

    NS_LOG_INFO("Registered device " << deviceId << " from rank " << sourceRank << " at position "
                                     << position);

    LogActivity("REGISTER",
                "Device " + std::to_string(deviceId) + " from rank " + std::to_string(sourceRank));

    return deviceId;
}

void
WifiChannelMpiProcessor::UnregisterDevice(uint32_t deviceId)
{
    NS_LOG_FUNCTION(this << deviceId);

    auto it = m_remoteDevices.find(deviceId);
    if (it != m_remoteDevices.end())
    {
        uint32_t rank = it->second.rank;
        m_remoteDevices.erase(it);

        NS_LOG_INFO("Unregistered device " << deviceId << " from rank " << rank);
        LogActivity("UNREGISTER", "Device " + std::to_string(deviceId));
    }
    else
    {
        NS_LOG_WARN("Attempted to unregister unknown device " << deviceId);
    }
}

void
WifiChannelMpiProcessor::UpdateDevicePosition(uint32_t deviceId, const Vector3D& newPosition)
{
    NS_LOG_FUNCTION(this << deviceId << newPosition);

    auto it = m_remoteDevices.find(deviceId);
    if (it != m_remoteDevices.end())
    {
        it->second.position = newPosition;
        it->second.lastActivity = Simulator::Now();

        NS_LOG_DEBUG("Updated position for device " << deviceId << " to " << newPosition);
    }
    else
    {
        NS_LOG_WARN("Attempted to update position for unknown device " << deviceId);
    }
}

void
WifiChannelMpiProcessor::ProcessTransmission(uint32_t transmitterId,
                                             const Vector3D& txPosition,
                                             double txPowerDbm,
                                             double frequency)
{
    NS_LOG_FUNCTION(this << transmitterId << txPosition << txPowerDbm << frequency);

    LogActivity("TX_PROCESS",
                "Processing transmission from device " + std::to_string(transmitterId));

    // Calculate reception for all other devices
    for (const auto& pair : m_remoteDevices)
    {
        uint32_t rxDeviceId = pair.first;
        const RemoteDeviceInfo& rxDevice = pair.second;

        // Skip self-transmission
        if (rxDeviceId == transmitterId)
        {
            continue;
        }

        // Calculate propagation effects
        double rxPowerDbm = CalculateRxPower(txPosition, rxDevice.position, txPowerDbm, frequency);
        double delaySeconds = CalculatePropagationDelay(txPosition, rxDevice.position);

        ReceptionInfo rxInfo;
        rxInfo.receiverId = rxDeviceId;
        rxInfo.transmitterId = transmitterId;
        rxInfo.rxPowerDbm = rxPowerDbm;
        rxInfo.txPowerDbm = txPowerDbm; // Add transmission power for path loss calculation
        rxInfo.delaySeconds = delaySeconds;
        rxInfo.frequency = frequency;

        // Send reception notification to device rank
        SendReceptionNotification(rxDevice, rxInfo);
    }
}

double
WifiChannelMpiProcessor::CalculateRxPower(const Vector3D& txPos,
                                          const Vector3D& rxPos,
                                          double txPowerDbm,
                                          double frequency) const
{
    // Simple free space path loss calculation
    double distance = CalculateDistance(txPos, rxPos);

    if (distance <= 0.0)
    {
        return txPowerDbm; // Same position
    }

    // Free space path loss in dB: 20*log10(4*pi*d*f/c)
    const double speedOfLight = 299792458.0; // m/s
    double pathLossDb = 20.0 * log10(4.0 * M_PI * distance * frequency / speedOfLight);

    double rxPowerDbm = txPowerDbm - pathLossDb;

    NS_LOG_DEBUG("Distance: " << distance << "m, Path Loss: " << pathLossDb
                              << "dB, RX Power: " << rxPowerDbm << "dBm");

    return rxPowerDbm;
}

double
WifiChannelMpiProcessor::CalculatePropagationDelay(const Vector3D& txPos,
                                                   const Vector3D& rxPos) const
{
    double distance = CalculateDistance(txPos, rxPos);
    const double speedOfLight = 299792458.0; // m/s

    return distance / speedOfLight;
}

double
WifiChannelMpiProcessor::CalculateDistance(const Vector3D& pos1, const Vector3D& pos2) const
{
    double dx = pos1.x - pos2.x;
    double dy = pos1.y - pos2.y;
    double dz = pos1.z - pos2.z;

    return sqrt(dx * dx + dy * dy + dz * dz);
}

void
WifiChannelMpiProcessor::SendReceptionNotification(const RemoteDeviceInfo& rxDevice,
                                                   const ReceptionInfo& rxInfo)
{
    NS_LOG_FUNCTION(this << rxDevice.deviceId << rxInfo.rxPowerDbm);

    try
    {
        // Phase 2.2.3c: Send real MPI RX notification message
        WifiMpiRxNotificationMessage rxNotificationMsg;

        // Set up message header
        rxNotificationMsg.header.messageType = WIFI_MPI_RX_NOTIFICATION;
        rxNotificationMsg.header.messageSize = sizeof(WifiMpiRxNotificationMessage);
        rxNotificationMsg.header.sequenceNumber = GetNextSequenceNumber();
        rxNotificationMsg.header.sourceRank = MpiInterface::GetSystemId();
        rxNotificationMsg.header.targetRank = rxDevice.rank;
        rxNotificationMsg.header.timestamp = Simulator::Now().GetNanoSeconds();
        rxNotificationMsg.header.checksum = 0; // Calculate if needed
        rxNotificationMsg.header.reserved = 0;

        // Set up RX notification details - Enhanced for Phase 2.2.3c
        rxNotificationMsg.receiverDeviceId = rxDevice.deviceId;
        rxNotificationMsg.transmitterDeviceId = rxInfo.transmitterId;
        rxNotificationMsg.targetPhyId = 0; // Default PHY for now
        rxNotificationMsg.rxPowerW =
            std::pow(10.0, (rxInfo.rxPowerDbm - 30.0) / 10.0); // dBm to Watts
        rxNotificationMsg.rxPowerDbm = rxInfo.rxPowerDbm;

        // Calculate additional propagation metrics
        Vector3D txPos = GetDevicePosition(rxInfo.transmitterId);
        double distance = CalculateDistance(txPos, rxDevice.position);
        double pathLossDb = rxInfo.txPowerDbm - rxInfo.rxPowerDbm; // Assuming txPowerDbm in rxInfo

        rxNotificationMsg.pathLossDb = pathLossDb;
        rxNotificationMsg.distanceM = distance;
        rxNotificationMsg.frequency = rxInfo.frequency;
        rxNotificationMsg.propagationDelay =
            static_cast<uint64_t>(rxInfo.delaySeconds * 1e9); // Convert to ns
        rxNotificationMsg.ppduSize = 0;                       // Simplified mode for Phase 2.2.3c
        rxNotificationMsg.transmissionTimestamp = Simulator::Now().GetNanoSeconds();

        // Create packet and send via MPI
        Ptr<Packet> packet = Create<Packet>(reinterpret_cast<const uint8_t*>(&rxNotificationMsg),
                                            sizeof(WifiMpiRxNotificationMessage));

        // Send MPI message to device rank
        Ptr<MpiReceiver> mpiReceiver =
            rxDevice.rank == MpiInterface::GetSystemId() ? nullptr : Create<MpiReceiver>();

        if (mpiReceiver && rxDevice.rank != MpiInterface::GetSystemId())
        {
            // Send to remote rank - provide current simulation time
            Time currentTime = Simulator::Now();
            MpiInterface::SendPacket(packet, currentTime, rxDevice.rank, 0);

            NS_LOG_INFO("Sent RX notification to device "
                        << rxDevice.deviceId << " on rank " << rxDevice.rank
                        << ": RX Power=" << rxInfo.rxPowerDbm << "dBm"
                        << ", Distance=" << distance << "m"
                        << ", Path Loss=" << pathLossDb << "dB");
        }
        else
        {
            NS_LOG_DEBUG("Skipping MPI send to same rank " << rxDevice.rank);
        }

        LogActivity("RX_NOTIFY_SENT",
                    "Device " + std::to_string(rxDevice.deviceId) +
                        " Rank=" + std::to_string(rxDevice.rank) +
                        " Power=" + std::to_string(rxInfo.rxPowerDbm) + "dBm" +
                        " Distance=" + std::to_string(distance) + "m");
    }
    catch (const std::exception& e)
    {
        NS_LOG_ERROR("Error sending RX notification to device " << rxDevice.deviceId << ": "
                                                                << e.what());
        LogActivity("RX_NOTIFY_ERROR",
                    "Failed to send to device " + std::to_string(rxDevice.deviceId) + ": " +
                        std::string(e.what()));
    }
}

void
WifiChannelMpiProcessor::LogActivity(const std::string& action, const std::string& details) const
{
    // Simple logging without complex time checks
    NS_LOG_INFO("WiFi Channel MPI [" << action << "] " << details << " at "
                                     << Simulator::Now().GetNanoSeconds() << "ns");
}

uint32_t
WifiChannelMpiProcessor::GetNextSequenceNumber()
{
    return ++m_sequenceNumber;
}

Vector3D
WifiChannelMpiProcessor::GetDevicePosition(uint32_t deviceId) const
{
    auto it = m_remoteDevices.find(deviceId);
    if (it != m_remoteDevices.end())
    {
        return it->second.position;
    }
    NS_LOG_WARN("Device " << deviceId << " not found, returning default position");
    return Vector3D(0.0, 0.0, 0.0);
}

std::vector<uint32_t>
WifiChannelMpiProcessor::GetRegisteredDevices() const
{
    std::vector<uint32_t> deviceIds;
    deviceIds.reserve(m_remoteDevices.size());

    for (const auto& pair : m_remoteDevices)
    {
        deviceIds.push_back(pair.first);
    }

    return deviceIds;
}

uint32_t
WifiChannelMpiProcessor::GetDeviceCount() const
{
    return m_remoteDevices.size();
}

bool
WifiChannelMpiProcessor::IsDeviceRegistered(uint32_t deviceId) const
{
    return m_remoteDevices.find(deviceId) != m_remoteDevices.end();
}

void
WifiChannelMpiProcessor::SetupMpiReception()
{
    NS_LOG_FUNCTION(this);

    if (!MpiInterface::IsEnabled())
    {
        NS_LOG_WARN("MPI not enabled - cannot set up message reception");
        return;
    }

    // In ns-3 MPI, message reception is handled automatically by the simulator
    // calling ReceiveMessages() periodically. We don't need to set up explicit
    // callbacks like in other MPI implementations.

    NS_LOG_INFO("MPI reception configured - using ns-3 polling pattern");
    LogActivity("MPI_SETUP", "Reception configured for WiFi channel processor");
}

void
WifiChannelMpiProcessor::HandleMpiMessage(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);

    if (!packet)
    {
        NS_LOG_WARN("Received null MPI packet");
        return;
    }

    NS_LOG_DEBUG("Received WiFi MPI packet of size: " << packet->GetSize());

    try
    {
        // Copy packet data to buffer for parsing (following ns-3 MPI patterns)
        uint32_t packetSize = packet->GetSize();
        uint8_t* buffer = new uint8_t[packetSize];
        packet->CopyData(buffer, packetSize);

        // Parse message header first (following ns-3 MPI approach)
        if (packetSize < sizeof(uint32_t))
        {
            NS_LOG_WARN("Packet too small for message header");
            delete[] buffer;
            return;
        }

        // Extract message type from header (first 4 bytes)
        uint32_t messageType = *reinterpret_cast<uint32_t*>(buffer);

        LogActivity("MPI_RECEIVE",
                    "Processing message type " + std::to_string(messageType) + " size " +
                        std::to_string(packetSize));

        // Route based on message type
        switch (static_cast<WifiMpiMessageType>(messageType))
        {
        case WIFI_MPI_DEVICE_REGISTER:
            HandleDeviceRegistrationMessage(packet);
            break;

        case WIFI_MPI_TX_REQUEST:
            HandleTransmissionRequestMessage(packet);
            break;

        default:
            NS_LOG_WARN("Unknown WiFi MPI message type: " << messageType);
            LogActivity("MPI_RECEIVE", "Unknown message type: " + std::to_string(messageType));
            break;
        }

        delete[] buffer;
    }
    catch (const std::exception& e)
    {
        NS_LOG_ERROR("Error parsing WiFi MPI message: " << e.what());
        LogActivity("MPI_RECEIVE", "Error parsing message: " + std::string(e.what()));
    }
}

void
WifiChannelMpiProcessor::ProcessDeviceRegistration(const WifiMpiDeviceRegisterMessage& message)
{
    NS_LOG_FUNCTION(this);

    // Simplified device registration for Phase 2.2.2
    // Full implementation will be added in Phase 2.2.3

    NS_LOG_INFO("Device registration received - implementation pending Phase 2.2.3");
    LogActivity("DEVICE_REG", "Registration message received");
}

void
WifiChannelMpiProcessor::ProcessTransmissionRequest(const WifiMpiTxRequestMessage& message)
{
    NS_LOG_FUNCTION(this);

    // Simplified transmission processing for Phase 2.2.2
    // Full implementation will be added in Phase 2.2.3

    NS_LOG_INFO("Transmission request received - implementation pending Phase 2.2.3");
    LogActivity("TX_REQUEST", "Transmission message received");
}

void
WifiChannelMpiProcessor::HandleDeviceRegistrationMessage(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);

    try
    {
        // Parse device registration message using proper packet access
        uint32_t packetSize = packet->GetSize();
        uint8_t* buffer = new uint8_t[packetSize];
        packet->CopyData(buffer, packetSize);

        // Parse message header
        WifiMpiMessageHeader* header = reinterpret_cast<WifiMpiMessageHeader*>(buffer);

        // Parse device registration message body
        if (packetSize < sizeof(WifiMpiMessageHeader) + sizeof(WifiMpiDeviceRegisterMessage) -
                             sizeof(WifiMpiMessageHeader))
        {
            NS_LOG_WARN("Packet too small for device registration message");
            delete[] buffer;
            return;
        }

        WifiMpiDeviceRegisterMessage* regMsg =
            reinterpret_cast<WifiMpiDeviceRegisterMessage*>(buffer);

        uint32_t sourceRank = header->sourceRank;
        uint32_t deviceId = regMsg->deviceId;
        uint32_t nodeId = regMsg->nodeId;

        NS_LOG_INFO("Processing device registration from rank "
                    << sourceRank << " deviceId " << deviceId << " nodeId " << nodeId);

        // For now, register with default position - can be enhanced later to include position data
        Vector3D position(0.0, 0.0, 0.0);
        uint32_t assignedDeviceId = RegisterDevice(sourceRank, position);

        LogActivity("DEVICE_REG_PROCESSED",
                    "Registered device " + std::to_string(assignedDeviceId) + " from rank " +
                        std::to_string(sourceRank) + " original deviceId " +
                        std::to_string(deviceId) + " nodeId " + std::to_string(nodeId));

        NS_LOG_INFO("Successfully registered device " << assignedDeviceId << " from rank "
                                                      << sourceRank);

        delete[] buffer;

        // TODO: Send registration acknowledgment in Phase 2.2.3c
    }
    catch (const std::exception& e)
    {
        NS_LOG_ERROR("Error processing device registration message: " << e.what());
        LogActivity("DEVICE_REG_ERROR", "Failed to process registration: " + std::string(e.what()));
    }
}

void
WifiChannelMpiProcessor::HandleTransmissionRequestMessage(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);

    try
    {
        // Parse transmission request message using proper packet access
        uint32_t packetSize = packet->GetSize();
        uint8_t* buffer = new uint8_t[packetSize];
        packet->CopyData(buffer, packetSize);

        // Parse message header
        WifiMpiMessageHeader* header = reinterpret_cast<WifiMpiMessageHeader*>(buffer);

        // Parse transmission request message body
        if (packetSize < sizeof(WifiMpiMessageHeader) + sizeof(WifiMpiTxRequestMessage) -
                             sizeof(WifiMpiMessageHeader))
        {
            NS_LOG_WARN("Packet too small for transmission request message");
            delete[] buffer;
            return;
        }

        WifiMpiTxRequestMessage* txMsg = reinterpret_cast<WifiMpiTxRequestMessage*>(buffer);

        // Extract transmission parameters
        uint32_t sourceRank = header->sourceRank;
        uint32_t deviceId = txMsg->deviceId;
        double txPowerW = txMsg->txPowerW;
        double txPowerDbm = 10.0 * log10(txPowerW * 1000.0); // Convert W to dBm

        NS_LOG_INFO("Processing transmission request from rank "
                    << sourceRank << " deviceId " << deviceId << " power " << txPowerDbm << " dBm");

        LogActivity("TX_REQ_PROCESSING",
                    "Processing transmission from device " + std::to_string(deviceId) + " rank " +
                        std::to_string(sourceRank) + " power " + std::to_string(txPowerDbm) +
                        " dBm");

        // Check if transmitting device is registered
        if (IsDeviceRegistered(deviceId))
        {
            // Get device info for position
            auto it = m_remoteDevices.find(deviceId);
            if (it != m_remoteDevices.end())
            {
                const RemoteDeviceInfo& txDevice = it->second;

                // Call existing ProcessTransmission method for propagation calculation
                // Use 2.4 GHz as default frequency for now
                double frequency = 2.4e9; // 2.4 GHz
                ProcessTransmission(deviceId, txDevice.position, txPowerDbm, frequency);

                LogActivity("TX_REQ_PROCESSED",
                            "Processed transmission from device " + std::to_string(deviceId) +
                                " at position (" + std::to_string(txDevice.position.x) + "," +
                                std::to_string(txDevice.position.y) + "," +
                                std::to_string(txDevice.position.z) + ")");

                NS_LOG_INFO("Successfully processed transmission from device " << deviceId);
            }
            else
            {
                NS_LOG_WARN("Device " << deviceId << " registered but not found in device map");
            }
        }
        else
        {
            NS_LOG_WARN("Transmission request from unregistered device " << deviceId);
            LogActivity("TX_REQ_ERROR",
                        "Transmission from unregistered device " + std::to_string(deviceId));
        }

        delete[] buffer;
    }
    catch (const std::exception& e)
    {
        NS_LOG_ERROR("Error processing transmission request message: " << e.what());
        LogActivity("TX_REQ_ERROR", "Failed to process transmission: " + std::string(e.what()));
    }
}

#else

// ============================================================================
// WifiChannelMpiProcessor Implementation (Stub Version)
// ============================================================================

TypeId
WifiChannelMpiProcessor::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WifiChannelMpiProcessor")
                            .SetParent<Object>()
                            .SetGroupName("Wifi")
                            .AddConstructor<WifiChannelMpiProcessor>();
    return tid;
}

WifiChannelMpiProcessor::WifiChannelMpiProcessor()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("WifiChannelMpiProcessor created in stub mode (MPI not available)");
}

WifiChannelMpiProcessor::~WifiChannelMpiProcessor()
{
    NS_LOG_FUNCTION(this);
}

void
WifiChannelMpiProcessor::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Object::DoDispose();
}

bool
WifiChannelMpiProcessor::Initialize()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("WifiChannelMpiProcessor stub initialization - no MPI operations");
    return true;
}

uint32_t
WifiChannelMpiProcessor::RegisterDevice(uint32_t, const Vector3D&)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Stub: Device registration - MPI not available");
    return 0;
}

void
WifiChannelMpiProcessor::UnregisterDevice(uint32_t)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Stub: Device unregistration - MPI not available");
}

void
WifiChannelMpiProcessor::UpdateDevicePosition(uint32_t, const Vector3D&)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("Stub: Position update - MPI not available");
}

void
WifiChannelMpiProcessor::ProcessTransmission(uint32_t, const Vector3D&, double, double)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("Stub: Transmission processing - MPI not available");
}

std::vector<uint32_t>
WifiChannelMpiProcessor::GetRegisteredDevices() const
{
    return std::vector<uint32_t>();
}

uint32_t
WifiChannelMpiProcessor::GetDeviceCount() const
{
    return 0;
}

bool
WifiChannelMpiProcessor::IsDeviceRegistered(uint32_t) const
{
    return false;
}

#endif // NS3_MPI

} // namespace ns3
