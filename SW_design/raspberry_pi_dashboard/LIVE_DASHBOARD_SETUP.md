# Live ESP32 Dashboard Setup Guide

This guide explains how to access and use the **live** Pocket Lab dashboard that connects to real ESP32 sensor data.

## Quick Start

### 1. Make Sure Backend is Running

```bash
cd raspberry_pi_dashboard
source venv/bin/activate
python app.py
```

The server should start on port **8080** and display:
```
* Running on http://0.0.0.0:8080
UDP listener started on port 5000
```

### 2. Access the Dashboard

**Mockup (for testing UI):**
- URL: `http://192.168.10.2:8080/pocket-lab-mockup.html`
- Uses simulated data at 1 Hz
- No ESP32 required

**Live Dashboard (real ESP32 data):**
- URL: `http://192.168.10.2:8080/pocket-lab-live.html`
- Connects to real ESP32 devices
- Requires ESP32 sending UDP packets

## ESP32 Setup

### UDP Packet Format

Your ESP32 must send JSON packets to the Raspberry Pi on **UDP port 5000**.

**Example JSON packet:**
```json
{
  "node_id": "ESP32_LAB_01",
  "timestamp": 1234567,
  "temperature": 23.5,
  "pressure": 101.3,
  "light_intensity": 542.0,
  "accel_x": 0.12,
  "accel_y": -0.05,
  "accel_z": 9.81,
  "accel_norm": 9.82,
  "gyro_x": 0.02,
  "gyro_y": -0.01,
  "gyro_z": 0.00,
  "gyro_norm": 0.02,
  "mag_x": 25.3,
  "mag_y": -12.7,
  "mag_z": 48.9,
  "mag_norm": 56.4,
  "volume_rms": -28.5,
  "spectral_f1": 245,
  "spectral_f2": 312,
  "spectral_f3": 589,
  "spectral_f4": 678,
  "spectral_fz": 456,
  "spectral_fy": 892,
  "spectral_f5": 723,
  "spectral_f6": 567,
  "spectral_fxl": 634,
  "spectral_f7": 421,
  "spectral_f8": 289,
  "spectral_nir": 156,
  "spectral_vis": 4567,
  "spectral_fd": 234
}
```

### Required Fields

- `node_id` (string): Unique identifier for the ESP32 (e.g., "ESP32_LAB_01")
- `timestamp` (int): ESP32 millis() timestamp (optional)

### Sensor Fields (all optional - send what you have)

**Environmental:**
- `temperature` (float): Temperature in °C
- `pressure` (float): Pressure in kPa
- `light_intensity` (float): Ambient light in lux

**IMU - Accelerometer:**
- `accel_x`, `accel_y`, `accel_z` (float): m/s²
- `accel_norm` (float): Magnitude

**IMU - Gyroscope:**
- `gyro_x`, `gyro_y`, `gyro_z` (float): deg/s
- `gyro_norm` (float): Magnitude

**IMU - Magnetometer:**
- `mag_x`, `mag_y`, `mag_z` (float): µT
- `mag_norm` (float): Magnitude

**Audio:**
- `volume_rms` (float): RMS volume in dB

**Spectral (AS7343 sensor):**
- `spectral_f1` through `spectral_nir`: 14 spectral channels

### ESP32 Code Example (Arduino)

```cpp
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

const char* ssid = "PocketLabAP";
const char* password = "your_password";
const char* serverIP = "192.168.10.2";  // Raspberry Pi IP
const int serverPort = 5000;

WiFiUDP udp;

void setup() {
    Serial.begin(115200);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected!");
}

void loop() {
    // Create JSON packet
    StaticJsonDocument<512> doc;
    doc["node_id"] = "ESP32_LAB_01";
    doc["timestamp"] = millis();

    // Add sensor readings
    doc["temperature"] = readTemperature();
    doc["pressure"] = readPressure();
    doc["light_intensity"] = readAmbientLight();
    // ... add more sensor data ...

    // Serialize and send
    char buffer[512];
    serializeJson(doc, buffer);

    udp.beginPacket(serverIP, serverPort);
    udp.write((uint8_t*)buffer, strlen(buffer));
    udp.endPacket();

    // Send at 25-50 Hz (20-40ms delay)
    delay(40);
}
```

## Dashboard Features

### Device Selection

1. **Automatic Discovery**: Devices appear automatically when they send data
2. **Active Status**: Shows which devices are currently sending data (< 5s ago)
3. **Last Seen**: Displays time since last data packet

