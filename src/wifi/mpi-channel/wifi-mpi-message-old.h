#ifndef WIFI_MPI_MESSAGE_H
#define WIFI_MPI_MESSAGE_H

#include "ns3/buffer.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"

#ifdef NS3_MPI
#include "ns3/wifi-ppdu.h"
#include "ns3/wifi-tx-vector.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"

#include <cstdint>

namespace ns3
{

/**
 * \brief MPI message types for distributed WiFi simulation
 *
 * These message types define the communication protocol between
 * device ranks and the channel rank in distributed WiFi simulation.
 */
enum WifiMpiMessageType : uint32_t
{
    // Device → Channel messages (from ranks 1-N to rank 0)
    WIFI_MPI_DEVICE_REGISTER = 100,    //!< Register device with channel
    WIFI_MPI_CONFIG_DELAY_MODEL = 101, //!< Configure propagation delay model
    WIFI_MPI_CONFIG_LOSS_MODEL = 102,  //!< Configure propagation loss model
    WIFI_MPI_TX_REQUEST = 103,         //!< Send transmission request
    
    // Channel → Device messages (from rank 0 to ranks 1-N)
    WIFI_MPI_RX_NOTIFICATION = 200,    //!< Notify device of reception
    WIFI_MPI_CHANNEL_STATE = 201,      //!< Channel state update
    
    // Bidirectional control messages
    WIFI_MPI_ERROR_RESPONSE = 300,     //!< Error response
    WIFI_MPI_HEARTBEAT = 301           //!< Heartbeat/keepalive message
};

#ifdef NS3_MPI
// Full MPI implementation when MPI is available
    WIFI_MPI_CONFIG_LOSS_MODEL = 102,  //!< Configure propagation loss model
    WIFI_MPI_TX_REQUEST = 103,         //!< Packet transmission request
    WIFI_MPI_DEVICE_REMOVE = 104,      //!< Remove device from channel

    // Channel → Device messages (from rank 0 to ranks 1-N)  
    WIFI_MPI_RX_NOTIFICATION = 200,    //!< Reception event notification
    WIFI_MPI_TX_START_NOTIFY = 201,    //!< Transmission start notification
    WIFI_MPI_TX_END_NOTIFY = 202,      //!< Transmission end notification
    WIFI_MPI_CONFIG_ACK = 203,         //!< Configuration acknowledgment
    WIFI_MPI_ERROR_NOTIFY = 204        //!< Error notification
};

/**
 * \brief Common header for all WiFi MPI messages
 *
 * This header provides message identification, routing, and timing information
 * following the existing ns-3 MPI message patterns.
 */
struct WifiMpiMessageHeader
{
    uint32_t messageType;     //!< WifiMpiMessageType
    uint32_t messageSize;     //!< Total message size in bytes
    uint32_t sourceRank;      //!< Sender MPI rank
    uint32_t targetRank;      //!< Receiver MPI rank
    uint64_t timestamp;       //!< Simulation timestamp (GetTimeStep())
    uint32_t sequenceNumber;  //!< Message sequence number
    uint32_t deviceId;        //!< Device identifier (node ID)
    uint32_t reserved;        //!< Reserved for future use

    /**
     * \brief Serialize header to buffer
     * \param buffer The buffer to write to
     */
    void Serialize(Buffer::Iterator& buffer) const;

    /**
     * \brief Deserialize header from buffer
     * \param buffer The buffer to read from
     */
    void Deserialize(Buffer::Iterator& buffer);

    /**
     * \brief Get serialized size of header
     * \return Size in bytes
     */
    static uint32_t GetSerializedSize();
};

/**
 * \brief Device registration message (Device → Channel)
 *
 * Sent when a device wants to register with the channel.
 * Based on the existing RemoteChannelBundle pattern.
 */
struct WifiMpiDeviceRegisterMessage
{
    WifiMpiMessageHeader header;
    uint32_t phyId;          //!< PHY identifier within device
    uint32_t phyType;        //!< PHY type (YansWifiPhy, etc.)
    uint32_t channelNumber;  //!< WiFi channel number
    uint32_t channelWidth;   //!< Channel width in MHz

    void Serialize(Buffer::Iterator& buffer) const;
    void Deserialize(Buffer::Iterator& buffer);
    static uint32_t GetSerializedSize();
};

