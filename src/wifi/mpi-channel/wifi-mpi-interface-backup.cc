#include "wifi-mpi-interface.h"

#ifdef NS3_MPI

#include "ns3/assert.h"
#include "ns3/buffer.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/wifi-ppdu.h"
#include "ns3/wifi-tx-vector.h"
#include "ns3/wifi-utils.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiMpiInterface");

NS_OBJECT_ENSURE_REGISTERED(WifiMpiInterface);

// Global instance management
Ptr<WifiMpiInterface> WifiMpi::s_instance = nullptr;

// ===== WifiMpiInterface Implementation =====

TypeId
WifiMpiInterface::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WifiMpiInterface")
                            .SetParent<Object>()
                            .SetGroupName("Wifi")
                            .AddConstructor<WifiMpiInterface>();
    return tid;
}

WifiMpiInterface::WifiMpiInterface()
    : m_initialized(false),
      m_rank(0),
      m_size(1),
      m_verboseLogging(false),
      m_messagesSent(0),
      m_messagesReceived(0),
      m_bytesTransferred(0),
      m_nextSequenceNumber(1)
{
    NS_LOG_FUNCTION(this);
}

WifiMpiInterface::~WifiMpiInterface()
{
    NS_LOG_FUNCTION(this);
    if (m_initialized)
    {
        Shutdown();
    }
}

bool
WifiMpiInterface::Initialize(uint32_t rank, uint32_t size)
{
    NS_LOG_FUNCTION(this << rank << size);

    if (m_initialized)
    {
        NS_LOG_WARN("WiFi MPI interface already initialized");
        return true;
    }

    // Verify that regular ns-3 MPI is enabled
    if (!MpiInterface::IsEnabled())
    {
        NS_LOG_ERROR("Regular ns-3 MPI interface must be enabled before WiFi MPI");
        return false;
    }

    // Verify rank and size consistency
    if (rank != MpiInterface::GetSystemId() || size != MpiInterface::GetSize())
    {
        NS_LOG_ERROR("Rank/size mismatch with regular MPI interface");
        return false;
    }

    m_rank = rank;
    m_size = size;
    m_initialized = true;

    NS_LOG_INFO("WiFi MPI interface initialized - Rank: "
                << m_rank << ", Size: " << m_size
                << ", Type: " << (IsChannelRank() ? "CHANNEL" : "DEVICE"));

    return true;
}

void
WifiMpiInterface::Shutdown()
{
    NS_LOG_FUNCTION(this);

    if (!m_initialized)
    {
        return;
    }

    // Process any remaining messages
    ProcessIncomingMessages();

    // Log final statistics
    NS_LOG_INFO("WiFi MPI interface shutting down - Messages sent: "
                << m_messagesSent << ", received: " << m_messagesReceived
                << ", bytes transferred: " << m_bytesTransferred);

    m_initialized = false;
    m_messageHandlers.clear();

    // Clear pending message queue
    while (!m_pendingMessages.empty())
    {
        m_pendingMessages.pop();
    }
}

bool
WifiMpiInterface::IsInitialized() const
{
    return m_initialized;
}

uint32_t
WifiMpiInterface::GetRank() const
{
    return m_rank;
}

uint32_t
WifiMpiInterface::GetSize() const
{
    return m_size;
}

bool
WifiMpiInterface::IsChannelRank() const
{
    return m_rank == 0;
}

bool
WifiMpiInterface::IsDeviceRank() const
{
    return m_rank > 0;
}

void
WifiMpiInterface::RegisterMessageHandler(WifiMpiMessageType messageType, MessageHandler handler)
{
    NS_LOG_FUNCTION(this << static_cast<uint32_t>(messageType));

    NS_ASSERT(m_initialized);
    m_messageHandlers[messageType] = handler;

    NS_LOG_DEBUG("Registered handler for message type: "
                 << WifiMpiMessageUtils::GetMessageTypeName(messageType));
}

