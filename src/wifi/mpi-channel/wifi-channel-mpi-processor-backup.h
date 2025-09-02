#ifndef WIFI_CHANNEL_MPI_PROCESSOR_H
#define WIFI_CHANNEL_MPI_PROCESSOR_H

#include "ns3/object.h"
#include "ns3/nstime.h"
#include "ns3/vector.h"
#include "ns3/packet.h"
#include "ns3/callback.h"

#ifdef NS3_MPI
#include "wifi-mpi-stub.h"
#include "ns3/mpi-interface.h"
#include <map>
#include <set>
#endif

namespace ns3
{

#ifdef NS3_MPI

// Forward declarations
class YansWifiChannelProxy;
class PropagationLossModel;
class PropagationDelayModel;
class WifiPpdu;

/**
 * \brief Information about a remote WiFi device registered from another rank
 */
struct RemoteDeviceInfo
{
    uint32_t deviceId;           //!< Unique device identifier
    uint32_t rank;              //!< MPI rank where device is located
    Vector3D position;          //!< Device position for propagation calculation
    double antennaGainDb;       //!< Antenna gain in dB
    uint32_t frequency;         //!< Operating frequency in Hz
    Time lastActivity;          //!< Last time this device was active
    bool isActive;              //!< Whether device is currently active
    
    RemoteDeviceInfo()
        : deviceId(0),
          rank(0),
          position(0, 0, 0),
          antennaGainDb(0.0),
          frequency(0),
          lastActivity(Seconds(0)),
          isActive(true)
    {
    }
    
    RemoteDeviceInfo(uint32_t id, uint32_t r, const Vector3D& pos)
        : deviceId(id),
          rank(r),
          position(pos),
          antennaGainDb(0.0),
          frequency(2400000000), // Default 2.4 GHz
          lastActivity(Simulator::Now()),
          isActive(true)
    {
    }
};

/**
 * \brief Reception information for transmission events
 */
struct ReceptionInfo
{
    uint32_t transmitterId;     //!< ID of transmitting device
    uint32_t receiverId;        //!< ID of receiving device
    double rxPowerDbm;          //!< Received power in dBm
    double txPowerDbm;          //!< Transmitted power in dBm
    Time propagationDelay;      //!< Signal propagation delay
    Time duration;              //!< Signal duration
    uint32_t packetSize;        //!< Packet size in bytes
    
    ReceptionInfo()
        : transmitterId(0),
          receiverId(0),
          rxPowerDbm(-100.0),
          txPowerDbm(0.0),
          propagationDelay(Seconds(0)),
          duration(Seconds(0)),
          packetSize(0)
    {
    }
};

/**
 * \brief MPI processor for WiFi channel operations running on rank 0
 *
 * This class handles all MPI message processing for the distributed WiFi channel.
 * It receives messages from device ranks, processes them using real propagation
 * models, and sends back reception notifications.
 *
 * Key responsibilities:
 * - Device registration from remote ranks
 * - Transmission request processing with propagation calculation
 * - Reception notification to target devices
 * - Configuration updates for propagation models
 * - Spatial device management for efficient processing
 */
class WifiChannelMpiProcessor : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    
    /**
     * \brief Constructor
     */
    WifiChannelMpiProcessor();
    
    /**
     * \brief Destructor
     */
    virtual ~WifiChannelMpiProcessor();
    
    // Core lifecycle methods
    
    /**
     * \brief Initialize the MPI processor
     * \return true if initialization successful
     */
    bool Initialize();
    
    /**
     * \brief Start processing MPI messages
     */
    void StartProcessing();
    
    /**
     * \brief Stop processing MPI messages
     */
    void StopProcessing();
    
    /**
     * \brief Check if processor is currently running
     * \return true if processing messages
     */
    bool IsProcessing() const;
    
    // Configuration methods
    
    /**
     * \brief Set the WiFi channel proxy for propagation calculations
     * \param channel The channel proxy instance
     */
    void SetChannelProxy(Ptr<YansWifiChannelProxy> channel);
    
    /**
     * \brief Get the current channel proxy
     * \return The channel proxy instance
     */
    Ptr<YansWifiChannelProxy> GetChannelProxy() const;
    
