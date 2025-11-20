# Distributed WiFi MPI Simulation Guide

This guide shows you how to use the newly built distributed WiFi simulation framework with MPI support.

## 🎯 Quick Start

### Prerequisites
- NS-3 compiled with `--enable-mpi` flag
- Multiple machines with SSH access (or single machine for testing)
- MPI installed (OpenMPI or MPICH)

### 1. Basic Test - Verify MPI Setup

First, verify your MPI setup works across machines:

```bash
# Test MPI connectivity
mpirun --hostfile ~/hosts.txt hostname

# Run the basic MPI test
mpirun --oversubscribe -np 4 --hostfile ~/hosts.txt \
  ./build/scratch/ns3-dev-test-default
```

**Expected Output:**
```
Rank 0 host: ip-172-31-29-177 (PID: 12345)
Rank 1 host: ip-172-31-29-177 (PID: 12346)
Rank 2 host: ip-172-31-13-30 (PID: 67890)
Rank 3 host: ip-172-31-13-30 (PID: 67891)
```

### 2. Home WiFi Scenario (8 Devices)

Simulate a typical home WiFi network with 8 diverse devices:

```bash
# Run 60-second simulation
mpirun --oversubscribe -np 4 --hostfile ~/hosts.txt \
  ./ns3 run scratch/home-wifi-scenario

# Run for 5 minutes with heavy traffic
mpirun --oversubscribe -np 4 --hostfile ~/hosts.txt \
  ./ns3 run scratch/home-wifi-scenario
```

**What You Get:**
- 8 devices (Smart TV, Laptop, Phone, Camera, etc.)
- Different traffic patterns per device type
- Indoor propagation model
- MPI distribution across machines

### 3. Home WiFi with PCAP Capture

Get detailed packet analysis with PCAP files:

```bash
# Run with PCAP capture enabled
mpirun --oversubscribe -np 4 --hostfile ~/hosts.txt \
  ./ns3 run scratch/home-wifi-with-pcap
```

**PCAP Files Generated:**
```bash
# List captured files
ls -lh home-wifi-rank*.pcap

# Analyze with tcpdump
tcpdump -r home-wifi-rank1-0-1.pcap -n | head -20

# Open in Wireshark
wireshark home-wifi-rank1-0-1.pcap
```

### 4. V2X Highway Scenario

Simulate vehicular communication on a highway:

```bash
# Default: 4 lanes, 50 vehicles/lane, 5 minutes
mpirun --oversubscribe -np 4 --hostfile ~/hosts.txt \
  ./ns3 run scratch/v2x-mpi-scenario

# Custom configuration
mpirun --oversubscribe -np 4 --hostfile ~/hosts.txt \
  ./ns3 run "scratch/v2x-mpi-scenario --lanes=6 --vehiclesPerLane=100 --simTime=600"
```

**What You Get:**
- Multiple vehicle types (Cars, Trucks, Emergency, Motorcycles)
- Road Side Units (RSUs)
- Realistic vehicular mobility
- V2X beacon communication
- PCAP capture of V2X messages

## 📊 Understanding the Architecture

### Hybrid MPI + PCAP Approach

Each scenario uses a **dual-network architecture**:

1. **MPI Network** (Performance Testing)
   - Heavy traffic between ranks
   - Cross-machine communication
   - Tests distributed processing
   - Uses `RemoteYansWifiChannelStub`

2. **Local Monitoring Network** (PCAP Capture)
   - Light traffic for observation
   - Local WiFi channel
   - Generates PCAP files
   - Full protocol stack capture

### Rank Distribution

**4 Ranks Setup (Recommended):**
- **Rank 0**: WiFi Channel Processor (infrastructure)
- **Rank 1**: Device Group 1
- **Rank 2**: Device Group 2
- **Rank 3**: Device Group 3

**Example Distribution (Home WiFi):**
- Rank 1: Living Room TV + Kitchen Tablet
- Rank 2: Bedroom Laptop + Kids Room Phone
- Rank 3: Security Camera + Smart Speaker + Gaming Console + Smart Thermostat

## 🔧 Configuration Options

### Hostfile Setup

Create `~/hosts.txt`:
```
machine1 slots=2
machine2 slots=2
```

Or with IP addresses:
```
192.168.1.10 slots=2
192.168.1.11 slots=2
```

### Common MPI Options

```bash
# Allow oversubscription (more ranks than cores)
--oversubscribe

# Specify number of processes
-np 4

# Use hostfile
--hostfile ~/hosts.txt

# Verbose MPI debugging
--mca plm_base_verbose 10

# Network interface binding
--mca btl_tcp_if_include eth0
```

### Scenario Parameters

**Home WiFi Scenario:**
```bash
# No parameters needed - runs with defaults
./ns3 run scratch/home-wifi-scenario

# Or with PCAP
./ns3 run scratch/home-wifi-with-pcap
```

