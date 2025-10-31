# ESP32 Network Configuration Guide

## Overview

This document describes the WiFi networking implementation for the Physics Lab ESP32 sensor nodes. The implementation allows multiple ESP32 nodes to send sensor data over WiFi to a Raspberry Pi server for real-time dashboard display.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    WiFi Access Point (AP)                   │
│                  (e.g., PhysicsLab_AP)                      │
│                                                             │
│  2.4 GHz: ESP32 Nodes (reliable, longer range)             │
│  5 GHz:   Student devices (higher bandwidth)               │
└─────────────────────────────────────────────────────────────┘
           │                                    │
           │ Static IP                          │ DHCP
           │ 192.168.1.101-110                  │ Dynamic IPs
           │                                    │
    ┌──────▼─────┐                       ┌─────▼──────┐
    │  ESP32 #1  │                       │  Students  │
    │  ESP32 #2  │                       │  (Mobile)  │
    │  ESP32 #n  │                       │  Devices   │
    └────────────┘                       └────────────┘
           │
           │ UDP Port 5000
           │ JSON Data Packets
           │
    ┌──────▼──────────────────────┐
    │   Raspberry Pi 5 Server     │
    │   IP: 192.168.1.10          │
    │   - UDP Listener (Port 5000)│
    │   - Web Server              │
    │   - Dashboard Display       │
    └─────────────────────────────┘
```

## What Was Implemented

### 1. NetworkTask State Machine

A new FreeRTOS task (`NetworkTask`) was added to handle all WiFi connectivity and data transmission. It follows the same architectural pattern as existing tasks (SensorTask, DisplayTask, etc.).

**State Machine States:**
- **BOOT**: Wait for BMS to latch before initializing WiFi
- **INIT**: Initialize WiFi hardware and configure static IP
- **CONNECTING**: Initial 30-second connection attempt with flashing LED
- **CONNECTED**: Successfully connected, sending data at task rate
- **DISCONNECTED**: Connection lost after initial success, retry every 10 seconds
- **NO_NETWORK**: Terminal state if initial connection fails (LED off, no retries)

### 2. Connection Behavior

As per your requirements:
- **On wake-up**: Wait for BMS latching, then try to connect for 30 seconds while flashing LED on pin 15
- **If unsuccessful**: Turn LED off, assume no network available, don't retry
- **If successful**: Keep LED solid on, start sending data
- **If connection lost**: Retry every 10 seconds indefinitely (LED flashing)

### 3. Data Transmission

- Sends the **last frame** from SharedBuffer at TASK_RATE_NETWORK (50 Hz)
- Uses UDP protocol for low-latency, connectionless transmission
- Data formatted as JSON for easy parsing on the server side
- Only includes sensors that have valid data (checked via timestamp-based has*() methods)

### 4. LED Status Indication

- **OFF**: No network available, or initial connection failed
- **FLASHING**: Attempting to connect/reconnect
- **SOLID ON**: Successfully connected and transmitting

## Configuration Guide

### Per-Node Configuration

Each ESP32 node must be configured individually. Edit the file:
```
SW_design/PlatformIO_C6/src/shared_resources/globals.h
```

**Key settings to modify for each node:**

```cpp
// =========== NETWORK CONFIGURATION ===============

// 1. WiFi Credentials (same for all nodes)
#define NETWORK_SSID                "PhysicsLab_AP"      // Your WiFi SSID
#define NETWORK_PASSWORD            "physics2025"        // Your WiFi password

// 2. Node Identification (MUST BE UNIQUE FOR EACH ESP32!)
#define NETWORK_NODE_ID             "ESP32_01"           // Change to ESP32_02, ESP32_03, etc.

// 3. Static IP Configuration (MUST BE UNIQUE FOR EACH ESP32!)
#define NETWORK_STATIC_IP           192,168,1,101        // ESP32 #1: .101, ESP32 #2: .102, etc.

// 4. Network Settings (same for all nodes)
#define NETWORK_GATEWAY             192,168,1,1          // Your router/AP IP
#define NETWORK_SUBNET              255,255,255,0        // Standard subnet mask
#define NETWORK_HOSTNAME            "PhysicsLab-ESP32-01"// Change for each node

// 5. Server Configuration (same for all nodes)
#define SERVER_IP_ADDRESS           192,168,1,10         // Your Raspberry Pi IP
#define SERVER_UDP_PORT             5000                 // UDP port for data

