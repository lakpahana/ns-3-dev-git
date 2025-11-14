/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wifi-mpi-messages.h"

#include <cstring>

namespace ns3
{

// Helper functions for serialization
template <typename T>
void
SerializeValue(uint8_t*& buffer, const T& value)
{
    std::memcpy(buffer, &value, sizeof(T));
    buffer += sizeof(T);
}

template <typename T>
void
DeserializeValue(const uint8_t*& buffer, T& value)
{
    std::memcpy(&value, buffer, sizeof(T));
    buffer += sizeof(T);
}

// WifiMpiMessageHeader implementation

uint32_t
WifiMpiMessageHeader::Serialize(uint8_t* buffer) const
{
    uint8_t* start = buffer;
    SerializeValue(buffer, timeTs);
    SerializeValue(buffer, guaranteeTs);
    SerializeValue(buffer, nodeId);
    SerializeValue(buffer, devIfIndex);
    SerializeValue(buffer, msgType);
    return buffer - start;
}

uint32_t
WifiMpiMessageHeader::Deserialize(const uint8_t* buffer)
{
    const uint8_t* start = buffer;
    DeserializeValue(buffer, timeTs);
    DeserializeValue(buffer, guaranteeTs);
    DeserializeValue(buffer, nodeId);
    DeserializeValue(buffer, devIfIndex);
    DeserializeValue(buffer, msgType);
    return buffer - start;
}

// WifiTxRequestMessage implementation

uint32_t
WifiTxRequestMessage::Serialize(uint8_t* buffer, uint32_t maxSize) const
{
    uint8_t* start = buffer;

    // Serialize header
    buffer += header.Serialize(buffer);

    // Serialize message fields
    SerializeValue(buffer, senderNode);
    SerializeValue(buffer, senderDev);
    SerializeValue(buffer, ppduUid);
    SerializeValue(buffer, txPower);
    SerializeValue(buffer, channelNumber);
    SerializeValue(buffer, mcs);
    SerializeValue(buffer, nss);
    SerializeValue(buffer, guardInterval);
    SerializeValue(buffer, channelWidth);
    SerializeValue(buffer, preambleType);
    SerializeValue(buffer, psduSize);

    // PSDU data would be serialized here by the caller

    return buffer - start;
}

uint32_t
WifiTxRequestMessage::Deserialize(const uint8_t* buffer, uint32_t size)
{
    const uint8_t* start = buffer;

    // Deserialize header
    buffer += header.Deserialize(buffer);

    // Deserialize message fields
    DeserializeValue(buffer, senderNode);
    DeserializeValue(buffer, senderDev);
    DeserializeValue(buffer, ppduUid);
    DeserializeValue(buffer, txPower);
    DeserializeValue(buffer, channelNumber);
    DeserializeValue(buffer, mcs);
    DeserializeValue(buffer, nss);
    DeserializeValue(buffer, guardInterval);
    DeserializeValue(buffer, channelWidth);
    DeserializeValue(buffer, preambleType);
    DeserializeValue(buffer, psduSize);

    // PSDU data would be deserialized here by the caller

    return buffer - start;
}

uint32_t
WifiTxRequestMessage::GetSerializedSize() const
{
    return WifiMpiMessageHeader::GetSerializedSize() + sizeof(senderNode) + sizeof(senderDev) +
           sizeof(ppduUid) + sizeof(txPower) + sizeof(channelNumber) + sizeof(mcs) + sizeof(nss) +
           sizeof(guardInterval) + sizeof(channelWidth) + sizeof(preambleType) + sizeof(psduSize);
}

// WifiRxEventMessage implementation

uint32_t
WifiRxEventMessage::Serialize(uint8_t* buffer, uint32_t maxSize) const
{
    uint8_t* start = buffer;

    // Serialize header
    buffer += header.Serialize(buffer);

    // Serialize message fields
    SerializeValue(buffer, ppduUid);
    SerializeValue(buffer, rxPower);
    SerializeValue(buffer, txDuration);
    SerializeValue(buffer, mcs);
    SerializeValue(buffer, nss);
    SerializeValue(buffer, guardInterval);
    SerializeValue(buffer, channelWidth);
    SerializeValue(buffer, preambleType);
    SerializeValue(buffer, psduSize);

    // PSDU data would be serialized here by the caller

    return buffer - start;
}

uint32_t
WifiRxEventMessage::Deserialize(const uint8_t* buffer, uint32_t size)
{
    const uint8_t* start = buffer;

    // Deserialize header
    buffer += header.Deserialize(buffer);

    // Deserialize message fields
    DeserializeValue(buffer, ppduUid);
    DeserializeValue(buffer, rxPower);
    DeserializeValue(buffer, txDuration);
    DeserializeValue(buffer, mcs);
    DeserializeValue(buffer, nss);
    DeserializeValue(buffer, guardInterval);
    DeserializeValue(buffer, channelWidth);
    DeserializeValue(buffer, preambleType);
    DeserializeValue(buffer, psduSize);

    // PSDU data would be deserialized here by the caller

    return buffer - start;
}

uint32_t
WifiRxEventMessage::GetSerializedSize() const
{
    return WifiMpiMessageHeader::GetSerializedSize() + sizeof(ppduUid) + sizeof(rxPower) +
           sizeof(txDuration) + sizeof(mcs) + sizeof(nss) + sizeof(guardInterval) +
           sizeof(channelWidth) + sizeof(preambleType) + sizeof(psduSize);
}

// WifiMobilityUpdateMessage implementation

uint32_t
WifiMobilityUpdateMessage::Serialize(uint8_t* buffer) const
{
    uint8_t* start = buffer;

    // Serialize header
    buffer += header.Serialize(buffer);

    // Serialize message fields
    SerializeValue(buffer, nodeId);
    SerializeValue(buffer, posX);
    SerializeValue(buffer, posY);
    SerializeValue(buffer, posZ);
    SerializeValue(buffer, velX);
    SerializeValue(buffer, velY);
    SerializeValue(buffer, velZ);

    return buffer - start;
}

uint32_t
WifiMobilityUpdateMessage::Deserialize(const uint8_t* buffer)
{
    const uint8_t* start = buffer;

    // Deserialize header
    buffer += header.Deserialize(buffer);

    // Deserialize message fields
    DeserializeValue(buffer, nodeId);
    DeserializeValue(buffer, posX);
    DeserializeValue(buffer, posY);
    DeserializeValue(buffer, posZ);
    DeserializeValue(buffer, velX);
    DeserializeValue(buffer, velY);
    DeserializeValue(buffer, velZ);

    return buffer - start;
}

// WifiRegistrationMessage implementation

uint32_t
WifiRegistrationMessage::Serialize(uint8_t* buffer) const
{
    uint8_t* start = buffer;

    // Serialize header
    buffer += header.Serialize(buffer);

    // Serialize message fields
    SerializeValue(buffer, nodeId);
    SerializeValue(buffer, devIfIndex);
    SerializeValue(buffer, rankId);
    SerializeValue(buffer, channelNumber);
    SerializeValue(buffer, rxSensitivity);
    SerializeValue(buffer, rxGain);
    SerializeValue(buffer, hasPropagationInfo);

    // Serialize propagation model type IDs
    std::memcpy(buffer, propagationLossModelTypeId, 128);
    buffer += 128;
    std::memcpy(buffer, propagationDelayModelTypeId, 128);
    buffer += 128;

    return buffer - start;
}

uint32_t
WifiRegistrationMessage::Deserialize(const uint8_t* buffer)
{
    const uint8_t* start = buffer;

    // Deserialize header
    buffer += header.Deserialize(buffer);

    // Deserialize message fields
    DeserializeValue(buffer, nodeId);
    DeserializeValue(buffer, devIfIndex);
    DeserializeValue(buffer, rankId);
    DeserializeValue(buffer, channelNumber);
    DeserializeValue(buffer, rxSensitivity);
    DeserializeValue(buffer, rxGain);
    DeserializeValue(buffer, hasPropagationInfo);

    // Deserialize propagation model type IDs
    std::memcpy(propagationLossModelTypeId, buffer, 128);
    buffer += 128;
    std::memcpy(propagationDelayModelTypeId, buffer, 128);
    buffer += 128;

    return buffer - start;
}

uint32_t
WifiRegistrationMessage::GetSerializedSize() const
{
    return WifiMpiMessageHeader::GetSerializedSize() + sizeof(nodeId) + sizeof(devIfIndex) +
           sizeof(rankId) + sizeof(channelNumber) + sizeof(rxSensitivity) + sizeof(rxGain) +
           sizeof(hasPropagationInfo) + 128 + 128;
}

} // namespace ns3
