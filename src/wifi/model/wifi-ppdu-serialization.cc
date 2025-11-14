/*
 * Copyright (c) 2025
 *
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

#include "wifi-ppdu-serialization.h"

#include "wifi-mac-header.h"
#include "wifi-phy-common.h"
#include "wifi-psdu.h"

#include "ns3/log.h"
#include "ns3/packet.h"

#include <cstring>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiPpduSerialization");

uint32_t
SerializedPpdu::Serialize(uint8_t* buffer, uint32_t maxSize) const
{
    NS_LOG_FUNCTION(this << maxSize);

    uint32_t requiredSize = GetSerializedSize();
    if (maxSize < requiredSize)
    {
        NS_LOG_ERROR("Buffer too small: " << maxSize << " < " << requiredSize);
        return 0;
    }

    uint8_t* ptr = buffer;

    // Serialize fixed-size fields
    std::memcpy(ptr, &ppduUid, sizeof(ppduUid));
    ptr += sizeof(ppduUid);

    uint64_t txDurationNs = txDuration.GetNanoSeconds();
    std::memcpy(ptr, &txDurationNs, sizeof(txDurationNs));
    ptr += sizeof(txDurationNs);

    std::memcpy(ptr, &channelWidth, sizeof(channelWidth));
    ptr += sizeof(channelWidth);

    std::memcpy(ptr, &guardInterval, sizeof(guardInterval));
    ptr += sizeof(guardInterval);

    std::memcpy(ptr, &centerFreq, sizeof(centerFreq));
    ptr += sizeof(centerFreq);

    *ptr++ = mcs;
    *ptr++ = nss;
    *ptr++ = preambleType;
    *ptr++ = modulationClass;
    *ptr++ = isSingle ? 1 : 0;

    std::memcpy(ptr, &nMpdus, sizeof(nMpdus));
    ptr += sizeof(nMpdus);

    uint32_t psduSize = psduBytes.size();
    std::memcpy(ptr, &psduSize, sizeof(psduSize));
    ptr += sizeof(psduSize);

    // Serialize PSDU bytes
    if (psduSize > 0)
    {
        std::memcpy(ptr, psduBytes.data(), psduSize);
        ptr += psduSize;
    }

    return ptr - buffer;
}

uint32_t
SerializedPpdu::Deserialize(const uint8_t* buffer, uint32_t size)
{
    NS_LOG_FUNCTION(this << size);

    const uint8_t* ptr = buffer;
    const uint8_t* end = buffer + size;

    // Check minimum size for fixed fields
    uint32_t minSize = sizeof(ppduUid) + sizeof(uint64_t) + sizeof(channelWidth) +
                       sizeof(guardInterval) + sizeof(centerFreq) + 5 + sizeof(nMpdus) +
                       sizeof(uint32_t);
    if (size < minSize)
    {
        NS_LOG_ERROR("Buffer too small for deserialization: " << size << " < " << minSize);
        return 0;
    }

    // Deserialize fixed-size fields
    std::memcpy(&ppduUid, ptr, sizeof(ppduUid));
    ptr += sizeof(ppduUid);

    uint64_t txDurationNs;
    std::memcpy(&txDurationNs, ptr, sizeof(txDurationNs));
    txDuration = NanoSeconds(txDurationNs);
    ptr += sizeof(txDurationNs);

    std::memcpy(&channelWidth, ptr, sizeof(channelWidth));
    ptr += sizeof(channelWidth);

    std::memcpy(&guardInterval, ptr, sizeof(guardInterval));
    ptr += sizeof(guardInterval);

    std::memcpy(&centerFreq, ptr, sizeof(centerFreq));
    ptr += sizeof(centerFreq);

    mcs = *ptr++;
    nss = *ptr++;
    preambleType = *ptr++;
    modulationClass = *ptr++;
    isSingle = (*ptr++ != 0);

    std::memcpy(&nMpdus, ptr, sizeof(nMpdus));
    ptr += sizeof(nMpdus);

    uint32_t psduSize;
    std::memcpy(&psduSize, ptr, sizeof(psduSize));
    ptr += sizeof(psduSize);

    // Check if remaining buffer is sufficient for PSDU
    if (ptr + psduSize > end)
    {
        NS_LOG_ERROR("Insufficient data for PSDU: expected " << psduSize << " bytes");
        return 0;
    }

    // Deserialize PSDU bytes
    psduBytes.resize(psduSize);
    if (psduSize > 0)
    {
        std::memcpy(psduBytes.data(), ptr, psduSize);
        ptr += psduSize;
    }

    return ptr - buffer;
}

uint32_t
SerializedPpdu::GetSerializedSize() const
{
    return sizeof(ppduUid) +        // 8 bytes
           sizeof(uint64_t) +       // txDuration as nanoseconds (8 bytes)
           sizeof(channelWidth) +   // 2 bytes
           sizeof(guardInterval) +  // 2 bytes
           sizeof(centerFreq) +     // 2 bytes
           1 +                      // mcs
           1 +                      // nss
           1 +                      // preambleType
           1 +                      // modulationClass
           1 +                      // isSingle
           sizeof(nMpdus) +         // 4 bytes
           sizeof(uint32_t) +       // psduSize
           psduBytes.size();        // PSDU data
}

SerializedPpdu
WifiPpduSerialization::SerializePpdu(Ptr<const WifiPpdu> ppdu)
{
    NS_LOG_FUNCTION(ppdu);

    SerializedPpdu serialized;

    // Extract PPDU metadata
    serialized.ppduUid = ppdu->GetUid();
    serialized.txDuration = ppdu->GetTxDuration();
    serialized.centerFreq = ppdu->GetTxCenterFreq();

    // Extract TxVector parameters
    const WifiTxVector& txVector = ppdu->GetTxVector();
    serialized.channelWidth = txVector.GetChannelWidth();
    serialized.guardInterval = txVector.GetGuardInterval();
    serialized.mcs = txVector.GetMode().GetMcsValue();
    serialized.nss = txVector.GetNss();
    serialized.preambleType = static_cast<uint8_t>(txVector.GetPreambleType());
    serialized.modulationClass = static_cast<uint8_t>(txVector.GetMode().GetModulationClass());

    // Extract PSDU
    Ptr<const WifiPsdu> psdu = ppdu->GetPsdu();
    serialized.isSingle = psdu->IsSingle();
    serialized.nMpdus = psdu->GetNMpdus();
    serialized.psduBytes = SerializePsdu(psdu);

    NS_LOG_DEBUG("Serialized PPDU UID=" << serialized.ppduUid << " size="
                                        << serialized.GetSerializedSize() << " bytes");

    return serialized;
}

Ptr<WifiPpdu>
WifiPpduSerialization::DeserializePpdu(const SerializedPpdu& serialized,
                                        const WifiPhyOperatingChannel& channel)
{
    NS_LOG_FUNCTION_NOARGS();

    // Reconstruct PSDU
    Ptr<WifiPsdu> psdu =
        DeserializePsdu(serialized.psduBytes, serialized.nMpdus, serialized.isSingle);

    if (!psdu)
    {
        NS_LOG_ERROR("Failed to deserialize PSDU");
        return nullptr;
    }

    // Reconstruct TxVector
    WifiTxVector txVector;
    txVector.SetChannelWidth(serialized.channelWidth);
    txVector.SetGuardInterval(serialized.guardInterval);
    txVector.SetNss(serialized.nss);
    txVector.SetPreambleType(static_cast<WifiPreamble>(serialized.preambleType));

    // Set mode based on modulation class and MCS
    WifiMode mode;
    WifiModulationClass modClass = static_cast<WifiModulationClass>(serialized.modulationClass);

    // Create mode based on modulation class
    // This is a simplified approach - in practice, you'd need to properly reconstruct
    // the mode from the modulation class and MCS value
    switch (modClass)
    {
    case WIFI_MOD_CLASS_OFDM:
    case WIFI_MOD_CLASS_ERP_OFDM:
        mode = WifiMode("OfdmRate" + std::to_string(6 * (1 << serialized.mcs)) + "Mbps");
        break;
    case WIFI_MOD_CLASS_HT:
        mode = WifiMode("HtMcs" + std::to_string(serialized.mcs));
        break;
    case WIFI_MOD_CLASS_VHT:
        mode = WifiMode("VhtMcs" + std::to_string(serialized.mcs));
        break;
    case WIFI_MOD_CLASS_HE:
        mode = WifiMode("HeMcs" + std::to_string(serialized.mcs));
        break;
    case WIFI_MOD_CLASS_EHT:
        mode = WifiMode("EhtMcs" + std::to_string(serialized.mcs));
        break;
    default:
        NS_LOG_WARN("Unsupported modulation class: " << static_cast<int>(modClass));
        mode = WifiMode("OfdmRate6Mbps"); // Fallback
        break;
    }

    txVector.SetMode(mode);

    // Create PPDU with reconstructed parameters
    Ptr<WifiPpdu> ppdu = Create<WifiPpdu>(psdu, txVector, channel, serialized.ppduUid);

    NS_LOG_DEBUG("Deserialized PPDU UID=" << serialized.ppduUid);

    return ppdu;
}

std::vector<uint8_t>
WifiPpduSerialization::SerializePsdu(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(psdu);

    std::vector<uint8_t> bytes;

    if (!psdu)
    {
        return bytes;
    }

    // Get the full packet representation
    Ptr<const Packet> packet = psdu->GetPacket();
    uint32_t size = packet->GetSize();

    // Resize buffer and serialize
    bytes.resize(size);
    if (size > 0)
    {
        packet->CopyData(bytes.data(), size);
    }

    NS_LOG_DEBUG("Serialized PSDU: " << size << " bytes");

    return bytes;
}

Ptr<WifiPsdu>
WifiPpduSerialization::DeserializePsdu(const std::vector<uint8_t>& bytes,
                                        uint32_t nMpdus,
                                        bool isSingle)
{
    NS_LOG_FUNCTION(bytes.size() << nMpdus << isSingle);

    if (bytes.empty())
    {
        NS_LOG_ERROR("Cannot deserialize empty PSDU");
        return nullptr;
    }

    // Create packet from bytes
    Ptr<Packet> packet = Create<Packet>(bytes.data(), bytes.size());

    // For simplicity, create a basic MAC header
    // In a full implementation, you would deserialize the actual MAC headers
    WifiMacHeader header;
    header.SetType(WIFI_MAC_QOSDATA);
    header.SetQosAckPolicy(WifiMacHeader::NORMAL_ACK);

    // Create PSDU
    Ptr<WifiPsdu> psdu = Create<WifiPsdu>(packet, header);

    NS_LOG_DEBUG("Deserialized PSDU: " << bytes.size() << " bytes, " << nMpdus << " MPDUs");

    return psdu;
}

uint32_t
WifiPpduSerialization::SerializeTxVector(const WifiTxVector& txVector, uint8_t* buffer)
{
    NS_LOG_FUNCTION_NOARGS();

    uint8_t* ptr = buffer;

    *ptr++ = txVector.GetMode().GetMcsValue();
    *ptr++ = txVector.GetNss();

    uint16_t gi = txVector.GetGuardInterval();
    std::memcpy(ptr, &gi, sizeof(gi));
    ptr += sizeof(gi);

    uint16_t chWidth = txVector.GetChannelWidth();
    std::memcpy(ptr, &chWidth, sizeof(chWidth));
    ptr += sizeof(chWidth);

    *ptr++ = static_cast<uint8_t>(txVector.GetPreambleType());
    *ptr++ = static_cast<uint8_t>(txVector.GetMode().GetModulationClass());

    return ptr - buffer;
}

uint32_t
WifiPpduSerialization::DeserializeTxVector(const uint8_t* buffer, WifiTxVector& txVector)
{
    NS_LOG_FUNCTION_NOARGS();

    const uint8_t* ptr = buffer;

    uint8_t mcs = *ptr++;
    uint8_t nss = *ptr++;

    uint16_t gi;
    std::memcpy(&gi, ptr, sizeof(gi));
    ptr += sizeof(gi);

    uint16_t chWidth;
    std::memcpy(&chWidth, ptr, sizeof(chWidth));
    ptr += sizeof(chWidth);

    uint8_t preamble = *ptr++;
    uint8_t modClass = *ptr++;

    // Set TxVector parameters
    txVector.SetNss(nss);
    txVector.SetChannelWidth(chWidth);
    txVector.SetGuardInterval(gi);
    txVector.SetPreambleType(static_cast<WifiPreamble>(preamble));

    // Reconstruct mode (simplified)
    WifiModulationClass modulation = static_cast<WifiModulationClass>(modClass);
    WifiMode mode;

    switch (modulation)
    {
    case WIFI_MOD_CLASS_HT:
        mode = WifiMode("HtMcs" + std::to_string(mcs));
        break;
    case WIFI_MOD_CLASS_VHT:
        mode = WifiMode("VhtMcs" + std::to_string(mcs));
        break;
    case WIFI_MOD_CLASS_HE:
        mode = WifiMode("HeMcs" + std::to_string(mcs));
        break;
    default:
        mode = WifiMode("OfdmRate6Mbps");
        break;
    }

    txVector.SetMode(mode);

    return ptr - buffer;
}

} // namespace ns3
