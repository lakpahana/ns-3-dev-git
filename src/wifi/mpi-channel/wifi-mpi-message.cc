#include "wifi-mpi-message.h"

#ifdef NS3_MPI

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/node-list.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiMpiMessage");

uint32_t WifiMpiMessageUtils::s_sequenceNumber = 0;

// ===== WifiMpiMessageHeader Implementation =====

void
WifiMpiMessageHeader::Serialize(Buffer::Iterator& buffer) const
{
    buffer.WriteHtonU32(messageType);
    buffer.WriteHtonU32(messageSize);
    buffer.WriteHtonU32(sourceRank);
    buffer.WriteHtonU32(targetRank);
    buffer.WriteHtonU64(timestamp);
    buffer.WriteHtonU32(sequenceNumber);
    buffer.WriteHtonU32(deviceId);
    buffer.WriteHtonU32(reserved);
}

void
WifiMpiMessageHeader::Deserialize(Buffer::Iterator& buffer)
{
    messageType = buffer.ReadNtohU32();
    messageSize = buffer.ReadNtohU32();
    sourceRank = buffer.ReadNtohU32();
    targetRank = buffer.ReadNtohU32();
    timestamp = buffer.ReadNtohU64();
    sequenceNumber = buffer.ReadNtohU32();
    deviceId = buffer.ReadNtohU32();
    reserved = buffer.ReadNtohU32();
}

uint32_t
WifiMpiMessageHeader::GetSerializedSize()
{
    return 8 * sizeof(uint32_t) + sizeof(uint64_t); // 9 * 4 + 8 = 44 bytes
}

// ===== WifiMpiDeviceRegisterMessage Implementation =====

void
WifiMpiDeviceRegisterMessage::Serialize(Buffer::Iterator& buffer) const
{
    header.Serialize(buffer);
    buffer.WriteHtonU32(phyId);
    buffer.WriteHtonU32(phyType);
    buffer.WriteHtonU32(channelNumber);
    buffer.WriteHtonU32(channelWidth);
}

void
WifiMpiDeviceRegisterMessage::Deserialize(Buffer::Iterator& buffer)
{
    header.Deserialize(buffer);
    phyId = buffer.ReadNtohU32();
    phyType = buffer.ReadNtohU32();
    channelNumber = buffer.ReadNtohU32();
    channelWidth = buffer.ReadNtohU32();
}

uint32_t
WifiMpiDeviceRegisterMessage::GetSerializedSize()
{
    return WifiMpiMessageHeader::GetSerializedSize() + 4 * sizeof(uint32_t);
}

// ===== WifiMpiConfigMessage Implementation =====

void
WifiMpiConfigMessage::Serialize(Buffer::Iterator& buffer) const
{
    header.Serialize(buffer);
    buffer.WriteHtonU32(configType);
    buffer.WriteHtonU32(modelType);
    buffer.WriteHtonU32(serializedSize);
}

void
WifiMpiConfigMessage::Deserialize(Buffer::Iterator& buffer)
{
    header.Deserialize(buffer);
    configType = buffer.ReadNtohU32();
    modelType = buffer.ReadNtohU32();
    serializedSize = buffer.ReadNtohU32();
}

uint32_t
WifiMpiConfigMessage::GetSerializedSize()
{
    return WifiMpiMessageHeader::GetSerializedSize() + 3 * sizeof(uint32_t);
}

// ===== WifiMpiTxRequestMessage Implementation =====

void
WifiMpiTxRequestMessage::Serialize(Buffer::Iterator& buffer) const
{
    header.Serialize(buffer);
    buffer.WriteHtonU32(senderPhyId);
    buffer.WriteHtonU64(static_cast<uint64_t>(txPowerW * 1e12)); // Convert to pW for precision
    buffer.WriteHtonU32(packetSize);
    buffer.WriteHtonU32(ppduSerializedSize);
    buffer.WriteHtonU32(txVectorSerializedSize);
}

