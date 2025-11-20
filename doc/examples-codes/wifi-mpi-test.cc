#include "ns3/core-module.h"

using namespace ns3;

int
main()
{
    std::cout << "=== WiFi MPI Compilation Test ===" << std::endl;

#ifdef NS3_MPI
    std::cout << "SUCCESS: NS3_MPI is defined!" << std::endl;
    std::cout << "WiFi module is now compiling with MPI support!" << std::endl;
    return 0;
#else
    std::cout << "ERROR: NS3_MPI is not defined" << std::endl;
    return 1;
#endif
}
