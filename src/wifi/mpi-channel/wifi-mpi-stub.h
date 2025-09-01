#ifndef WIFI_MPI_STUB_H
#define WIFI_MPI_STUB_H

#ifdef NS3_MPI

namespace ns3
{

// Minimal message type enum for compilation
enum WifiMpiMessageType
{
    WIFI_MPI_DEVICE_REGISTER = 1,
    WIFI_MPI_TX_REQUEST = 2,
    WIFI_MPI_RX_NOTIFICATION = 3,
    WIFI_MPI_CONFIG_LOSS_MODEL = 4,
    WIFI_MPI_CONFIG_DELAY_MODEL = 5,
    WIFI_MPI_ERROR_RESPONSE = 6
};

// Minimal message header for compilation
struct WifiMpiMessageHeader
{
    uint32_t messageType;
    uint32_t messageSize;
    uint32_t sourceRank;
    uint32_t targetRank;
    uint64_t timestamp;
    uint32_t sequenceNumber;
    uint32_t reserved;
};

// Minimal WifiMpi class for static wrapper methods
class WifiMpi
{
  public:
    static bool IsEnabled()
    {
        return true;
    }

    static void SendDeviceRegistration(uint32_t, uint32_t, uint32_t)
    {
        // Stub implementation
    }

    static void SendTransmissionRequest(uint32_t, uint32_t, Ptr<const WifiPpdu>, dBm_u)
    {
        // Stub implementation
    }

    static void SendLossModelConfig(uint32_t, Ptr<PropagationLossModel>)
    {
        // Stub implementation
    }

    static void SendDelayModelConfig(uint32_t, Ptr<PropagationDelayModel>)
    {
        // Stub implementation
    }
};

} // namespace ns3

#endif // NS3_MPI
#endif // WIFI_MPI_STUB_H