void
WifiMpiTxRequestMessage::Deserialize(Buffer::Iterator& buffer)
{
    header.Deserialize(buffer);
    senderPhyId = buffer.ReadNtohU32();
    uint64_t powerPW = buffer.ReadNtohU64();
    txPowerW = static_cast<double>(powerPW) / 1e12; // Convert back from pW
    packetSize = buffer.ReadNtohU32();
    ppduSerializedSize = buffer.ReadNtohU32();
    txVectorSerializedSize = buffer.ReadNtohU32();
}

uint32_t
WifiMpiTxRequestMessage::GetSerializedSize()
{
    return WifiMpiMessageHeader::GetSerializedSize() + 4 * sizeof(uint32_t) + sizeof(uint64_t);
}

// ===== WifiMpiRxNotificationMessage Implementation =====

void
WifiMpiRxNotificationMessage::Serialize(Buffer::Iterator& buffer) const
{
    header.Serialize(buffer);
    buffer.WriteHtonU32(receiverPhyId);
    buffer.WriteHtonU64(static_cast<uint64_t>(rxPowerW * 1e12)); // Convert to pW for precision
    buffer.WriteHtonU64(rxDuration);
    buffer.WriteHtonU32(packetSize);
    buffer.WriteHtonU32(ppduSerializedSize);
    buffer.WriteHtonU32(rxSignalInfoSize);
}

void
WifiMpiRxNotificationMessage::Deserialize(Buffer::Iterator& buffer)
{
    header.Deserialize(buffer);
    receiverPhyId = buffer.ReadNtohU32();
    uint64_t powerPW = buffer.ReadNtohU64();
    rxPowerW = static_cast<double>(powerPW) / 1e12; // Convert back from pW
    rxDuration = buffer.ReadNtohU64();
    packetSize = buffer.ReadNtohU32();
    ppduSerializedSize = buffer.ReadNtohU32();
    rxSignalInfoSize = buffer.ReadNtohU32();
}

uint32_t
WifiMpiRxNotificationMessage::GetSerializedSize()
{
    return WifiMpiMessageHeader::GetSerializedSize() + 4 * sizeof(uint32_t) + 2 * sizeof(uint64_t);
}

// ===== WifiMpiMessageUtils Implementation =====

Ptr<Packet>
WifiMpiMessageUtils::CreateMessageBuffer(WifiMpiMessageType messageType, uint32_t extraSize)
{
    uint32_t baseSize = 0;

    switch (messageType)
    {
    case WIFI_MPI_DEVICE_REGISTER:
        baseSize = WifiMpiDeviceRegisterMessage::GetSerializedSize();
        break;
    case WIFI_MPI_CONFIG_DELAY_MODEL:
    case WIFI_MPI_CONFIG_LOSS_MODEL:
        baseSize = WifiMpiConfigMessage::GetSerializedSize();
        break;
    case WIFI_MPI_TX_REQUEST:
        baseSize = WifiMpiTxRequestMessage::GetSerializedSize();
        break;
    case WIFI_MPI_RX_NOTIFICATION:
        baseSize = WifiMpiRxNotificationMessage::GetSerializedSize();
        break;
    default:
        baseSize = WifiMpiMessageHeader::GetSerializedSize();
        break;
    }

    uint32_t totalSize = baseSize + extraSize;
    Ptr<Packet> packet = Create<Packet>(totalSize);

    NS_LOG_DEBUG("Created message buffer for type "
                 << GetMessageTypeName(messageType) << ", base size: " << baseSize
                 << ", extra size: " << extraSize << ", total size: " << totalSize);

    return packet;
}