// Note: Data is sent at TASK_RATE_NETWORK (50 Hz, defined in Task Rates section)
```

### Example: Configuring Multiple Nodes

**ESP32 Node #1:**
```cpp
#define NETWORK_NODE_ID             "ESP32_01"
#define NETWORK_STATIC_IP           192,168,1,101
#define NETWORK_HOSTNAME            "PhysicsLab-ESP32-01"
```

**ESP32 Node #2:**
```cpp
#define NETWORK_NODE_ID             "ESP32_02"
#define NETWORK_STATIC_IP           192,168,1,102
#define NETWORK_HOSTNAME            "PhysicsLab-ESP32-02"
```

**ESP32 Node #3:**
```cpp
#define NETWORK_NODE_ID             "ESP32_03"
#define NETWORK_STATIC_IP           192,168,1,103
#define NETWORK_HOSTNAME            "PhysicsLab-ESP32-03"
```

## Data Format

### JSON Packet Structure

Each UDP packet contains a JSON object with the following structure:

```json
{
  "node_id": "ESP32_01",
  "timestamp": 123456,
  "temperature": 23.45,
  "pressure": 101325.00,
  "light_intensity": 5432.10,
  "accel_x": 0.012,
  "accel_y": -0.005,
  "accel_z": 1.003,
  "accel_norm": 1.004,
  "gyro_x": 0.234,
  "gyro_y": -0.123,
  "gyro_z": 0.456,
  "gyro_norm": 0.567,
  "mag_x": 23.4,
  "mag_y": -12.3,
  "mag_z": 45.6,
  "mag_norm": 52.1,
  "volume_rms": -25.3,
  "spectral_f1": 1234,
  "spectral_f2": 2345,
  ... (14 spectral channels if available)
}
```

**Notes:**
- Only sensors with valid data are included in the packet
- `timestamp`: Milliseconds since ESP32 boot (from `millis()`)
- Spectral data only included if AS7343 sensor is detected
- All floating-point values are formatted to 2-3 decimal places

### Sensor Data Fields

| Field | Description | Unit | Source Sensor |
|-------|-------------|------|---------------|
| `node_id` | ESP32 node identifier | string | Configuration |
| `timestamp` | Time since boot | ms | System |
| `temperature` | Ambient temperature | °C | ICP201XX |
| `pressure` | Atmospheric pressure | Pa | ICP201XX |
| `light_intensity` | Ambient light level | lux | BH1750 |
| `accel_x/y/z` | Linear acceleration | mg | ICM20948 |
| `accel_norm` | Acceleration magnitude | mg | ICM20948 |
| `gyro_x/y/z` | Rotational velocity | deg/s | ICM20948 |
| `gyro_norm` | Gyro magnitude | deg/s | ICM20948 |
| `mag_x/y/z` | Magnetic field | µT | ICM20948 |
| `mag_norm` | Magnetic magnitude | µT | ICM20948 |
| `volume_rms` | Audio level | dB | MICT3902 |
| `spectral_f1-f8` | Spectral channels | ADC counts | AS7343 |
| `spectral_fz/fy/fxl` | Special channels | ADC counts | AS7343 |
| `spectral_nir` | Near-infrared | ADC counts | AS7343 |
| `spectral_vis` | Visible light | ADC counts | AS7343 |
| `spectral_fd` | Flicker detection | ADC counts | AS7343 |

## Building and Flashing

### Prerequisites
- PlatformIO (VSCode extension or CLI)
- ESP32-C6 USB drivers installed

### Build Process
1. Open project in VSCode with PlatformIO extension
2. Select environment: `esp32-c6-devkitm-1`
3. Configure the node (modify `globals.h` as described above)
4. Build: Click "Build" button or run `pio run`
5. Upload: Click "Upload" button or run `pio run --target upload`
6. Monitor: Click "Serial Monitor" or run `pio device monitor`

### Quick Build Commands
```bash
cd SW_design/PlatformIO_C6

# Build only (check for errors)
pio run

# Build and upload to connected ESP32
pio run --target upload

# Monitor serial output
pio device monitor --baud 115200

# Build + Upload + Monitor (all in one)
pio run --target upload && pio device monitor
```

## Troubleshooting

### LED Status Codes

| LED State | Meaning | Action |
|-----------|---------|--------|
| Off | No network / Initial connection failed | Check WiFi SSID/password, verify AP is on |
| Flashing (during boot) | Attempting initial connection | Wait up to 30 seconds |
| Flashing (after connected) | Reconnecting after loss | Check AP stability |
| Solid On | Connected and transmitting | Normal operation |

### Common Issues

**1. LED stays off after boot**
- WiFi credentials incorrect
- Access point not powered on
- ESP32 out of range
- Wrong SSID/password in `globals.h`

**2. LED flashing continuously**
- Initial connection succeeded, but now lost
- AP rebooted or became unstable
- ESP32 moved out of range
- Check AP power and coverage

**3. No data received on server**
- Server IP address incorrect in `globals.h`
- Server not listening on UDP port 5000
- Firewall blocking UDP traffic
- Wrong network subnet

**4. Compilation errors**
- Verify all commas in IP address defines (use commas, not dots)
  - Correct: `192,168,1,101`
  - Wrong: `192.168.1.101`

### Debug Output

Enable verbose network debugging in:
```cpp
// SW_design/PlatformIO_C6/src/shared_resources/global_debug.h