void
WifiMpiInterface::SendDeviceRegister(uint32_t deviceId,
                                     uint32_t phyId,
                                     uint32_t phyType,
                                     uint32_t channelNumber,
                                     uint32_t channelWidth)
{
    NS_LOG_FUNCTION(this << deviceId << phyId << phyType << channelNumber << channelWidth);

    NS_ASSERT(m_initialized);
    NS_ASSERT(IsDeviceRank()); // Only device ranks send registration messages

    // Create message
    WifiMpiDeviceRegisterMessage msg;
    msg.header = CreateMessageHeader(WIFI_MPI_DEVICE_REGISTER,
                                     WifiMpiDeviceRegisterMessage::GetSerializedSize(),
                                     0, // target channel rank
                                     deviceId);
    msg.phyId = phyId;
    msg.phyType = phyType;
    msg.channelNumber = channelNumber;
    msg.channelWidth = channelWidth;

    // Serialize message
    Ptr<Packet> packet = Create<Packet>(WifiMpiDeviceRegisterMessage::GetSerializedSize());
    Buffer buffer;
    buffer.AddAtStart(WifiMpiDeviceRegisterMessage::GetSerializedSize());
    Buffer::Iterator iter = buffer.Begin();
    msg.Serialize(iter);

    packet->AddAtStart(buffer);

    // Send to channel rank (rank 0)
    SendMessage(0, WIFI_MPI_DEVICE_REGISTER, packet, deviceId);
}

void
WifiMpiInterface::SendConfigMessage(uint32_t deviceId,
                                    uint32_t configType,
                                    uint32_t modelType,
                                    Ptr<Packet> modelData)
{
    NS_LOG_FUNCTION(this << deviceId << configType << modelType);

    NS_ASSERT(m_initialized);
    NS_ASSERT(IsDeviceRank()); // Only device ranks send config messages

    uint32_t modelDataSize = modelData ? modelData->GetSize() : 0;
    uint32_t totalSize = WifiMpiConfigMessage::GetSerializedSize() + modelDataSize;

    // Create message
    WifiMpiConfigMessage msg;
    WifiMpiMessageType msgType =
        (configType == 0) ? WIFI_MPI_CONFIG_DELAY_MODEL : WIFI_MPI_CONFIG_LOSS_MODEL;
    msg.header = CreateMessageHeader(msgType, totalSize, 0, deviceId);
    msg.configType = configType;
    msg.modelType = modelType;
    msg.serializedSize = modelDataSize;

    // Serialize message header
    Buffer buffer;
    buffer.AddAtStart(totalSize);
    Buffer::Iterator iter = buffer.Begin();
    msg.Serialize(iter);

    // Add model data if present
    if (modelData && modelDataSize > 0)
    {
        modelData->CopyData(&iter, modelDataSize);
    }

    Ptr<Packet> packet = Create<Packet>();
    packet->AddAtStart(buffer);

    // Send to channel rank (rank 0)
    SendMessage(0, msgType, packet, deviceId);
}