bool
WifiMpiMessageUtils::ValidateHeader(const WifiMpiMessageHeader& header)
{
    // Validate message type
    if (header.messageType < WIFI_MPI_DEVICE_REGISTER || header.messageType > WIFI_MPI_ERROR_NOTIFY)
    {
        NS_LOG_ERROR("Invalid message type: " << header.messageType);
        return false;
    }

    // Validate message size (should be reasonable)
    if (header.messageSize < WifiMpiMessageHeader::GetSerializedSize() ||
        header.messageSize > 1000000) // 1MB max
    {
        NS_LOG_ERROR("Invalid message size: " << header.messageSize);
        return false;
    }

    // Validate timestamp (should not be in the future relative to current sim time)
    Time currentTime = Simulator::Now();
    Time messageTime = Time::FromInteger(header.timestamp, Time::GetResolution());
    if (messageTime > currentTime + Seconds(1)) // Allow 1 second tolerance
    {
        NS_LOG_WARN("Message timestamp appears to be in the future: "
                    << messageTime << " vs current: " << currentTime);
    }

    return true;
}

std::string
WifiMpiMessageUtils::GetMessageTypeName(WifiMpiMessageType messageType)
{
    switch (messageType)
    {
    case WIFI_MPI_DEVICE_REGISTER:
        return "DEVICE_REGISTER";
    case WIFI_MPI_CONFIG_DELAY_MODEL:
        return "CONFIG_DELAY_MODEL";
    case WIFI_MPI_CONFIG_LOSS_MODEL:
        return "CONFIG_LOSS_MODEL";
    case WIFI_MPI_TX_REQUEST:
        return "TX_REQUEST";
    case WIFI_MPI_DEVICE_REMOVE:
        return "DEVICE_REMOVE";
    case WIFI_MPI_RX_NOTIFICATION:
        return "RX_NOTIFICATION";
    case WIFI_MPI_TX_START_NOTIFY:
        return "TX_START_NOTIFY";
    case WIFI_MPI_TX_END_NOTIFY:
        return "TX_END_NOTIFY";
    case WIFI_MPI_CONFIG_ACK:
        return "CONFIG_ACK";
    case WIFI_MPI_ERROR_NOTIFY:
        return "ERROR_NOTIFY";
    default:
        return "UNKNOWN";
    }
}

uint32_t
WifiMpiMessageUtils::CalculateMessageSize(uint32_t baseSize, uint32_t extraData)
{
    return baseSize + extraData;
}

template <typename T>
uint32_t
WifiMpiMessageUtils::SerializeObject(Ptr<T> obj, Buffer::Iterator& buffer)
{
    if (!obj)
    {
        buffer.WriteHtonU32(0); // Null object marker
        return sizeof(uint32_t);
    }

    // Get serialized size
    uint32_t serializedSize = obj->GetSerializedSize();
    buffer.WriteHtonU32(serializedSize);

    // Serialize the object
    Buffer tempBuffer(0);
    tempBuffer.AddAtStart(serializedSize);
    Buffer::Iterator tempIter = tempBuffer.Begin();
    obj->Serialize(tempIter);

    // Copy to main buffer
    tempIter = tempBuffer.Begin();
    for (uint32_t i = 0; i < serializedSize; ++i)
    {
        buffer.WriteU8(tempIter.ReadU8());
    }

    return sizeof(uint32_t) + serializedSize;
}

template <typename T>
Ptr<T>
WifiMpiMessageUtils::DeserializeObject(Buffer::Iterator& buffer, uint32_t size)
{
    if (size == 0)
    {
        return nullptr; // Null object
    }

    // Read the serialized data
    Buffer tempBuffer(0);
    tempBuffer.AddAtStart(size);
    Buffer::Iterator tempIter = tempBuffer.Begin();

    for (uint32_t i = 0; i < size; ++i)
    {
        tempIter.WriteU8(buffer.ReadU8());
    }

    // Create and deserialize the object
    Ptr<T> obj = Create<T>();
    tempIter = tempBuffer.Begin();
    obj->Deserialize(tempIter);

    return obj;
}

} // namespace ns3

#endif // NS3_MPI
