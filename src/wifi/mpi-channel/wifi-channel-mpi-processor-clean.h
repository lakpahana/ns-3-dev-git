/*
 * Copyright (c) 2025 ns-3 Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * WiFi Channel MPI Processor - Simplified Working Version Header
 *
 * Author: AI Assistant
 * Date: January 2025
 */

#ifndef WIFI_CHANNEL_MPI_PROCESSOR_H
#define WIFI_CHANNEL_MPI_PROCESSOR_H

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/vector.h"

#ifdef NS3_MPI
#include <cmath>
#include <map>
#include <vector>
#endif

namespace ns3
{

#ifdef NS3_MPI

// Forward declarations
class YansWifiChannelProxy;

/**
 * \brief Information about a remote WiFi device registered from another rank
 */
struct RemoteDeviceInfo
{
    uint32_t deviceId; //!< Unique device identifier
    uint32_t rank;     //!< MPI rank hosting this device
    Vector3D position; //!< Current position of the device
    Time lastActivity; //!< Last time device was active
    bool isActive;     //!< Whether device is currently active

    /**
     * Constructor
     * \param id The device identifier
     * \param r The MPI rank
     * \param pos The device position
     */
    RemoteDeviceInfo(uint32_t id, uint32_t r, const Vector3D& pos);
};

/**
 * \brief Information about a reception event to be sent to a device
 */
struct ReceptionInfo
{
    uint32_t receiverId;    //!< ID of receiving device
    uint32_t transmitterId; //!< ID of transmitting device
    double rxPowerDbm;      //!< Received power in dBm
    double delaySeconds;    //!< Propagation delay in seconds
    double frequency;       //!< Signal frequency in Hz
};

#endif // NS3_MPI

/**
 * \brief WiFi Channel MPI Processor for distributed simulation
 *
 * This class manages WiFi channel operations in a distributed MPI environment.
 * It runs on rank 0 (channel rank) and processes transmission requests from
 * device ranks, calculates propagation effects, and sends reception notifications.
 *
 * When MPI is not available, it operates in stub mode with no-op implementations.
 */
class WifiChannelMpiProcessor : public Object
{
  public:
    /**
     * \brief Get the type ID
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     */
    WifiChannelMpiProcessor();

    /**
     * Destructor
     */
    ~WifiChannelMpiProcessor() override;

    /**
     * \brief Initialize the MPI processor
     * \return true if initialization successful, false otherwise
     */
    bool Initialize();

    /**
     * \brief Register a device from a remote rank
     * \param sourceRank The MPI rank hosting the device
     * \param position The device position
     * \return The assigned device ID
     */
    uint32_t RegisterDevice(uint32_t sourceRank, const Vector3D& position);

    /**
     * \brief Unregister a device
     * \param deviceId The device ID to unregister
     */
    void UnregisterDevice(uint32_t deviceId);

    /**
     * \brief Update device position
     * \param deviceId The device ID
     * \param newPosition The new position
     */
    void UpdateDevicePosition(uint32_t deviceId, const Vector3D& newPosition);

    /**
     * \brief Process a transmission and calculate reception for all devices
     * \param transmitterId The transmitting device ID
     * \param txPosition The transmission position
     * \param txPowerDbm The transmission power in dBm
     * \param frequency The signal frequency in Hz
     */
    void ProcessTransmission(uint32_t transmitterId,
                             const Vector3D& txPosition,
                             double txPowerDbm,
                             double frequency);

    /**
     * \brief Get list of registered device IDs
     * \return Vector of device IDs
     */
    std::vector<uint32_t> GetRegisteredDevices() const;

    /**
     * \brief Get number of registered devices
     * \return Device count
     */
    uint32_t GetDeviceCount() const;

    /**
     * \brief Check if device is registered
     * \param deviceId The device ID to check
     * \return true if registered, false otherwise
     */
    bool IsDeviceRegistered(uint32_t deviceId) const;

  protected:
    void DoDispose() override;

  private:
#ifdef NS3_MPI
    /**
     * \brief Calculate received power
     * \param txPos Transmission position
     * \param rxPos Reception position
     * \param txPowerDbm Transmission power in dBm
     * \param frequency Signal frequency in Hz
     * \return Received power in dBm
     */
    double CalculateRxPower(const Vector3D& txPos,
                            const Vector3D& rxPos,
                            double txPowerDbm,
                            double frequency) const;

    /**
     * \brief Calculate propagation delay
     * \param txPos Transmission position
     * \param rxPos Reception position
     * \return Delay in seconds
     */
    double CalculatePropagationDelay(const Vector3D& txPos, const Vector3D& rxPos) const;

    /**
     * \brief Calculate distance between two positions
     * \param pos1 First position
     * \param pos2 Second position
     * \return Distance in meters
     */
    double CalculateDistance(const Vector3D& pos1, const Vector3D& pos2) const;

    /**
     * \brief Send reception notification to device rank
     * \param rxDevice The receiving device info
     * \param rxInfo The reception information
     */
    void SendReceptionNotification(const RemoteDeviceInfo& rxDevice, const ReceptionInfo& rxInfo);

    /**
     * \brief Log processor activity
     * \param action The action being performed
     * \param details Additional details
     */
    void LogActivity(const std::string& action, const std::string& details) const;

    bool m_initialized;                                   //!< Whether processor is initialized
    uint32_t m_systemId;                                  //!< This rank's system ID
    uint32_t m_systemCount;                               //!< Total number of ranks
    Ptr<YansWifiChannelProxy> m_channelProxy;             //!< Channel proxy reference
    uint32_t m_deviceCounter;                             //!< Device ID counter
    std::map<uint32_t, RemoteDeviceInfo> m_remoteDevices; //!< Registered remote devices
    std::map<uint32_t, Ptr<Object>> m_lossModels;         //!< Cached loss models
    std::map<uint32_t, Ptr<Object>> m_delayModels;        //!< Cached delay models
#endif                                                    // NS3_MPI
};

} // namespace ns3

#endif /* WIFI_CHANNEL_MPI_PROCESSOR_H */