    /**
     * \brief Set the reception power threshold
     * \param thresholdDbm Minimum reception power in dBm
     */
    void SetReceptionThreshold(double thresholdDbm);
    
    /**
     * \brief Get the current reception threshold
     * \return Reception threshold in dBm
     */
    double GetReceptionThreshold() const;
    
    // Device management methods
    
    /**
     * \brief Get number of registered remote devices
     * \return Number of devices across all ranks
     */
    uint32_t GetNumRegisteredDevices() const;
    
    /**
     * \brief Get number of devices on a specific rank
     * \param rank The MPI rank to query
     * \return Number of devices on that rank
     */
    uint32_t GetNumDevicesOnRank(uint32_t rank) const;
    
    /**
     * \brief Get all registered device IDs
     * \return Vector of all device IDs
     */
    std::vector<uint32_t> GetRegisteredDeviceIds() const;
    
    /**
     * \brief Check if a device is registered
     * \param deviceId The device ID to check
     * \return true if device is registered
     */
    bool IsDeviceRegistered(uint32_t deviceId) const;
    
    /**
     * \brief Get information about a specific device
     * \param deviceId The device ID
     * \return Device information structure
     */
    RemoteDeviceInfo GetDeviceInfo(uint32_t deviceId) const;
    
    // Statistics and monitoring
    
    /**
     * \brief Get total number of processed messages
     * \return Total message count
     */
    uint64_t GetTotalProcessedMessages() const;
    
    /**
     * \brief Get number of processed transmission requests
     * \return Transmission request count
     */
    uint64_t GetProcessedTransmissions() const;
    
    /**
     * \brief Get number of sent reception notifications
     * \return Reception notification count
     */
    uint64_t GetSentReceptionNotifications() const;
    
    /**
     * \brief Print processor statistics
     */
    void PrintStatistics() const;
    
    // Message handler callbacks (for testing and monitoring)
    
    /**
     * \brief Set callback for device registration events
     * \param callback Function to call when device registers
     */
    void SetDeviceRegistrationCallback(Callback<void, uint32_t, uint32_t> callback);
    
    /**
     * \brief Set callback for transmission processing events
     * \param callback Function to call when transmission is processed
     */
    void SetTransmissionProcessingCallback(Callback<void, uint32_t, uint32_t> callback);
    
  protected:
    /**
     * \brief Dispose of the object
     */
    void DoDispose() override;
    
  private:
    // Core MPI message processing
    
    /**
     * \brief Main message processing callback from MpiReceiver
     * \param packet The received MPI packet
     */
    void ProcessIncomingMessage(Ptr<Packet> packet);
    
    /**
     * \brief Route message to appropriate handler based on type
     * \param header The message header
     * \param packet The message payload
     * \param sourceRank The source MPI rank
     */
    void RouteMessage(const WifiMpiMessageHeader& header, Ptr<Packet> packet, uint32_t sourceRank);
    
    // Specific message handlers
    
    /**
     * \brief Handle device registration from remote rank
     * \param packet The message packet
     * \param sourceRank Source MPI rank
     */
    void HandleDeviceRegistration(Ptr<Packet> packet, uint32_t sourceRank);
    
    /**
     * \brief Handle transmission request from remote device
     * \param packet The message packet
     * \param sourceRank Source MPI rank
     */
    void HandleTransmissionRequest(Ptr<Packet> packet, uint32_t sourceRank);
    
    /**
     * \brief Handle configuration update from remote rank
     * \param packet The message packet
     * \param sourceRank Source MPI rank
     */
    void HandleConfigurationUpdate(Ptr<Packet> packet, uint32_t sourceRank);
    
    /**
     * \brief Handle device unregistration from remote rank
     * \param packet The message packet
     * \param sourceRank Source MPI rank
     */
    void HandleDeviceUnregistration(Ptr<Packet> packet, uint32_t sourceRank);
    
    // Propagation calculation and processing
    
    /**
     * \brief Calculate devices in communication range for transmission
     * \param txDevice Transmitting device info
     * \param txPowerDbm Transmission power in dBm
     * \param frequency Operating frequency
     * \return List of devices that can receive the transmission
     */
    std::vector<RemoteDeviceInfo> GetDevicesInRange(const RemoteDeviceInfo& txDevice, 
                                                    double txPowerDbm, 
                                                    uint32_t frequency);
    
