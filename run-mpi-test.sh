#!/bin/bash

# MPI Test Runner for NS-3 Phase 2.2.3c
# This script helps run the MPI integration test across multiple machines

echo "=== NS-3 MPI WiFi Test Runner ==="
echo "This script will run the Phase 2.2.3c MPI integration test"
echo ""

# Function to check if hosts are reachable
check_hosts() {
    local hosts="$1"
    echo "Checking host connectivity..."
    IFS=',' read -ra HOST_ARRAY <<< "$hosts"
    for host in "${HOST_ARRAY[@]}"; do
        if ping -c 1 -W 2 "$host" >/dev/null 2>&1; then
            echo "✅ $host is reachable"
        else
            echo "❌ $host is NOT reachable"
            return 1
        fi
    done
    echo ""
}

# Function to display expected output
show_expected_output() {
    echo "=== EXPECTED OUTPUT VERIFICATION ==="
    echo "For proper multi-machine execution, you should see:"
    echo "1. Different hostnames for each rank"
    echo "2. Different process IDs (PIDs)"
    echo "3. MPI messages flowing between ranks"
    echo "4. No errors about shared memory or local communication"
    echo ""
    echo "Example correct output:"
    echo "  Rank 0: Hostname: machine1, PID: 12345"
    echo "  Rank 1: Hostname: machine2, PID: 67890"
    echo "  Rank 2: Hostname: machine3, PID: 54321"
    echo "  Rank 3: Hostname: machine4, PID: 98765"
    echo ""
}

# Default configuration
DEFAULT_HOSTS="localhost,localhost,localhost,localhost"
NP=4

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -H|--hosts)
            HOSTS="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -H, --hosts HOST_LIST   Comma-separated list of hosts (default: localhost,localhost,localhost,localhost)"
            echo "  -h, --help             Show this help message"
            echo ""
            echo "Examples:"
            echo "  # Run on localhost (single machine test)"
            echo "  $0"
            echo ""
            echo "  # Run across 4 different machines"
            echo "  $0 -H machine1,machine2,machine3,machine4"
            echo ""
            echo "  # Run with mix of machines"
            echo "  $0 -H node1.cluster.edu,node2.cluster.edu,node3.cluster.edu,node4.cluster.edu"
            echo ""
            show_expected_output
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Use default hosts if not specified
if [ -z "$HOSTS" ]; then
    HOSTS="$DEFAULT_HOSTS"
    echo "⚠️  Using default localhost configuration (single machine)"
    echo "   For true multi-machine testing, use: $0 -H host1,host2,host3,host4"
    echo ""
fi

echo "Configuration:"
echo "  Hosts: $HOSTS"
echo "  Processes: $NP"
echo ""

# Check if hosts are reachable (skip for localhost)
if [[ "$HOSTS" != *"localhost"* ]]; then
    if ! check_hosts "$HOSTS"; then
        echo "❌ Host connectivity check failed. Please ensure all hosts are reachable."
        exit 1
    fi
fi

# Build the project first
echo "Building NS-3 project..."
if ! ./ns3 build; then
    echo "❌ Build failed. Please check your NS-3 setup."
    exit 1
fi
echo "✅ Build successful"
echo ""

# Show expected output before running
show_expected_output

# Run the MPI test
echo "=== STARTING MPI TEST ==="
echo "Command: mpirun -np $NP -H $HOSTS ./ns3 run scratch/test"
echo ""

# Create a timestamp for logs
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="mpi_test_${TIMESTAMP}.log"

echo "Running test... (output will be saved to $LOG_FILE)"
mpirun -np $NP -H "$HOSTS" ./ns3 run scratch/test 2>&1 | tee "$LOG_FILE"

# Analyze the results
echo ""
echo "=== TEST ANALYSIS ==="
echo "Checking for multi-machine execution indicators..."

UNIQUE_HOSTS=$(grep -o "Hostname: [^,]*" "$LOG_FILE" | sort -u | wc -l)
TOTAL_RANKS=$(grep "Hostname:" "$LOG_FILE" | wc -l)

echo "Results:"
echo "  Total ranks detected: $TOTAL_RANKS"
echo "  Unique hostnames: $UNIQUE_HOSTS"

if [ "$UNIQUE_HOSTS" -gt 1 ]; then
    echo "✅ SUCCESS: Multiple machines detected"
    echo "   Test is running across $UNIQUE_HOSTS different machines"
else
    echo "⚠️  WARNING: Only one hostname detected"
    echo "   This might be running on a single machine"
    if [[ "$HOSTS" == *"localhost"* ]]; then
        echo "   This is expected since you're using localhost"
    else
        echo "   Check your MPI configuration and host connectivity"
    fi
fi

echo ""
echo "Detailed hostname analysis:"
grep "Hostname:" "$LOG_FILE" | sort

echo ""
echo "Log saved to: $LOG_FILE"
echo "=== TEST COMPLETE ==="