/**
 * \brief Propagation model configuration message (Device → Channel)
 *
 * Sent when device configures propagation models on the channel.
 * Uses existing Buffer serialization patterns for complex objects.
 */
struct WifiMpiConfigMessage
{
    WifiMpiMessageHeader header;
    uint32_t configType;     //!< 0=DelayModel, 1=LossModel
    uint32_t modelType;      //!< Model TypeId hash
    uint32_t serializedSize; //!< Size of serialized model data

    void Serialize(Buffer::Iterator& buffer) const;
    void Deserialize(Buffer::Iterator& buffer);
    static uint32_t GetSerializedSize();
    
    // Note: Actual model data follows this struct in the buffer
};

/**
 * \brief Transmission request message (Device → Channel)
 *
 * Sent when device wants to transmit a packet through the channel.
 * Based on existing SendPacket pattern in MPI interface.
 */
struct WifiMpiTxRequestMessage
{
    WifiMpiMessageHeader header;
    uint32_t senderPhyId;    //!< Sender PHY ID
    double txPowerW;         //!< Transmission power in watts
    uint32_t packetSize;     //!< Packet size in bytes
    uint32_t ppduSerializedSize;  //!< Serialized PPDU size
    uint32_t txVectorSerializedSize; //!< Serialized TxVector size

    void Serialize(Buffer::Iterator& buffer) const;
    void Deserialize(Buffer::Iterator& buffer);
    static uint32_t GetSerializedSize();
    
    // Note: Serialized packet, PPDU, and TxVector data follow this struct
};

/**
 * \brief Reception notification message (Channel → Device)
 *
 * Sent when channel delivers a packet to a device.
 * Based on existing packet delivery patterns.
 */
struct WifiMpiRxNotificationMessage
{
    WifiMpiMessageHeader header;
    uint32_t receiverPhyId;  //!< Receiver PHY ID
    double rxPowerW;         //!< Received power in watts
    uint64_t rxDuration;     //!< Reception duration (GetTimeStep())
    uint32_t packetSize;     //!< Packet size in bytes
    uint32_t ppduSerializedSize;  //!< Serialized PPDU size
    uint32_t rxSignalInfoSize;    //!< Serialized signal info size

    void Serialize(Buffer::Iterator& buffer) const;
    void Deserialize(Buffer::Iterator& buffer);
    static uint32_t GetSerializedSize();
    
    // Note: Serialized packet, PPDU, and signal info follow this struct
};

/**
 * \brief WiFi MPI message utilities
 *
 * Provides helper functions for message creation, validation,
 * and buffer management following ns-3 patterns.
 */
class WifiMpiMessageUtils
{
public:
    /**
     * \brief Create a buffer for a message
     * \param messageType The type of message
     * \param extraSize Additional space needed beyond the struct
     * \return Buffer with correct size
     */
    static Ptr<Packet> CreateMessageBuffer(WifiMpiMessageType messageType, uint32_t extraSize = 0);

    /**
     * \brief Validate message header
     * \param header The header to validate
     * \return True if valid
     */
    static bool ValidateHeader(const WifiMpiMessageHeader& header);

    /**
     * \brief Get message type name for debugging
     * \param messageType The message type
     * \return Human-readable name
     */
    static std::string GetMessageTypeName(WifiMpiMessageType messageType);

    /**
     * \brief Calculate total message size
     * \param baseSize Size of the message struct
     * \param extraData Size of additional data
     * \return Total size including header
     */
    static uint32_t CalculateMessageSize(uint32_t baseSize, uint32_t extraData = 0);

    /**
     * \brief Serialize a Ptr<T> object to buffer
     * \param obj The object to serialize
     * \param buffer The buffer to write to
     * \return Number of bytes written
     */
    template<typename T>
    static uint32_t SerializeObject(Ptr<T> obj, Buffer::Iterator& buffer);

    /**
     * \brief Deserialize a Ptr<T> object from buffer
     * \param buffer The buffer to read from
     * \param size The size of serialized data
     * \return Deserialized object
     */
    template<typename T>
    static Ptr<T> DeserializeObject(Buffer::Iterator& buffer, uint32_t size);

private:
    static uint32_t s_sequenceNumber; //!< Global sequence number counter
};

#endif // NS3_MPI

} // namespace ns3

#endif /* WIFI_MPI_MESSAGE_H */