void
WifiMpiInterface::SendTxRequest(uint32_t deviceId,
                                uint32_t phyId,
                                double txPowerW,
                                Ptr<const WifiPpdu> ppdu,
                                const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << deviceId << phyId << txPowerW);

    NS_ASSERT(m_initialized);
    NS_ASSERT(IsDeviceRank()); // Only device ranks send TX requests

    // Serialize PPDU and TxVector
    uint32_t ppduSize = ppdu ? ppdu->GetSerializedSize() : 0;
    uint32_t txVectorSize = 0; // TODO: Implement TxVector serialization
    uint32_t totalSize = WifiMpiTxRequestMessage::GetSerializedSize() + ppduSize + txVectorSize;

    // Create message
    WifiMpiTxRequestMessage msg;
    msg.header = CreateMessageHeader(WIFI_MPI_TX_REQUEST, totalSize, 0, deviceId);
    msg.senderPhyId = phyId;
    msg.txPowerW = txPowerW;
    msg.packetSize = ppdu ? ppdu->GetSize() : 0;
    msg.ppduSerializedSize = ppduSize;
    msg.txVectorSerializedSize = txVectorSize;

    // Serialize message
    Buffer buffer;
    buffer.AddAtStart(totalSize);
    Buffer::Iterator iter = buffer.Begin();
    msg.Serialize(iter);

    // Add PPDU data
    if (ppdu && ppduSize > 0)
    {
        Buffer ppduBuffer;
        ppduBuffer.AddAtStart(ppduSize);
        Buffer::Iterator ppduIter = ppduBuffer.Begin();
        ppdu->Serialize(ppduIter);

        ppduIter = ppduBuffer.Begin();
        for (uint32_t i = 0; i < ppduSize; ++i)
        {
            iter.WriteU8(ppduIter.ReadU8());
        }
    }

    // TODO: Add TxVector serialization when available

    Ptr<Packet> packet = Create<Packet>();
    packet->AddAtStart(buffer);

    // Send to channel rank (rank 0)
    SendMessage(0, WIFI_MPI_TX_REQUEST, packet, deviceId);
}

void
WifiMpiInterface::SendRxNotification(uint32_t targetRank,
                                     uint32_t deviceId,
                                     uint32_t phyId,
                                     double rxPowerW,
                                     Time rxDuration,
                                     Ptr<const WifiPpdu> ppdu,
                                     Ptr<Packet> signalInfo)
{
    NS_LOG_FUNCTION(this << targetRank << deviceId << phyId << rxPowerW << rxDuration);

    NS_ASSERT(m_initialized);
    NS_ASSERT(IsChannelRank()); // Only channel rank sends RX notifications

    // Calculate sizes
    uint32_t ppduSize = ppdu ? ppdu->GetSerializedSize() : 0;
    uint32_t signalInfoSize = signalInfo ? signalInfo->GetSize() : 0;
    uint32_t totalSize =
        WifiMpiRxNotificationMessage::GetSerializedSize() + ppduSize + signalInfoSize;

    // Create message
    WifiMpiRxNotificationMessage msg;
    msg.header = CreateMessageHeader(WIFI_MPI_RX_NOTIFICATION, totalSize, targetRank, deviceId);
    msg.receiverPhyId = phyId;
    msg.rxPowerW = rxPowerW;
    msg.rxDuration = rxDuration.GetTimeStep();
    msg.packetSize = ppdu ? ppdu->GetSize() : 0;
    msg.ppduSerializedSize = ppduSize;
    msg.rxSignalInfoSize = signalInfoSize;

    // Serialize message
    Buffer buffer;
    buffer.AddAtStart(totalSize);
    Buffer::Iterator iter = buffer.Begin();
    msg.Serialize(iter);

    // Add PPDU data
    if (ppdu && ppduSize > 0)
    {
        Buffer ppduBuffer;
        ppduBuffer.AddAtStart(ppduSize);
        Buffer::Iterator ppduIter = ppduBuffer.Begin();
        ppdu->Serialize(ppduIter);

        ppduIter = ppduBuffer.Begin();
        for (uint32_t i = 0; i < ppduSize; ++i)
        {
            iter.WriteU8(ppduIter.ReadU8());
        }
    }

    // Add signal info data
    if (signalInfo && signalInfoSize > 0)
    {
        signalInfo->CopyData(&iter, signalInfoSize);
    }

    Ptr<Packet> packet = Create<Packet>();
    packet->AddAtStart(buffer);

    // Send to target device rank
    SendMessage(targetRank, WIFI_MPI_RX_NOTIFICATION, packet, deviceId);
}

