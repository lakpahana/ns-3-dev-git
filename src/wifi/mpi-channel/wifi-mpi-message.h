#ifndef WIFI_MPI_MESSAGE_H
#define WIFI_MPI_MESSAGE_H

#include "ns3/buffer.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/ptr.h"
#include "ns3/wifi-ppdu.h"
#include "ns3/wifi-tx-vector.h"

#include <cstdint>
#include <map>
#include <string>

namespace ns3
{

/**
 * \brief MPI message types for distributed WiFi simulation
 */
enum WifiMpiMessageType : uint32_t
{
    WIFI_MPI_DEVICE_REGISTER = 1,
    WIFI_MPI_CONFIG_LOSS_MODEL = 2,
    WIFI_MPI_TX_REQUEST = 3,
    WIFI_MPI_RX_NOTIFICATION = 4,
    WIFI_MPI_CONFIG_DELAY_MODEL = 5,
    WIFI_MPI_CHANNEL_STATE = 6,
    WIFI_MPI_ERROR_RESPONSE = 7,
    WIFI_MPI_HEARTBEAT = 8
};

#ifdef NS3_MPI

/**
 * \brief Common header for all WiFi MPI messages
 */
struct WifiMpiMessageHeader
{
    uint32_t messageType;    //!< WifiMpiMessageType
    uint32_t messageSize;    //!< Total message size in bytes
    uint32_t sequenceNumber; //!< Unique sequence number
    uint32_t sourceRank;     //!< Source MPI rank
    uint32_t targetRank;     //!< Target MPI rank
    uint64_t timestamp;      //!< Simulation timestamp in nanoseconds
    uint32_t checksum;       //!< Message integrity checksum
    uint32_t reserved;       //!< Reserved for future use

    /**
     * \brief Serialize the header to a buffer
     * \param buffer Buffer to serialize to
     */
    void Serialize(Buffer::Iterator& buffer) const;

    /**
     * \brief Deserialize the header from a buffer
     * \param buffer Buffer to deserialize from
     */
    void Deserialize(Buffer::Iterator& buffer);

    /**
     * \brief Get the serialized size of the header
     * \return Size in bytes
     */
    static uint32_t GetSerializedSize();

    /**
     * \brief Calculate checksum for the header
     * \return Checksum value
     */
    uint32_t CalculateChecksum() const;

    /**
     * \brief Validate the header integrity
     * \return True if header is valid
     */
    bool IsValid() const;
};

/**
 * \brief Message for device registration with the channel
 */
struct WifiMpiDeviceRegisterMessage
{
    WifiMpiMessageHeader header;
    uint32_t deviceId;      //!< Device identifier
    uint32_t phyId;         //!< PHY layer identifier
    uint32_t phyType;       //!< PHY type hash
    uint32_t channelNumber; //!< Channel number
    uint32_t channelWidth;  //!< Channel width in MHz
    uint32_t nodeId;        //!< Node identifier

    void Serialize(Buffer::Iterator& buffer) const;
    void Deserialize(Buffer::Iterator& buffer);
    static uint32_t GetSerializedSize();
};

/**
 * \brief Message for propagation model configuration
 */
struct WifiMpiConfigMessage
{
    WifiMpiMessageHeader header;
    uint32_t configType;     //!< Configuration type (loss model, delay model)
    uint32_t modelType;      //!< Model type identifier
    uint32_t parametersSize; //!< Size of serialized parameters
    // Variable-length parameters data follows

    void Serialize(Buffer::Iterator& buffer, const std::string& parameters) const;
    void Deserialize(Buffer::Iterator& buffer, std::string& parameters);
    static uint32_t GetSerializedSize(uint32_t parametersSize);
};

/**
 * \brief Message for transmission requests
 */
struct WifiMpiTxRequestMessage
{
    WifiMpiMessageHeader header;
    uint32_t deviceId;     //!< Source device identifier
    uint32_t phyId;        //!< Source PHY identifier
    double txPowerW;       //!< Transmission power in Watts
    uint32_t ppduSize;     //!< Serialized PPDU size
    uint32_t txVectorSize; //!< Serialized TxVector size
    // Variable-length PPDU and TxVector data follows

