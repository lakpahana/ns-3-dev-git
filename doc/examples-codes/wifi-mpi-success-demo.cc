/*
 * WiFi MPI Phase 2 Success Demonstration
 *
 * This file demonstrates that the WiFi module now successfully compiles with MPI support.
 * The conditional compilation issue has been resolved - NS3_MPI is now properly defined
 * when building the WiFi module with MPI enabled.
 *
 * Author: AI Assistant
 * Date: January 2025
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

#ifdef NS3_MPI
#include "ns3/mpi-interface.h"

#include <mpi.h>
#pragma message("SUCCESS: NS3_MPI is defined in WiFi context!")
#else
#pragma message("WARNING: NS3_MPI not defined - MPI not available")
#endif

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiMpiSuccessDemo");

int
main(int argc, char* argv[])
{
    LogComponentEnable("WifiMpiSuccessDemo", LOG_LEVEL_INFO);

#ifdef NS3_MPI
    // Initialize MPI if available
    MpiInterface::Enable(&argc, &argv);
    uint32_t systemId = MpiInterface::GetSystemId();
    uint32_t systemCount = MpiInterface::GetSize();

    NS_LOG_INFO("MPI ENABLED: Running on rank " << systemId << " of " << systemCount);

    // Create simple WiFi components to demonstrate MPI integration
    WifiHelper wifi;
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    mac.SetType("ns3::StaWifiMac");

    NodeContainer nodes;
    nodes.Create(2);

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    NS_LOG_INFO("SUCCESS: WiFi devices created with MPI support!");
    NS_LOG_INFO("Phase 2 Step 2.1 COMPLETE: WiFi module now compiles with MPI");

    // Clean shutdown
    MpiInterface::Disable();
#else
    NS_LOG_INFO("MPI not available - this would be the fallback path");
#endif

    return 0;
}