void
WifiMpiInterface::ProcessIncomingMessages()
{
    // This is a placeholder for message processing
    // In the real implementation, this would check for incoming MPI messages
    // and add them to the pending queue for processing

    // Process pending messages
    while (!m_pendingMessages.empty())
    {
        PendingMessage pending = m_pendingMessages.front();
        m_pendingMessages.pop();

        HandleReceivedMessage(pending.buffer, pending.sourceRank);
    }
}

void
WifiMpiInterface::GetStatistics(uint32_t& sentCount,
                                uint32_t& receivedCount,
                                uint64_t& bytesTransferred) const
{
    sentCount = m_messagesSent;
    receivedCount = m_messagesReceived;
    bytesTransferred = m_bytesTransferred;
}

void
WifiMpiInterface::SetVerboseLogging(bool enable)
{
    m_verboseLogging = enable;
    NS_LOG_INFO("Verbose logging " << (enable ? "enabled" : "disabled"));
}

// ===== Private Methods =====

void
WifiMpiInterface::SendMessage(uint32_t targetRank,
                              WifiMpiMessageType messageType,
                              Ptr<Packet> messageData,
                              uint32_t deviceId)
{
    NS_LOG_FUNCTION(this << targetRank << static_cast<uint32_t>(messageType) << deviceId);

    NS_ASSERT(m_initialized);
    NS_ASSERT(messageData);

    uint32_t messageSize = messageData->GetSize();

    // Use existing ns-3 MPI infrastructure to send the packet
    // The MpiInterface::SendPacket method handles the actual MPI communication
    Time rxTime = Simulator::Now(); // Immediate delivery for synchronous messaging

    // For now, we'll use the node ID as the device identifier
    // In a full implementation, we'd need to map device IDs to node IDs
    MpiInterface::SendPacket(messageData, rxTime, targetRank, deviceId);

    // Update statistics
    m_messagesSent++;
    m_bytesTransferred += messageSize;

    // Log if verbose logging is enabled
    if (m_verboseLogging)
    {
        LogMessageActivity("SEND", messageType, m_rank, targetRank, messageSize);
    }

    NS_LOG_DEBUG("Sent message type " << WifiMpiMessageUtils::GetMessageTypeName(messageType)
                                      << " to rank " << targetRank << ", size: " << messageSize
                                      << " bytes");
}

WifiMpiMessageHeader
WifiMpiInterface::CreateMessageHeader(WifiMpiMessageType messageType,
                                      uint32_t messageSize,
                                      uint32_t targetRank,
                                      uint32_t deviceId)
{
    WifiMpiMessageHeader header;
    header.messageType = static_cast<uint32_t>(messageType);
    header.messageSize = messageSize;
    header.sourceRank = m_rank;
    header.targetRank = targetRank;
    header.timestamp = Simulator::Now().GetTimeStep();
    header.sequenceNumber = m_nextSequenceNumber++;
    header.deviceId = deviceId;
    header.reserved = 0;

    return header;
}

void
WifiMpiInterface::HandleReceivedMessage(Ptr<Packet> buffer, uint32_t sourceRank)
{
    NS_LOG_FUNCTION(this << sourceRank);

    NS_ASSERT(buffer);

    // Extract and validate header
    Buffer::Iterator iter = buffer->Begin();
    WifiMpiMessageHeader header;
    header.Deserialize(iter);

    if (!ValidateIncomingMessage(header, sourceRank))
    {
        NS_LOG_ERROR("Invalid message received from rank " << sourceRank);
        return;
    }

    WifiMpiMessageType messageType = static_cast<WifiMpiMessageType>(header.messageType);

    // Update statistics
    m_messagesReceived++;
    m_bytesTransferred += header.messageSize;

    // Log if verbose logging is enabled
    if (m_verboseLogging)
    {
        LogMessageActivity("RECV", messageType, sourceRank, m_rank, header.messageSize);
    }

    // Dispatch to registered handler
    auto handlerIter = m_messageHandlers.find(messageType);
    if (handlerIter != m_messageHandlers.end())
    {
        handlerIter->second(buffer, messageType, sourceRank);
    }
    else
    {
        NS_LOG_WARN("No handler registered for message type: "
                    << WifiMpiMessageUtils::GetMessageTypeName(messageType));
    }
}

