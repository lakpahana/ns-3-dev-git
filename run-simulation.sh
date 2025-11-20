#!/bin/bash
#
# Simple WiFi Simulation Script
# Demonstrates distributed WiFi scenarios with MPI
#

set -e  # Exit on error

echo "=========================================="
echo "  Distributed WiFi Simulation Script"
echo "=========================================="
echo ""

# Configuration
HOSTS_FILE="${HOME}/hosts.txt"
NS3_DIR="${HOME}/ns-3-dev-git"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Helper functions
print_step() {
    echo -e "${BLUE}==>${NC} $1"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

# Check if in NS-3 directory
if [ ! -f "ns3" ]; then
    print_error "Not in NS-3 directory. Please cd to ${NS3_DIR}"
    exit 1
fi

# Check if MPI is available
if ! command -v mpirun &> /dev/null; then
    print_error "mpirun not found. Please install MPI (OpenMPI or MPICH)"
    exit 1
fi

print_success "Environment checks passed"
echo ""

# Main menu
echo "Available Scenarios:"
echo "  1) Simple Test (2 ranks) - Basic MPI verification"
echo "  2) Home WiFi (4 ranks) - 8 devices, no PCAP"
echo "  3) Home WiFi + PCAP (4 ranks) - 8 devices with packet capture"
echo "  4) V2X Highway (4 ranks) - Vehicular communication"
echo "  5) Local WiFi Test (1 rank) - PCAP verification"
echo ""

read -p "Select scenario (1-5): " choice

case $choice in
    1)
        SCENARIO="test"
        RANKS=2
        DESCRIPTION="Simple MPI Test"
        BINARY="build/scratch/ns3-dev-test-default"
        USE_NS3=false
        ;;
    2)
        SCENARIO="home-wifi-scenario"
        RANKS=4
        DESCRIPTION="Home WiFi (no PCAP)"
        BINARY=""
        USE_NS3=true
        ;;
    3)
        SCENARIO="home-wifi-with-pcap"
        RANKS=4
        DESCRIPTION="Home WiFi with PCAP"
        BINARY=""
        USE_NS3=true
        ;;
    4)
        SCENARIO="v2x-mpi-scenario"
        RANKS=4
        DESCRIPTION="V2X Highway"
        BINARY=""
        USE_NS3=true
        ;;
    5)
        SCENARIO="local-wifi-pcap-test"
        RANKS=1
        DESCRIPTION="Local WiFi PCAP Test"
        BINARY=""
        USE_NS3=true
        ;;
    *)
        print_error "Invalid choice"
        exit 1
        ;;
esac

echo ""
print_step "Selected: ${DESCRIPTION}"

# Ask for execution mode
if [ $RANKS -gt 1 ]; then
    echo ""
    echo "Execution mode:"
    echo "  1) Local (single machine, --oversubscribe)"
    echo "  2) Distributed (multiple machines, hostfile)"
    read -p "Select mode (1-2): " mode
else
    mode=1
fi

# Build the scenario
if [ "$USE_NS3" = true ]; then
    print_step "Building scenario..."
    if ./ns3 build 2>&1 | tail -5; then
        print_success "Build completed"
    else
        print_error "Build failed"
        exit 1
    fi
fi

# Prepare command
echo ""
print_step "Preparing simulation..."

if [ $mode -eq 1 ]; then
    # Local execution
    print_step "Running locally with ${RANKS} ranks"
    if [ "$USE_NS3" = true ]; then
        CMD="mpirun --oversubscribe -np ${RANKS} ./ns3 run scratch/${SCENARIO}"
    else
        CMD="mpirun --oversubscribe -np ${RANKS} ${BINARY}"
    fi
else
    # Distributed execution
    if [ ! -f "${HOSTS_FILE}" ]; then
        print_error "Hostfile not found: ${HOSTS_FILE}"
        print_step "Creating example hostfile..."
        cat > "${HOSTS_FILE}" << EOF
localhost slots=2
localhost slots=2
EOF
        print_success "Created ${HOSTS_FILE} (update with your machines)"
        exit 1
    fi
    
    print_step "Running distributed with hostfile: ${HOSTS_FILE}"
    cat "${HOSTS_FILE}"
    
    if [ "$USE_NS3" = true ]; then
        CMD="mpirun --oversubscribe -np ${RANKS} --hostfile ${HOSTS_FILE} ./ns3 run scratch/${SCENARIO}"
    else
        CMD="mpirun --oversubscribe -np ${RANKS} --hostfile ${HOSTS_FILE} ${BINARY}"
    fi
fi

echo ""
print_step "Command: ${CMD}"
echo ""
read -p "Press Enter to start simulation..."

# Run simulation
echo ""
echo "=========================================="
echo "  Running Simulation"
echo "=========================================="
echo ""

START_TIME=$(date +%s)

if $CMD; then
    END_TIME=$(date +%s)
    DURATION=$((END_TIME - START_TIME))
    
    echo ""
    echo "=========================================="
    print_success "Simulation completed in ${DURATION} seconds"
    echo "=========================================="
    
    # Check for PCAP files
    if [ "$SCENARIO" = "home-wifi-with-pcap" ] || [ "$SCENARIO" = "local-wifi-pcap-test" ] || [ "$SCENARIO" = "v2x-mpi-scenario" ]; then
        echo ""
        print_step "Checking for PCAP files..."
        
        PCAP_COUNT=$(ls -1 *.pcap 2>/dev/null | wc -l)
        if [ $PCAP_COUNT -gt 0 ]; then
            print_success "Found ${PCAP_COUNT} PCAP files:"
            ls -lh *.pcap | tail -10
            
            echo ""
            echo "Quick PCAP analysis:"
            for file in $(ls *.pcap 2>/dev/null | head -3); do
                PACKETS=$(tcpdump -r "$file" 2>/dev/null | wc -l)
                SIZE=$(ls -lh "$file" | awk '{print $5}')
                echo "  - $file: $SIZE, $PACKETS packets"
            done
            
            echo ""
            echo "To analyze PCAP files:"
            echo "  tcpdump -r <file>.pcap -n | head -20"
            echo "  wireshark <file>.pcap"
        else
            print_error "No PCAP files found"
        fi
    fi
    
    echo ""
    print_success "All done!"
    
else
    print_error "Simulation failed"
    exit 1
fi