### Measurement Types

Click measurement cards to add visualizations:

1. **Time-graph**: Rolling time-series chart (5s to 5min windows)
2. **Numeric Only**: Current value display
3. **Numeric w. Statistics**: Min, Max, Avg, StdDev with reset
4. **Vector** (for IMU data): 3D visualization with component breakdown
5. **Electromagnetic Spectrum**: Bar chart of 14 spectral channels

### WebSocket Connection

The dashboard uses **Socket.IO** for real-time updates:
- Connects automatically on page load
- Subscribes to sensor_data for selected device
- Updates displays in real-time as data arrives
- Reconnects automatically if connection drops

## Troubleshooting

### "No active devices found"

**Check:**
1. ESP32 is powered on and connected to WiFi
2. ESP32 is sending UDP packets to correct IP (192.168.10.2) and port (5000)
3. Backend server is running
4. Check debug console at bottom of page for errors

**Test UDP reception manually:**
```bash
# On Raspberry Pi
sudo tcpdump -i wlan0 -n udp port 5000
```

You should see packets from your ESP32 IP address.

### "WebSocket connection error"

**Check:**
1. Backend server is running (Flask + Socket.IO)
2. No firewall blocking port 8080
3. Browser console (F12) for detailed errors

### Charts not updating

**Check:**
1. Debug console shows "Received data from [node_id]"
2. Selected device matches the node_id in ESP32 packets
3. Sensor fields in ESP32 match expected field names
4. Check browser console for JavaScript errors

### ESP32 not connecting to WiFi

**Check:**
1. SSID and password are correct
2. Raspberry Pi AP is running
3. ESP32 is in range
4. Check ESP32 serial monitor for connection status

## Advanced Configuration

### Change UDP Port

Edit `config.py`:
```python
UDP_PORT = 5000  # Change to your port
```

Then restart backend and update ESP32 code.

### Adjust Activity Timeout

Edit `config.py`:
```python
ACTIVITY_TIMEOUT_SECONDS = 5  # Devices inactive after 5s
```

### Custom Node IDs

Use meaningful node IDs in ESP32 code:
```cpp
doc["node_id"] = "CLASSROOM_A_SENSOR_1";
```

The dashboard will display this as the device name.

## Data Flow Diagram

```
ESP32 Sensor
    |
    | UDP JSON packets (25-50 Hz)
    | Port 5000
    v
Raspberry Pi (app.py)
    |
    ├──> SQLite Database (historical data)
    |
    └──> Socket.IO Broadcast
            |
            | WebSocket
            | Real-time updates
            v
        Web Browser
        (pocket-lab-live.html)
            |
            └──> Live Charts & Displays
```

## Database

All sensor data is stored in SQLite at:
```
raspberry_pi_dashboard/instance/dashboard.db
```

**Retention:** Data older than 24 hours is automatically cleaned up.

**Query data:**
```bash
cd raspberry_pi_dashboard
sqlite3 instance/dashboard.db

SELECT * FROM sensor_data WHERE node_id = 'ESP32_LAB_01' ORDER BY timestamp DESC LIMIT 10;
```

## API Endpoints

### GET /api/devices
Returns list of active devices.

**Example:**
```bash
curl http://192.168.10.2:8080/api/devices
```

**Response:**
```json
[
  {
    "node_id": "ESP32_LAB_01",
    "hostname": "ESP32_LAB_01",
    "ip_address": "192.168.10.11",
    "last_seen": "2025-11-02T14:30:45.123456",
    "is_active": true
  }
]
```

### GET /api/devices/<node_id>/sensor-data
Returns recent sensor data for a specific device.

**Parameters:**
- `limit`: Number of records (default: 100)
- `hours`: Time window in hours (default: 1)

**Example:**
```bash
curl "http://192.168.10.2:8080/api/devices/ESP32_LAB_01/sensor-data?limit=10"
```

## Performance

- **UDP Packet Rate:** 25-50 Hz (ESP32)
- **WebSocket Broadcast:** Real-time (< 10ms latency)
- **Chart Update Rate:** 1 Hz (every second)
- **Database Writes:** Every packet
- **Frontend:** No significant CPU load with modern browser

## Support

For issues or questions:
1. Check debug console at bottom of page
2. Check browser console (F12 → Console tab)
3. Check Flask server logs
4. Verify ESP32 serial monitor output

---

**Developed by the Bucharest Applied STEAM Museum**