    /**
     * \brief Calculate reception power for a device pair
     * \param txDevice Transmitting device
     * \param rxDevice Receiving device
     * \param txPowerDbm Transmission power in dBm
     * \return Reception power in dBm
     */
    double CalculateReceptionPower(const RemoteDeviceInfo& txDevice, 
                                   const RemoteDeviceInfo& rxDevice, 
                                   double txPowerDbm);
    
    /**
     * \brief Calculate propagation delay between devices
     * \param txDevice Transmitting device
     * \param rxDevice Receiving device
     * \return Propagation delay
     */
    Time CalculatePropagationDelay(const RemoteDeviceInfo& txDevice, 
                                   const RemoteDeviceInfo& rxDevice);
    
    /**
     * \brief Send reception notification to target device
     * \param rxDevice Receiving device info
     * \param rxInfo Reception information
     */
    void SendReceptionNotification(const RemoteDeviceInfo& rxDevice, const ReceptionInfo& rxInfo);
    
    // Utility methods
    
    /**
     * \brief Calculate distance between two positions
     * \param pos1 First position
     * \param pos2 Second position
     * \return Distance in meters
     */
    double CalculateDistance(const Vector3D& pos1, const Vector3D& pos2) const;
    
    /**
     * \brief Check if device is within communication range
     * \param txDevice Transmitting device
     * \param rxDevice Receiving device
     * \param txPowerDbm Transmission power in dBm
     * \return true if in range
     */
    bool IsInCommunicationRange(const RemoteDeviceInfo& txDevice, 
                                const RemoteDeviceInfo& rxDevice, 
                                double txPowerDbm) const;
    
    /**
     * \brief Generate unique message ID for logging
     * \return Unique message ID
     */
    uint64_t GenerateMessageId();
    
    /**
     * \brief Log processor activity for debugging
     * \param method Method name
     * \param details Activity details
     */
    void LogActivity(const std::string& method, const std::string& details) const;
    
    // Member variables
    
    bool m_initialized;                                    //!< Whether processor is initialized
    bool m_processing;                                     //!< Whether currently processing messages
    Ptr<MpiReceiver> m_mpiReceiver;                       //!< MPI message receiver
    Ptr<YansWifiChannelProxy> m_channelProxy;             //!< WiFi channel for propagation calculations
    
    // Device registry
    std::map<uint32_t, RemoteDeviceInfo> m_remoteDevices; //!< All registered remote devices
    std::map<uint32_t, std::set<uint32_t>> m_devicesByRank; //!< Devices grouped by rank for efficient lookup
    
    // Configuration
    double m_receptionThresholdDbm;                        //!< Minimum reception power threshold
    uint64_t m_nextMessageId;                             //!< Next unique message ID
    
    // Statistics
    uint64_t m_totalMessages;                             //!< Total processed messages
    uint64_t m_processedTransmissions;                    //!< Processed transmission requests
    uint64_t m_sentNotifications;                         //!< Sent reception notifications
    uint64_t m_registeredDevices;                         //!< Total device registrations
    
    // Callbacks for monitoring
    Callback<void, uint32_t, uint32_t> m_deviceRegistrationCallback;  //!< Device registration callback
    Callback<void, uint32_t, uint32_t> m_transmissionProcessingCallback; //!< Transmission processing callback
};

#else // NS3_MPI not defined

/**
 * \brief Stub implementation when MPI is not available
 */
class WifiChannelMpiProcessor : public Object
{
  public:
    static TypeId GetTypeId();
    WifiChannelMpiProcessor();
    virtual ~WifiChannelMpiProcessor();
    
    bool Initialize();
    void StartProcessing();
    void StopProcessing();
    bool IsProcessing() const;
    
    uint32_t GetNumRegisteredDevices() const;
    uint64_t GetTotalProcessedMessages() const;
    uint64_t GetProcessedTransmissions() const;
    uint64_t GetSentReceptionNotifications() const;
    double GetReceptionThreshold() const;
    void PrintStatistics() const;
};

#endif // NS3_MPI

} // namespace ns3

#endif /* WIFI_CHANNEL_MPI_PROCESSOR_H */
