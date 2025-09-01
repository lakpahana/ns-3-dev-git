#ifndef WIFI_MPI_INTERFACE_H
#define WIFI_MPI_INTERFACE_H

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/packet.h"

#ifdef NS3_MPI
#include "wifi-mpi-message.h"

#include "ns3/callback.h"
#include "ns3/mpi-interface.h"
#include "ns3/ptr.h"

#include <functional>
#include <map>
#include <queue>
#endif

namespace ns3
{

#ifdef NS3_MPI

/**
 * \brief MPI communication interface for distributed WiFi simulation
 *
 * This class provides a high-level interface for WiFi MPI communication,
 * building on the existing ns-3 MPI infrastructure (MpiInterface, etc.).
 * It handles message serialization, sending, receiving, and callback dispatch.
 */
class WifiMpiInterface : public Object
{
  public:
    /**
     * \brief Get the type ID
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * \brief Constructor
     */
    WifiMpiInterface();

    /**
     * \brief Destructor
     */
    virtual ~WifiMpiInterface();

    /**
     * \brief Message handler callback type
     * \param message The received message buffer
     * \param messageType The type of message received
     * \param sourceRank The rank that sent the message
     */
    typedef Callback<void, Ptr<Packet>, WifiMpiMessageType, uint32_t> MessageHandler;

    /**
     * \brief Initialize the WiFi MPI interface
     * \param rank The MPI rank of this process
     * \param size The total number of MPI ranks
     * \return True if initialization successful
     */
    bool Initialize(uint32_t rank, uint32_t size);

    /**
     * \brief Shutdown the WiFi MPI interface
     */
    void Shutdown();

    /**
     * \brief Check if the interface is initialized
     * \return True if initialized
     */
    bool IsInitialized() const;

    /**
     * \brief Get the MPI rank of this process
     * \return The MPI rank
     */
    uint32_t GetRank() const;

    /**
     * \brief Get the total number of MPI ranks
     * \return The MPI size
     */
    uint32_t GetSize() const;

    /**
     * \brief Check if this is the channel rank (rank 0)
     * \return True if this is rank 0
     */
    bool IsChannelRank() const;

    /**
     * \brief Check if this is a device rank (rank > 0)
     * \return True if this is a device rank
     */
    bool IsDeviceRank() const;

    /**
     * \brief Register a message handler for a specific message type
     * \param messageType The type of message to handle
     * \param handler The callback to invoke when this message type is received
     */
    void RegisterMessageHandler(WifiMpiMessageType messageType, MessageHandler handler);

    /**
     * \brief Send a device registration message to the channel rank
     * \param deviceId The device (node) ID
     * \param phyId The PHY ID within the device
     * \param phyType The PHY type identifier
     * \param channelNumber The WiFi channel number
     * \param channelWidth The channel width in MHz
     */
    void SendDeviceRegister(uint32_t deviceId,
                            uint32_t phyId,
                            uint32_t phyType,
                            uint32_t channelNumber,
                            uint32_t channelWidth);

    /**
     * \brief Send a propagation model configuration message to the channel rank
     * \param deviceId The device (node) ID
     * \param configType 0=DelayModel, 1=LossModel
     * \param modelType The model TypeId hash
     * \param modelData Serialized model configuration data
     */
    void SendConfigMessage(uint32_t deviceId,
                           uint32_t configType,
                           uint32_t modelType,
                           Ptr<Packet> modelData);

    /**
     * \brief Send a transmission request to the channel rank
     * \param deviceId The device (node) ID
     * \param phyId The sender PHY ID
     * \param txPowerW Transmission power in watts
     * \param ppdu The PPDU to transmit
     * \param txVector The transmission vector
     */
    void SendTxRequest(uint32_t deviceId,
                       uint32_t phyId,
                       double txPowerW,
                       Ptr<const WifiPpdu> ppdu,
                       const WifiTxVector& txVector);

    /**
     * \brief Send a reception notification to a device rank
     * \param targetRank The device rank to notify
     * \param deviceId The device (node) ID
     * \param phyId The receiver PHY ID
     * \param rxPowerW Reception power in watts
     * \param rxDuration Reception duration
     * \param ppdu The received PPDU
     * \param signalInfo Reception signal information
     */
    void SendRxNotification(uint32_t targetRank,
                            uint32_t deviceId,
                            uint32_t phyId,
                            double rxPowerW,
                            Time rxDuration,
                            Ptr<const WifiPpdu> ppdu,
                            Ptr<Packet> signalInfo);

