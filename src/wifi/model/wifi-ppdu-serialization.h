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

#ifndef WIFI_PPDU_SERIALIZATION_H
#define WIFI_PPDU_SERIALIZATION_H

#include "wifi-ppdu.h"
#include "wifi-tx-vector.h"

#include "ns3/nstime.h"

#include <cstdint>
#include <vector>

namespace ns3
{

/**
 * \ingroup wifi
 *
 * \brief Serialized PPDU data structure for MPI transmission
 *
 * This structure contains all necessary information to recreate a PPDU
 * on a remote MPI rank, including the PPDU UID, TxVector parameters,
 * and PSDU payload bytes.
 */
struct SerializedPpdu
{
    uint64_t ppduUid;           //!< Unique PPDU identifier
    Time txDuration;            //!< Total transmission duration
    uint16_t channelWidth;      //!< Channel width in MHz
    uint16_t guardInterval;     //!< Guard interval in nanoseconds
    uint16_t centerFreq;        //!< Center frequency in MHz
    uint8_t mcs;                //!< Modulation and coding scheme
    uint8_t nss;                //!< Number of spatial streams
    uint8_t preambleType;       //!< Preamble type (WifiPreamble)
    uint8_t modulationClass;    //!< Modulation class (WifiModulationClass)
    bool isSingle;              //!< True if S-MPDU, false if A-MPDU
    uint32_t nMpdus;            //!< Number of MPDUs in PSDU
    std::vector<uint8_t> psduBytes; //!< Serialized PSDU bytes

    /**
     * \brief Serialize this structure to a buffer
     * \param buffer Output buffer
     * \param maxSize Maximum buffer size
     * \return Number of bytes written
     */
    uint32_t Serialize(uint8_t* buffer, uint32_t maxSize) const;

    /**
     * \brief Deserialize this structure from a buffer
     * \param buffer Input buffer
     * \param size Buffer size
     * \return Number of bytes read
     */
    uint32_t Deserialize(const uint8_t* buffer, uint32_t size);

    /**
     * \brief Get the serialized size of this structure
     * \return Size in bytes
     */
    uint32_t GetSerializedSize() const;
};

/**
 * \ingroup wifi
 *
 * \brief Helper class for PPDU serialization/deserialization
 */
class WifiPpduSerialization
{
  public:
    /**
     * \brief Serialize a PPDU for MPI transmission
     * \param ppdu The PPDU to serialize
     * \return Serialized PPDU structure
     */
    static SerializedPpdu SerializePpdu(Ptr<const WifiPpdu> ppdu);

    /**
     * \brief Deserialize a PPDU received over MPI
     * \param serialized The serialized PPDU data
     * \param channel The operating channel for reconstruction
     * \return Reconstructed PPDU
     */
    static Ptr<WifiPpdu> DeserializePpdu(const SerializedPpdu& serialized,
                                          const WifiPhyOperatingChannel& channel);

    /**
     * \brief Serialize PSDU payload to byte vector
     * \param psdu The PSDU to serialize
     * \return Serialized bytes
     */
    static std::vector<uint8_t> SerializePsdu(Ptr<const WifiPsdu> psdu);

    /**
     * \brief Deserialize PSDU from byte vector
     * \param bytes Serialized PSDU bytes
     * \param nMpdus Number of MPDUs in the PSDU
     * \param isSingle True if S-MPDU
     * \return Reconstructed PSDU
     */
    static Ptr<WifiPsdu> DeserializePsdu(const std::vector<uint8_t>& bytes,
                                          uint32_t nMpdus,
                                          bool isSingle);

    /**
     * \brief Serialize TxVector parameters
     * \param txVector The TxVector to serialize
     * \param buffer Output buffer (must be at least GetTxVectorSerializedSize() bytes)
     * \return Number of bytes written
     */
    static uint32_t SerializeTxVector(const WifiTxVector& txVector, uint8_t* buffer);

    /**
     * \brief Deserialize TxVector parameters
     * \param buffer Input buffer
     * \param txVector Output TxVector
     * \return Number of bytes read
     */
    static uint32_t DeserializeTxVector(const uint8_t* buffer, WifiTxVector& txVector);

    /**
     * \brief Get the serialized size of TxVector parameters
     * \return Size in bytes
     */
    static constexpr uint32_t GetTxVectorSerializedSize()
    {
        return 1 + 1 + 2 + 2 + 1 + 1; // mcs + nss + guardInterval + channelWidth + preamble +
                                      // modulationClass
    }
};

} // namespace ns3

#endif /* WIFI_PPDU_SERIALIZATION_H */