**V2X Scenario:**
```bash
./ns3 run "scratch/v2x-mpi-scenario \
  --lanes=4 \
  --vehiclesPerLane=50 \
  --laneWidth=3.5 \
  --vehicleSpacing=20.0 \
  --vehicleSpeed=30.0 \
  --rsuCount=5 \
  --simTime=300 \
  --pcap=true \
  --rngRun=1"
```

## 📈 Monitoring Performance

### During Simulation

**On Machine 1:**
```bash
# Monitor CPU and memory
htop

# Watch MPI processes
watch "ps aux | grep mpi"

# Network traffic
sudo iftop -i eth0
```

**On Machine 2:**
```bash
# Same monitoring
htop
sudo iftop -i eth0
```

### After Simulation

**Check Output:**
```bash
# Simulation completes with summary
+60.000000000s -1 === Home Device Group Results ===
+60.000000000s -1 ✅ 2 devices simulated realistic home traffic
+60.000000000s -1 ✅ TX: 12345 packets, RX: 11000 packets
```

## 🔍 Analyzing Results

### PCAP Analysis

**Quick Statistics:**
```bash
# Count packets in each file
for file in home-wifi-rank*.pcap; do
    echo "$file: $(tcpdump -r $file 2>/dev/null | wc -l) packets"
done

# Check file sizes
ls -lh *.pcap

# Analyze specific device
tcpdump -r home-wifi-rank1-0-1.pcap -n 'port 8080' | head -20
```

**Deep Analysis:**
```bash
# WiFi frame types
tcpdump -r home-wifi-rank1-0-1.pcap | grep -E "(Beacon|Probe|Auth|Assoc)" | wc -l

# Data throughput
tcpdump -r home-wifi-rank1-0-1.pcap -n 'ip' | \
  awk '{bytes+=$NF} END {print "Total:", bytes, "bytes"}'

# Traffic patterns
tcpdump -r home-wifi-rank1-0-1.pcap -n 'udp' | \
  awk '{print $3, $5}' | sort | uniq -c
```

### Wireshark Analysis

```bash
# Open specific device
wireshark home-wifi-rank1-0-1.pcap &

# Useful filters:
# - wlan.fc.type == 2 (Data frames)
# - udp.port == 8080 (Specific application)
# - ip.addr == 10.1.0.1 (Specific device)
```

## 🚀 Advanced Usage

### Custom Scenarios

Create your own scenario using the template:

```cpp
#include "ns3/remote-yans-wifi-channel-stub.h"
#include "ns3/wifi-channel-mpi-processor.h"

// Rank 0: Channel processor
if (systemId == 0) {
    Ptr<WifiChannelMpiProcessor> processor = CreateObject<WifiChannelMpiProcessor>();
    // Configure channel...
}
// Other ranks: Devices
else {
    Ptr<RemoteYansWifiChannelStub> mpiStub = CreateObject<RemoteYansWifiChannelStub>();
    mpiStub->SetRemoteChannelRank(0);
    mpiStub->SetLocalDeviceRank(systemId);
    // Configure devices...
}
```

### Scaling Up

**More Machines:**
```bash
# 6 ranks across 3 machines
mpirun --oversubscribe -np 6 --hostfile ~/hosts.txt \
  ./ns3 run scratch/v2x-mpi-scenario
```

**More Devices:**
```bash
# Increase vehicles in V2X
mpirun -np 6 --hostfile ~/hosts.txt \
  ./ns3 run "scratch/v2x-mpi-scenario --vehiclesPerLane=200"
```

### Performance Tuning

**Optimize MPI:**
```bash
# Use specific network interface
mpirun --mca btl_tcp_if_include eth0 -np 4 --hostfile ~/hosts.txt \
  ./ns3 run scratch/home-wifi-with-pcap

# Adjust buffer sizes
export OMPI_MCA_btl_tcp_sndbuf=32768
export OMPI_MCA_btl_tcp_rcvbuf=32768
```

## 🐛 Troubleshooting

### MPI Not Starting

**Problem:** `mpirun` hangs or fails to connect
```bash
# Test basic MPI
mpirun --hostfile ~/hosts.txt hostname

# Check SSH access
ssh user@machine2 hostname

# Verify MPI installation
which mpirun
mpirun --version
```

### No PCAP Files

**Problem:** PCAP files not generated

**Solution:**
1. Check working directory: `pwd`
2. Look in NS-3 root: `ls ~/ns-3-dev-git/*.pcap`
3. Files use `-1.pcap` suffix (second interface)
4. Local monitoring network creates PCAPs, not MPI network

### Rank Mismatch

**Problem:** Wrong number of ranks
```
NS_LOG_ERROR: This scenario requires 2-6 MPI ranks
```