    void Serialize(Buffer::Iterator& buffer,
                   Ptr<const WifiPpdu> ppdu,
                   const WifiTxVector& txVector) const;
    void Deserialize(Buffer::Iterator& buffer, Ptr<WifiPpdu>& ppdu, WifiTxVector& txVector);
    static uint32_t GetSerializedSize(uint32_t ppduSize, uint32_t txVectorSize);
};

/**
 * \brief Message for reception notifications - Enhanced for Phase 2.2.3c
 */
struct WifiMpiRxNotificationMessage
{
    WifiMpiMessageHeader header;
    uint32_t receiverDeviceId;      //!< Target device identifier for reception
    uint32_t transmitterDeviceId;   //!< Source device that transmitted
    uint32_t targetPhyId;           //!< Target PHY identifier
    double rxPowerW;                //!< Received power in Watts
    double rxPowerDbm;              //!< Received power in dBm (for convenience)
    double pathLossDb;              //!< Path loss in dB
    double distanceM;               //!< Distance between devices in meters
    uint32_t frequency;             //!< Transmission frequency in Hz
    uint64_t propagationDelay;      //!< Propagation delay in nanoseconds
    uint32_t ppduSize;              //!< Serialized PPDU size (0 for simplified mode)
    uint64_t transmissionTimestamp; //!< Original transmission timestamp
    // Variable-length PPDU data follows (optional for Phase 2.2.3c)

    void Serialize(Buffer::Iterator& buffer, Ptr<const WifiPpdu> ppdu = nullptr) const;
    void Deserialize(Buffer::Iterator& buffer, Ptr<WifiPpdu>& ppdu);
    static uint32_t GetSerializedSize(uint32_t ppduSize = 0);

    // Phase 2.2.3c helper methods
    void SetSimplifiedMode(uint32_t rxDeviceId,
                           uint32_t txDeviceId,
                           double rxPowerDbm,
                           double pathLoss,
                           double distance,
                           uint32_t freq);

    bool IsSimplifiedMode() const
    {
        return ppduSize == 0;
    }
};

/**
 * \brief Utility class for WiFi MPI message operations
 */
class WifiMpiMessageUtils
{
  public:
    /**
     * \brief Create a message buffer with proper header
     * \param messageType Type of message to create
     * \param extraSize Additional size needed beyond header
     * \return Initialized packet buffer
     */
    static Ptr<Packet> CreateMessageBuffer(WifiMpiMessageType messageType, uint32_t extraSize = 0);

    /**
     * \brief Validate a message header
     * \param header Header to validate
     * \return True if header is valid
     */
    static bool ValidateHeader(const WifiMpiMessageHeader& header);

    /**
     * \brief Get message type name for logging
     * \param messageType Message type to get name for
     * \return String representation of message type
     */
    static std::string GetMessageTypeName(WifiMpiMessageType messageType);

    /**
     * \brief Serialize any ns-3 object to buffer
     * \param obj Object to serialize
     * \param buffer Buffer to write to
     * \return Number of bytes written
     */
    template <typename T>
    static uint32_t SerializeObject(Ptr<const T> obj, Buffer::Iterator& buffer);

    /**
     * \brief Deserialize any ns-3 object from buffer
     * \param buffer Buffer to read from
     * \param size Number of bytes to read
     * \return Deserialized object
     */
    template <typename T>
    static Ptr<T> DeserializeObject(Buffer::Iterator& buffer, uint32_t size);

  private:
    static uint32_t s_sequenceNumber; //!< Global sequence number counter
};

#endif // NS3_MPI

} // namespace ns3

#endif /* WIFI_MPI_MESSAGE_H */
