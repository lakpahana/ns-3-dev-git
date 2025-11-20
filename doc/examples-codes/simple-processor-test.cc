#include "ns3/core-module.h"
#ifdef NS3_MPI
#include "ns3/wifi-channel-mpi-processor.h"
#endif

using namespace ns3;

int
main()
{
    std::cout << "=== Simple Processor Test ===" << std::endl;

#ifdef NS3_MPI
    std::cout << "NS3_MPI is defined - testing MPI version" << std::endl;
    // The key test: try to create a processor
    // It should now print "MPI WifiChannelMpiProcessor constructor called"
    Ptr<WifiChannelMpiProcessor> processor = Create<WifiChannelMpiProcessor>();
    std::cout << "Processor created successfully" << std::endl;
#else
    std::cout << "NS3_MPI not defined - using stub version" << std::endl;
#endif

    return 0;
}