**Solution:**
```bash
# Adjust -np to match scenario requirements
mpirun --oversubscribe -np 4 --hostfile ~/hosts.txt ...
```

### Simulation Ends Too Fast

**Problem:** Simulation completes in seconds

**Solution:** Applications may have packet limits
- Use `OnOffApplication` for continuous traffic
- Increase simulation time
- Check for early stop conditions

## 📚 Available Scenarios

| Scenario | File | Ranks | Description |
|----------|------|-------|-------------|
| Basic Test | `test.cc` | 4 | MPI integration verification |
| Home WiFi | `home-wifi-scenario.cc` | 4 | 8 devices, various types |
| Home WiFi + PCAP | `home-wifi-with-pcap.cc` | 4 | 8 devices with packet capture |
| V2X Highway | `v2x-mpi-scenario.cc` | 4-6 | Vehicular communication |
| Local PCAP Test | `local-wifi-pcap-test.cc` | 1 | Verify PCAP generation |

## 🎓 Example Workflow

**Complete workflow from setup to analysis:**

```bash
# 1. Setup hostfile
cat > ~/hosts.txt << EOF
ip-172-31-29-177 slots=2
ip-172-31-13-30 slots=2
EOF

# 2. Build NS-3
cd ~/ns-3-dev-git
./ns3 configure --enable-mpi
./ns3 build

# 3. Run simulation
mpirun --oversubscribe -np 4 --hostfile ~/hosts.txt \
  ./ns3 run scratch/home-wifi-with-pcap

# 4. Verify PCAP files
ls -lh home-wifi-rank*.pcap

# 5. Analyze results
echo "=== PCAP Analysis ==="
for file in home-wifi-rank*.pcap; do
    packets=$(tcpdump -r "$file" 2>/dev/null | wc -l)
    size=$(ls -lh "$file" | awk '{print $5}')
    echo "$file: $size, $packets packets"
done

# 6. Deep dive with Wireshark
wireshark home-wifi-rank1-0-1.pcap &

# 7. Extract statistics
tcpdump -r home-wifi-rank1-0-1.pcap -n 'ip' | \
  awk '{bytes+=$NF} END {print "Total IP bytes:", bytes}'
```

## 🔗 Key Files

**Core MPI Components:**
- `src/wifi/model/remote-yans-wifi-channel-stub.h/cc` - MPI device stub
- `src/wifi/model/wifi-channel-mpi-processor.h/cc` - MPI channel processor

**Example Scenarios:**
- `scratch/test.cc` - Basic MPI test
- `scratch/home-wifi-scenario.cc` - Home WiFi without PCAP
- `scratch/home-wifi-with-pcap.cc` - Home WiFi with PCAP capture
- `scratch/v2x-mpi-scenario.cc` - Vehicular V2X scenario
- `scratch/local-wifi-pcap-test.cc` - PCAP verification

**Helper Scripts:**
- `run-mpi-test.sh` - Automated MPI test runner
- `verify-mpi-setup.sh` - MPI connectivity verification

## 📊 Performance Expectations

**Home WiFi Scenario (60 seconds, 4 ranks, 2 machines):**
- **Simulation time:** ~60-90 seconds real time
- **PCAP files:** 50-160 MB per device
- **Packets captured:** 150k-200k per device
- **Memory usage:** 50-100 MB per rank
- **Network traffic:** ~10-50 MB MPI communication

**V2X Scenario (300 seconds, 4 ranks, 200 vehicles):**
- **Simulation time:** ~5-10 minutes real time
- **PCAP files:** 100-300 MB per rank
- **Packets captured:** 500k-1M per rank
- **Memory usage:** 200-500 MB per rank
- **Network traffic:** ~100-500 MB MPI communication

## ✅ Success Indicators

Your distributed WiFi simulation is working correctly when you see:

1. **Different hostnames** across ranks in output
2. **PCAP files generated** with substantial size (>10 MB)
3. **Packet counts** in millions for long simulations
4. **No MPI errors** in output
5. **Consistent progress** without hanging
6. **Valid WiFi frames** in PCAP (ARP, UDP, 802.11 management)

## 🎉 Next Steps

1. **Customize scenarios** - Modify device types, positions, traffic patterns
2. **Scale up** - Add more machines, more vehicles, longer simulations
3. **Analyze protocols** - Study WiFi behavior, collisions, retransmissions
4. **Optimize performance** - Tune MPI settings, network configurations
5. **Create new scenarios** - Build your own distributed WiFi applications

---

**Need Help?**
- Check logs: `NS_LOG=RemoteYansWifiChannelStub:WifiChannelMpiProcessor=level_all`
- Verify MPI: `./verify-mpi-setup.sh machine1,machine2`
- Test locally first: `mpirun --oversubscribe -np 4 ./ns3 run scratch/test`

Happy simulating! 🚀
