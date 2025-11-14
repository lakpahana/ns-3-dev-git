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

#ifndef NS3_WIFI_MPI_MESSAGES_H
#define NS3_WIFI_MPI_MESSAGES_H

#include "ns3/nstime.h"

#include <cstdint>

namespace ns3
{

/**
 * \ingroup mpi
 * \brief Message type discriminator for WiFi MPI messages
 */
enum WifiMpiMessageType : uint8_t
{
    MPI_MSG_LEGACY_PACKET = 0,      ///< Reserve 0 for existing packet messages
    MPI_MSG_WIFI_TX_REQUEST = 1,    ///< TX Request message
    MPI_MSG_WIFI_RX_EVENT = 2,      ///< RX Event message
    MPI_MSG_WIFI_MOBILITY_UPDATE = 3, ///< Mobility Update message
    MPI_MSG_WIFI_REGISTRATION = 4   ///< Registration message
};

/**
 * \ingroup mpi
 * \brief Common header for all WiFi MPI messages
 */
struct WifiMpiMessageHeader
{
    uint64_t timeTs;      ///< Event timestamp
    uint64_t guaranteeTs; ///< Lookahead guarantee time
    uint32_t nodeId;      ///< Destination node (0 for server)
    uint32_t devIfIndex;  ///< Destination device (0 for server)
    uint8_t msgType;      ///< Message type discriminator

    /**
     * Serialize the header to a buffer
     * \param buffer The buffer to serialize to
     * \return Number of bytes serialized
     */
    uint32_t Serialize(uint8_t* buffer) const;

    /**
     * Deserialize the header from a buffer
     * \param buffer The buffer to deserialize from
     * \return Number of bytes deserialized
     */
    uint32_t Deserialize(const uint8_t* buffer);

    /**
     * Get the serialized size of the header
     * \return The serialized size in bytes
     */
    static constexpr uint32_t GetSerializedSize()
    {
        return 21; // 8+8+4+4+1
    }
};

/**
 * \ingroup mpi
 * \brief TX Request message (sender → server)
 */
struct WifiTxRequestMessage
{
    WifiMpiMessageHeader header; ///< Common header
    uint32_t senderNode;         ///< Sender node ID
    uint32_t senderDev;          ///< Sender device interface index
    uint64_t ppduUid;            ///< PPDU unique identifier
    double txPower;              ///< TX power in dBm
    uint8_t channelNumber;       ///< Channel number
    // TxVector fields
    uint8_t mcs;             ///< Modulation and coding scheme
    uint8_t nss;             ///< Number of spatial streams
    uint16_t guardInterval;  ///< Guard interval in nanoseconds
    uint16_t channelWidth;   ///< Channel width in MHz
    uint8_t preambleType;    ///< Preamble type
    // PSDU data
    uint32_t psduSize; ///< Size of PSDU data
    // Followed by: serialized PSDU bytes

    /**
     * Serialize the message to a buffer
     * \param buffer The buffer to serialize to
     * \param maxSize Maximum size of the buffer
     * \return Number of bytes serialized
     */
    uint32_t Serialize(uint8_t* buffer, uint32_t maxSize) const;

    /**
     * Deserialize the message from a buffer
     * \param buffer The buffer to deserialize from
     * \param size Size of the buffer
     * \return Number of bytes deserialized
     */
    uint32_t Deserialize(const uint8_t* buffer, uint32_t size);

    /**
     * Get the serialized size of the message (without PSDU data)
     * \return The serialized size in bytes
     */
    uint32_t GetSerializedSize() const;
};

/**
 * \ingroup mpi
 * \brief RX Event message (server → receiver rank)
 */
struct WifiRxEventMessage
{
    WifiMpiMessageHeader header; ///< Common header
    uint64_t ppduUid;            ///< PPDU unique identifier
    double rxPower;              ///< RX power in dBm
    uint64_t txDuration;         ///< TX duration in nanoseconds
    // TxVector fields (same as TX request)
    uint8_t mcs;             ///< Modulation and coding scheme
    uint8_t nss;             ///< Number of spatial streams
    uint16_t guardInterval;  ///< Guard interval in nanoseconds
    uint16_t channelWidth;   ///< Channel width in MHz
    uint8_t preambleType;    ///< Preamble type
    // PSDU data
    uint32_t psduSize; ///< Size of PSDU data
    // Followed by: serialized PSDU bytes

    /**
     * Serialize the message to a buffer
     * \param buffer The buffer to serialize to
     * \param maxSize Maximum size of the buffer
     * \return Number of bytes serialized
     */
    uint32_t Serialize(uint8_t* buffer, uint32_t maxSize) const;

    /**
     * Deserialize the message from a buffer
     * \param buffer The buffer to deserialize from
     * \param size Size of the buffer
     * \return Number of bytes deserialized
     */
    uint32_t Deserialize(const uint8_t* buffer, uint32_t size);

    /**
     * Get the serialized size of the message (without PSDU data)
     * \return The serialized size in bytes
     */
    uint32_t GetSerializedSize() const;
};

/**
 * \ingroup mpi
 * \brief Mobility Update message (rank → server)
 */
struct WifiMobilityUpdateMessage
{
    WifiMpiMessageHeader header; ///< Common header
    uint32_t nodeId;             ///< Node ID
    double posX;                 ///< X position
    double posY;                 ///< Y position
    double posZ;                 ///< Z position
    double velX;                 ///< X velocity (set to NaN if not available)
    double velY;                 ///< Y velocity
    double velZ;                 ///< Z velocity

    /**
     * Serialize the message to a buffer
     * \param buffer The buffer to serialize to
     * \return Number of bytes serialized
     */
    uint32_t Serialize(uint8_t* buffer) const;

    /**
     * Deserialize the message from a buffer
     * \param buffer The buffer to deserialize from
     * \return Number of bytes deserialized
     */
    uint32_t Deserialize(const uint8_t* buffer);

    /**
     * Get the serialized size of the message
     * \return The serialized size in bytes
     */
    static constexpr uint32_t GetSerializedSize()
    {
        return WifiMpiMessageHeader::GetSerializedSize() + 4 + 6 * 8;
    }
};

/**
 * \ingroup mpi
 * \brief Registration message (rank → server)
 */
struct WifiRegistrationMessage
{
    WifiMpiMessageHeader header; ///< Common header
    uint32_t nodeId;             ///< Node ID
    uint32_t devIfIndex;         ///< Device interface index
    uint32_t rankId;             ///< Rank ID
    uint8_t channelNumber;       ///< Channel number
    double rxSensitivity;        ///< RX sensitivity in dBm
    double rxGain;               ///< RX gain
    // Propagation model info (first registration only)
    bool hasPropagationInfo;                    ///< Whether propagation info is included
    char propagationLossModelTypeId[128];       ///< Propagation loss model type ID
    char propagationDelayModelTypeId[128];      ///< Propagation delay model type ID

    /**
     * Serialize the message to a buffer
     * \param buffer The buffer to serialize to
     * \return Number of bytes serialized
     */
    uint32_t Serialize(uint8_t* buffer) const;

    /**
     * Deserialize the message from a buffer
     * \param buffer The buffer to deserialize from
     * \return Number of bytes deserialized
     */
    uint32_t Deserialize(const uint8_t* buffer);

    /**
     * Get the serialized size of the message
     * \return The serialized size in bytes
     */
    uint32_t GetSerializedSize() const;
};

} // namespace ns3

#endif // NS3_WIFI_MPI_MESSAGES_H
