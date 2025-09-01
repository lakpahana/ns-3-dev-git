#ifndef WIFI_MPI_MESSAGES_H
#define WIFI_MPI_MESSAGES_H

#include "ns3/buffer.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/wifi-phy-common.h"
#include "ns3/wifi-tx-vector.h"

namespace ns3
{

/**
 * \brief Message types for WiFi MPI communication
 */
enum WifiMpiMessageType
{
    WIFI_MPI_TX_REQUEST = 1,      //!< Device -> Channel: Transmission request
    WIFI_MPI_RX_NOTIFICATION = 2, //!< Channel -> Device: Reception notification
    WIFI_MPI_HEARTBEAT = 3        //!< Time synchronization message
};

/**
 * \brief Transmission request message (Device -> Channel)
 */
struct WifiMpiTxRequest
{
    uint32_t senderId;   //!< ID of the sending device
    uint32_t senderRank; //!< MPI rank of the sending device
    double txPowerW;     //!< Transmission power in Watts
    Time txTime;         //!< Transmission time
    uint32_t packetSize; //!< Size of the packet in bytes
    // Note: Actual packet data would be serialized separately

    /**
     * \brief Default constructor
     */
    WifiMpiTxRequest();

    /**
     * \brief Serialize the request using a buffer iterator
     * \param start The buffer iterator to write to
     */
    void Serialize(Buffer::Iterator& start) const;

    /**
     * \brief Deserialize the request using a buffer iterator
     * \param start The buffer iterator to read from
     * \return The number of bytes read
     */
    uint32_t Deserialize(Buffer::Iterator& start);

    /**
     * \brief Get the serialized size
     * \return The size in bytes when serialized
     */
    uint32_t GetSerializedSize() const;
};

/**
 * \brief Reception notification message (Channel -> Device)
 */
struct WifiMpiRxNotification
{
    uint32_t receiverId;   //!< ID of the receiving device
    uint32_t receiverRank; //!< MPI rank of the receiving device
    double rxPowerW;       //!< Received power in Watts
    double snr;            //!< Signal-to-Noise Ratio
    Time rxTime;           //!< Reception time
    uint32_t packetSize;   //!< Size of the packet in bytes
    // Note: Actual packet data would be serialized separately

    /**
     * \brief Default constructor
     */
    WifiMpiRxNotification();

    /**
     * \brief Serialize the notification using a buffer iterator
     * \param start The buffer iterator to write to
     */
    void Serialize(Buffer::Iterator& start) const;

    /**
     * \brief Deserialize the notification using a buffer iterator
     * \param start The buffer iterator to read from
     * \return The number of bytes read
     */
    uint32_t Deserialize(Buffer::Iterator& start);

    /**
     * \brief Get the serialized size
     * \return The size in bytes when serialized
     */
    uint32_t GetSerializedSize() const;
};

/**
 * \brief Heartbeat message for time synchronization
 */
struct WifiMpiHeartbeat
{
    Time currentTime;    //!< Current simulation time
    uint32_t sourceRank; //!< Rank sending the heartbeat

    /**
     * \brief Default constructor
     */
    WifiMpiHeartbeat();

    /**
     * \brief Serialize the heartbeat using a buffer iterator
     * \param start The buffer iterator to write to
     */
    void Serialize(Buffer::Iterator& start) const;

    /**
     * \brief Deserialize the heartbeat using a buffer iterator
     * \param start The buffer iterator to read from
     * \return The number of bytes read
     */
    uint32_t Deserialize(Buffer::Iterator& start);

    /**
     * \brief Get the serialized size
     * \return The size in bytes when serialized
     */
    uint32_t GetSerializedSize() const;
};

} // namespace ns3

#endif /* WIFI_MPI_MESSAGES_H */
