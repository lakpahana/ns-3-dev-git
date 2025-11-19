#!/bin/bash

# Quick MPI verification script
# This script helps verify that MPI is working across multiple machines

echo "=== MPI Setup Verification ==="
echo ""

# Function to test basic MPI functionality
test_mpi_basic() {
    echo "1. Testing basic MPI functionality..."
    
    # Create a simple MPI test program
    cat > /tmp/mpi_test.c << 'EOF'
#include <mpi.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv) {
    char hostname[256];
    int rank, size;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    gethostname(hostname, sizeof(hostname));
    
    printf("Rank %d of %d running on hostname: %s (PID: %d)\n", 
           rank, size, hostname, getpid());
    
    MPI_Finalize();
    return 0;
}
EOF

    # Compile the test program
    if mpicc -o /tmp/mpi_test /tmp/mpi_test.c 2>/dev/null; then
        echo "✅ MPI compiler (mpicc) is working"
    else
        echo "❌ MPI compiler (mpicc) failed - please install MPI development packages"
        return 1
    fi
}

# Function to test MPI with hostfile
test_mpi_hosts() {
    local hosts="$1"
    echo ""
    echo "2. Testing MPI across specified hosts..."
    echo "   Hosts: $hosts"
    
    if mpirun -np 4 -H "$hosts" /tmp/mpi_test; then
        echo "✅ MPI execution successful"
        return 0
    else
        echo "❌ MPI execution failed"
        return 1
    fi
}

# Function to show common MPI issues and solutions
show_troubleshooting() {
    echo ""
    echo "=== TROUBLESHOOTING TIPS ==="
    echo ""
    echo "Common MPI issues and solutions:"
    echo ""
    echo "1. Permission denied / SSH key issues:"
    echo "   Solution: Set up passwordless SSH between machines"
    echo "   Command: ssh-copy-id user@remote_host"
    echo ""
    echo "2. MPI not found on remote machines:"
    echo "   Solution: Ensure MPI is installed on all machines"
    echo "   Ubuntu/Debian: sudo apt install openmpi-bin openmpi-dev"
    echo "   CentOS/RHEL: sudo yum install openmpi openmpi-devel"
    echo ""
    echo "3. Path issues:"
    echo "   Solution: Ensure mpirun and your program are in PATH on all machines"
    echo "   Add to ~/.bashrc: export PATH=\$PATH:/usr/lib64/openmpi/bin"
    echo ""
    echo "4. Firewall blocking MPI communication:"
    echo "   Solution: Open MPI ports or disable firewall for testing"
    echo "   Ports: Usually dynamic range, consider disabling firewall temporarily"
    echo ""
    echo "5. Different MPI versions:"
    echo "   Solution: Use same MPI implementation and version on all machines"
    echo "   Check with: mpirun --version"
    echo ""
}

# Function to create a hostfile template
create_hostfile_template() {
    echo ""
    echo "3. Creating MPI hostfile template..."
    
    cat > /tmp/mpi_hosts << 'EOF'
# MPI Hostfile Template
# Format: hostname slots=number_of_cores
# Example entries:

machine1.example.com slots=4
machine2.example.com slots=4
machine3.example.com slots=4
machine4.example.com slots=4

# For local testing:
# localhost slots=4

# Usage:
# mpirun -np 4 -hostfile /tmp/mpi_hosts your_program
EOF

    echo "✅ Hostfile template created at /tmp/mpi_hosts"
    echo "   Edit this file with your actual hostnames"
}

# Main execution
echo "This script will verify your MPI setup for multi-machine execution"
echo ""

# Test basic MPI
if ! test_mpi_basic; then
    show_troubleshooting
    exit 1
fi

# Get hosts from command line or use default
HOSTS="$1"
if [ -z "$HOSTS" ]; then
    HOSTS="localhost,localhost,localhost,localhost"
    echo ""
    echo "⚠️  No hosts specified, using localhost for basic test"
    echo "   Usage: $0 host1,host2,host3,host4"
fi

# Test MPI with hosts
if test_mpi_hosts "$HOSTS"; then
    echo ""
    echo "=== VERIFICATION SUCCESSFUL ==="
    echo "✅ MPI is working correctly with the specified hosts"
    echo "✅ You can now run the NS-3 MPI WiFi test"
    echo ""
    echo "Next steps:"
    echo "1. Run the NS-3 test: ./run-mpi-test.sh -H $HOSTS"
    echo "2. Check the output for different hostnames on each rank"
    echo "3. Verify that MPI communication is working between ranks"
else
    echo ""
    echo "=== VERIFICATION FAILED ==="
    show_troubleshooting
    exit 1
fi

# Create hostfile template
create_hostfile_template

# Cleanup
rm -f /tmp/mpi_test /tmp/mpi_test.c

echo ""
echo "=== VERIFICATION COMPLETE ==="