bool
WifiMpiInterface::ValidateIncomingMessage(const WifiMpiMessageHeader& header, uint32_t sourceRank)
{
    return WifiMpiMessageUtils::ValidateHeader(header) && header.sourceRank == sourceRank;
}

void
WifiMpiInterface::LogMessageActivity(const std::string& direction,
                                     WifiMpiMessageType messageType,
                                     uint32_t sourceRank,
                                     uint32_t targetRank,
                                     uint32_t messageSize)
{
    NS_LOG_INFO("[WIFI_MPI_" << direction << "] "
                             << WifiMpiMessageUtils::GetMessageTypeName(messageType) << " - "
                             << sourceRank << " -> " << targetRank << " (" << messageSize
                             << " bytes)"
                             << " [SimTime: " << Simulator::Now().GetSeconds() << "s]");
}

// ===== WifiMpi Singleton Implementation =====

Ptr<WifiMpiInterface>
WifiMpi::GetInstance()
{
    if (!s_instance)
    {
        s_instance = Create<WifiMpiInterface>();
    }
    return s_instance;
}

bool
WifiMpi::Initialize(uint32_t rank, uint32_t size)
{
    Ptr<WifiMpiInterface> instance = GetInstance();
    return instance->Initialize(rank, size);
}

void
WifiMpi::Shutdown()
{
    if (s_instance)
    {
        s_instance->Shutdown();
        s_instance = nullptr;
    }
}

bool
WifiMpi::IsEnabled()
{
    return s_instance && s_instance->IsInitialized();
}

void
WifiMpi::SendDeviceRegistration(uint32_t targetRank, uint32_t deviceId, uint32_t sourceRank)
{
    NS_LOG_FUNCTION(targetRank << deviceId << sourceRank);
    
    if (Ptr<WifiMpiInterface> instance = GetInstance())
    {
        // For device registration, we need PHY info that's not provided in the simplified interface
        // Using default values for the missing parameters
        instance->SendDeviceRegister(deviceId, deviceId, 0, 1, 20);
    }
}

void
WifiMpi::SendTransmissionRequest(uint32_t targetRank, uint32_t deviceId, 
                               Ptr<const WifiPpdu> ppdu, dBm_u txPower)
{
    NS_LOG_FUNCTION(targetRank << deviceId << txPower);
    
    if (Ptr<WifiMpiInterface> instance = GetInstance())
    {
        // Convert dBm to watts
        double txPowerW = DbmToW(txPower);
        
        // For TX request, we need additional info that's not provided
        // Using deviceId as PHY ID and creating a default TxVector
        WifiTxVector txVector; // Default TxVector
        instance->SendTxRequest(deviceId, deviceId, txPowerW, ppdu, txVector);
    }
}

void
WifiMpi::SendLossModelConfig(uint32_t targetRank, Ptr<PropagationLossModel> lossModel)
{
    NS_LOG_FUNCTION(targetRank);
    
    if (Ptr<WifiMpiInterface> instance = GetInstance())
    {
        // For loss model config, we need to serialize the model
        // For now, sending minimal config info
        instance->SendConfigMessage(0, 1, 0, nullptr); // configType=1 for loss model
    }
}

void
WifiMpi::SendDelayModelConfig(uint32_t targetRank, Ptr<PropagationDelayModel> delayModel)
{
    NS_LOG_FUNCTION(targetRank);
    
    if (Ptr<WifiMpiInterface> instance = GetInstance())
    {
        // For delay model config, we need to serialize the model
        // For now, sending minimal config info
        instance->SendConfigMessage(0, 0, 0, nullptr); // configType=0 for delay model
    }
}

} // namespace ns3

#endif // NS3_MPI