#define DEBUG_NETWORK           1  // Enable network state messages
#define DEBUG_NETWORK_PACKETS   1  // Enable packet content logging (very verbose!)
```

Monitor serial output at 115200 baud to see:
- Connection attempts
- State transitions
- Data transmission frequency
- Error messages

## Network Recommendations

### Band Separation (2.4 GHz vs 5 GHz)

As you suggested, it's beneficial to separate traffic:

**2.4 GHz Band (for ESP32 nodes):**
- Longer range
- Better wall penetration
- More reliable for sensors
- ESP32-C6 supports 2.4 GHz

**5 GHz Band (for student devices):**
- Higher bandwidth
- Less interference
- Better for web browsing
- Keeps sensor traffic unaffected

### Access Point Configuration

Recommended AP settings:
- SSID: `PhysicsLab_AP` (or your choice)
- Security: WPA2-PSK or WPA3
- Channel: Auto (or manually select least congested)
- DHCP range: 192.168.1.50-192.168.1.200 (for student devices)
- Reserved IPs: 192.168.1.101-192.168.1.110 (for ESP32 static IPs)
- Server IP: 192.168.1.10 (Raspberry Pi - static, outside DHCP range)

## Performance Characteristics

### Network Task
- Priority: 7 (lowest priority, won't interfere with sensors)
- Stack size: 8192 bytes (larger for WiFi buffers)
- Run rate: 50 Hz (TASK_RATE_NETWORK)
- Data send rate: 50 Hz (same as task rate - sends every iteration)

### Bandwidth Usage
- Approximate packet size: 200-400 bytes (depending on active sensors)
- At 50 Hz: 10-20 KB/s per node
- For 10 nodes: 100-200 KB/s total
- Well within WiFi capacity

### Latency
- UDP provides minimal latency
- Typical round-trip time: 5-20 ms on local network
- No acknowledgment overhead

## Next Steps

Now that the ESP32 networking is implemented, you'll need to:

1. **Raspberry Pi Server** (Python recommended):
   - UDP listener on port 5000
   - JSON parsing
   - Data storage/buffering
   - Web server for dashboard

2. **Web Dashboard** (HTML/JS):
   - Real-time strip charts per node
   - Tab-based navigation (one tab per ESP32)
   - Auto-hide sensors with NaN values
   - WebSocket or Server-Sent Events for live updates

3. **Student Interaction System**:
   - MAC address tracking
   - Name registration on first connection
   - Question/answer system
   - CSV data storage

Would you like me to start on the Raspberry Pi server implementation next?

## File Changes Summary

**New Files Created:**
- `src/network/NetworkTask.h` - Header file with NetworkTask class definition
- `src/network/NetworkTask.cpp` - Implementation of WiFi connectivity and data transmission

**Modified Files:**
- `src/shared_resources/globals.h` - Added network configuration constants
- `src/shared_resources/global_debug.h` - Added network debug flags
- `src/main.cpp` - Integrated NetworkTask into FreeRTOS system

**No Library Dependencies Added:**
- WiFi and WiFiUdp are built-in to ESP32 Arduino framework
- No additional PlatformIO libraries required

## Testing Checklist

Before deploying to the classroom:

- [ ] Configure unique NODE_ID for each ESP32
- [ ] Configure unique static IP for each ESP32
- [ ] Verify AP is powered on and broadcasting
- [ ] Verify Raspberry Pi is on network (192.168.1.10)
- [ ] Test one ESP32 first (LED should go solid)
- [ ] Verify UDP packets received on RPi (use netcat: `nc -ul 5000`)
- [ ] Test with multiple ESP32s simultaneously
- [ ] Verify no packet collisions or data loss
- [ ] Test reconnection behavior (power cycle AP)
- [ ] Verify LED behavior matches specification
- [ ] Test with student devices connected
- [ ] Verify sensor data appears correctly in JSON

## Contact & Support

For questions or issues with this implementation, check:
- Serial monitor output (115200 baud)
- LED status indicators
- Network configuration in `globals.h`
- Debug flags in `global_debug.h`

Happy experimenting! 🔬📡