    /**
     * \brief Process incoming MPI messages
     *
     * This should be called periodically to handle incoming messages.
     * In the distributed simulator, this is typically called from the
     * main simulation loop.
     */
    void ProcessIncomingMessages();

    /**
     * \brief Get statistics about message traffic
     * \param sentCount Reference to store number of messages sent
     * \param receivedCount Reference to store number of messages received
     * \param bytesTransferred Reference to store total bytes transferred
     */
    void GetStatistics(uint32_t& sentCount,
                       uint32_t& receivedCount,
                       uint64_t& bytesTransferred) const;

    /**
     * \brief Enable/disable verbose logging for debugging
     * \param enable True to enable verbose logging
     */
    void SetVerboseLogging(bool enable);

  private:
    /**
     * \brief Internal message sending function
     * \param targetRank The destination MPI rank
     * \param messageType The type of message to send
     * \param messageData The message data (already serialized)
     * \param deviceId The device ID for routing
     */
    void SendMessage(uint32_t targetRank,
                     WifiMpiMessageType messageType,
                     Ptr<Packet> messageData,
                     uint32_t deviceId);

    /**
     * \brief Create a message header
     * \param messageType The message type
     * \param messageSize The total message size
     * \param targetRank The target rank
     * \param deviceId The device ID
     * \return Filled message header
     */
    WifiMpiMessageHeader CreateMessageHeader(WifiMpiMessageType messageType,
                                             uint32_t messageSize,
                                             uint32_t targetRank,
                                             uint32_t deviceId);

    /**
     * \brief Handle a received message
     * \param buffer The received message buffer
     * \param sourceRank The source MPI rank
     */
    void HandleReceivedMessage(Ptr<Packet> buffer, uint32_t sourceRank);

    /**
     * \brief Validate an incoming message
     * \param header The message header
     * \param sourceRank The source rank
     * \return True if message is valid
     */
    bool ValidateIncomingMessage(const WifiMpiMessageHeader& header, uint32_t sourceRank);

    /**
     * \brief Log message activity for debugging
     * \param direction "SEND" or "RECV"
     * \param messageType The message type
     * \param sourceRank Source rank (for received messages)
     * \param targetRank Target rank (for sent messages)
     * \param messageSize Size of the message
     */
    void LogMessageActivity(const std::string& direction,
                            WifiMpiMessageType messageType,
                            uint32_t sourceRank,
                            uint32_t targetRank,
                            uint32_t messageSize);

    bool m_initialized;    //!< Whether interface is initialized
    uint32_t m_rank;       //!< This process's MPI rank
    uint32_t m_size;       //!< Total number of MPI ranks
    bool m_verboseLogging; //!< Enable verbose debug logging

    std::map<WifiMpiMessageType, MessageHandler> m_messageHandlers; //!< Registered message handlers

    // Statistics
    uint32_t m_messagesSent;     //!< Number of messages sent
    uint32_t m_messagesReceived; //!< Number of messages received
    uint64_t m_bytesTransferred; //!< Total bytes transferred

    // Message sequencing
    uint32_t m_nextSequenceNumber; //!< Next sequence number to use

    // Message queue for processing
    struct PendingMessage
    {
        Ptr<Packet> buffer;
        uint32_t sourceRank;
        Time receiveTime;
    };

    std::queue<PendingMessage> m_pendingMessages; //!< Queue of pending messages to process
};

/**
 * \brief Global access to the WiFi MPI interface singleton
 *
 * Provides global access to the WiFi MPI interface, similar to
 * the existing MpiInterface singleton pattern in ns-3.
 */
class WifiMpi
{
  public:
    /**
     * \brief Get the global WiFi MPI interface instance
     * \return Pointer to the global instance
     */
    static Ptr<WifiMpiInterface> GetInstance();

    /**
     * \brief Initialize the global WiFi MPI interface
     * \param rank The MPI rank of this process
     * \param size The total number of MPI ranks
     * \return True if initialization successful
     */
    static bool Initialize(uint32_t rank, uint32_t size);

    /**
     * \brief Shutdown the global WiFi MPI interface
     */
    static void Shutdown();

    /**
     * \brief Check if WiFi MPI is enabled and initialized
     * \return True if enabled
     */
    static bool IsEnabled();

  private:
    static Ptr<WifiMpiInterface> s_instance; //!< Global instance
};

#endif // NS3_MPI

} // namespace ns3

#endif /* WIFI_MPI_INTERFACE_H */
