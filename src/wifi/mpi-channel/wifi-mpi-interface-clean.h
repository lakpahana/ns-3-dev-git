/*
 * Copyright (c) 2025 ns-3 Project
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * WiFi MPI Interface Header - Working Version
 *
 * Author: AI Assistant
 * Date: January 2025
 */

#ifndef WIFI_MPI_INTERFACE_H
#define WIFI_MPI_INTERFACE_H

#include "ns3/ptr.h"

namespace ns3
{

// Forward declarations
class WifiPpdu;
class PropagationLossModel;
class PropagationDelayModel;

/**
 * \brief WiFi MPI Interface for distributed simulation
 *
 * Provides interface for WiFi devices to communicate with the
 * channel processor in distributed MPI simulations.
 */
class WifiMpi
{
  public:
    /**
     * \brief Check if WiFi MPI is enabled
     * \return true if MPI is available and enabled
     */
    static bool IsEnabled();

    /**
     * \brief Send device registration to channel rank
     * \param rank The target MPI rank (usually 0)
     * \param deviceId The device identifier
     * \param nodeId The ns-3 node identifier
     */
    static void SendDeviceRegistration(uint32_t rank, uint32_t deviceId, uint32_t nodeId);

    /**
     * \brief Send transmission request to channel rank
     * \param rank The target MPI rank (usually 0)
     * \param deviceId The transmitting device identifier
     * \param ppdu The WiFi PPDU being transmitted
     * \param txPowerDbm The transmission power in dBm
     */
    static void SendTransmissionRequest(uint32_t rank,
                                        uint32_t deviceId,
                                        Ptr<const WifiPpdu> ppdu,
                                        double txPowerDbm);

    /**
     * \brief Send propagation loss model configuration
     * \param rank The target MPI rank
     * \param model The loss model to configure
     */
    static void SendLossModelConfig(uint32_t rank, Ptr<PropagationLossModel> model);

    /**
     * \brief Send propagation delay model configuration
     * \param rank The target MPI rank
     * \param model The delay model to configure
     */
    static void SendDelayModelConfig(uint32_t rank, Ptr<PropagationDelayModel> model);
};

} // namespace ns3

#endif /* WIFI_MPI_INTERFACE_H */
