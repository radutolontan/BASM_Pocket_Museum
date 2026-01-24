# WebSocket Live Data Fix

## Problem

Live sensor data was not displaying on the dashboard. Symptoms:
- Cards showed `--` or nil values
- Backend received UDP packets (confirmed in terminal logs)
- Dashboard responded to ESP32 on/off
- WebSocket connected successfully
- But NO actual sensor readings appeared

## Root Cause

**Event Name Mismatch:**

The frontend was emitting a `'join'` event but the backend was listening for `'subscribe_device'`.

```javascript
// BEFORE (frontend):
AppState.socket.emit('join', { room: 'device_${device.node_id}' });

// Backend was listening for:
@socketio.on('subscribe_device')
def handle_subscribe_device(data):
    node_id = data.get('node_id')
    # ...
    join_room(f'device_{node_id}')
```

**Result:** The client never actually joined the WebSocket room, so it never received the sensor data broadcasts that were being sent to that room.

## The Fix

**Changed frontend to match backend API:**

```javascript
// AFTER (frontend):
AppState.socket.emit('subscribe_device', { node_id: device.node_id });
```

Now when you select a device:
1. Frontend emits `'subscribe_device'` with `{node_id: "192.168.10.11"}`
2. Backend handler receives this and calls `join_room('device_192.168.10.11')`
3. Backend broadcasts sensor data to `room='device_192.168.10.11'`
4. Frontend receives the sensor_data events
5. Displays update with live values!

## Enhanced Debug Logging

Added comprehensive logging to track data flow:

**WebSocket Events:**
- `📡 Subscribing to device...` - When selectDevice() is called
- `✅ Subscription confirmed for...` - Backend confirms room joined
- `📦 Received sensor_data event for...` - Data arrives from WebSocket

**Data Processing:**
- `🔍 handleSensorData called: node_id=...` - Entry point for sensor data
- `📊 Sensor data keys: temperature, pressure, accel_x...` - Fields in packet
- `📊 Processing sensor data for N active displays` - How many displays to update
- `→ temperature: 23.45` - Extracted value for each measurement
- `⚠️ No value extracted for...` - If field is missing

**Display Updates:**
- `🔄 updateDisplay: display-id, temperature, Numeric Only` - Update triggered

## How to Verify the Fix

### 1. Check WebSocket Subscription

Open the live dashboard and watch the debug console (bottom of page):

```
[HH:MM:SS] 🔌 Connecting to WebSocket...
[HH:MM:SS] ✅ WebSocket connected!
[HH:MM:SS] 📡 Subscribing to device 192.168.10.11...
[HH:MM:SS] ✅ Subscription confirmed for 192.168.10.11
```

If you see `❌ Cannot subscribe - WebSocket not connected`, refresh the page.

### 2. Check Data Reception

After selecting a device and adding a measurement display:

```
[HH:MM:SS] 📦 Received sensor_data event for 192.168.10.11
[HH:MM:SS] 🔍 handleSensorData called: node_id=192.168.10.11, selectedDevice=192.168.10.11
[HH:MM:SS] 📊 Sensor data keys: node_id, timestamp, temperature, pressure, accel_x, accel_y, accel_z...
[HH:MM:SS] 📊 Processing sensor data for 1 active displays
[HH:MM:SS]   → temperature: 23.45
[HH:MM:SS] 🔄 updateDisplay: display-temperature-Numeric Only, temperature, Numeric Only
```

### 3. Verify Live Values

The numeric displays should now show:
- **Actual sensor values** (e.g., "23.45 °C")
- **Timestamp** showing when data was received
- **Values updating** as new UDP packets arrive

### 4. Test Multiple Displays

Add several measurements and verify debug console shows:

```
[HH:MM:SS] 📊 Processing sensor data for 3 active displays
[HH:MM:SS]   → temperature: 23.45
[HH:MM:SS]   → pressure: 101.32
[HH:MM:SS]   → acceleration: {"x":0.12,"y":-0.05,"z":9.81,"norm":9.82}
[HH:MM:SS] 🔄 updateDisplay: display-temperature-Numeric Only...
[HH:MM:SS] 🔄 updateDisplay: display-pressure-Time-graph...
[HH:MM:SS] 🔄 updateDisplay: display-acceleration-Vector...
```

## Troubleshooting

### Still seeing `--` or nil values?

**Check debug console for these messages:**

1. **WebSocket not connected:**
   ```
   ❌ Cannot subscribe - WebSocket not connected
   ```
   **Fix:** Refresh the page

2. **Not receiving sensor_data events:**
   - Check if `📦 Received sensor_data event` appears
   - If not, backend may not be broadcasting
   - Check Flask server logs for UDP packet reception

3. **Receiving data but for wrong device:**
   ```
   ⏭️ Skipping data for 192.168.10.12 (selected: 192.168.10.11)
   ```
   **Fix:** This is normal - you're receiving broadcasts for other devices

4. **Missing sensor fields:**
   ```
   ⚠️ No value extracted for temperature
   ```
   **Fix:** ESP32 not sending that field - check ESP32 JSON packet

5. **No active displays:**
   ```
   📊 Processing sensor data for 0 active displays
   ```
   **Fix:** Add some measurement displays by clicking the measurement cards

### Backend not broadcasting?

Check Flask server logs for:

```
INFO - Received data from 192.168.10.11: {...}
INFO - Stored sensor data from 192.168.10.11
```

If you don't see these, the UDP listener isn't receiving packets. Check:
- ESP32 is sending to correct IP (192.168.10.2) and port (5000)
- ESP32 is connected to the AP
- Use `sudo tcpdump -i wlan0 -n udp port 5000` to monitor UDP traffic

## Testing Procedure

1. **Start backend:**
   ```bash
   cd raspberry_pi_dashboard
   source venv/bin/activate
   python app.py
   ```

2. **Power on ESP32**
   - Wait for WiFi connection
   - Should appear in device grid

3. **Open live dashboard:**
   - Navigate to `http://192.168.10.2:8080/pocket-lab-live.html`
   - Watch debug console for WebSocket connection

4. **Select device:**
   - Click on device card (e.g., 192.168.10.11)
   - Debug console should show subscription confirmation

5. **Add measurement displays:**
   - Click "Temperature → Numeric Only"
   - Click "Pressure → Time-graph"
   - Click "Acceleration → Vector"

6. **Verify live data:**
   - Temperature should show actual °C value
   - Pressure chart should plot real-time data
   - Acceleration vector should move with device motion
   - Debug console shows continuous data processing

## Files Changed

**`raspberry_pi_dashboard/static/js/pocket-lab-live.js`:**
- Line 232-239: Added debug logging for sensor_data events and subscription responses
- Line 444-448: Changed `emit('join', {room: ...})` to `emit('subscribe_device', {node_id: ...})`
- Line 251-292: Enhanced handleSensorData() with comprehensive logging
- Line 1024: Added logging to updateDisplay()

**Backend (no changes needed):**
- `app.py` already had the correct `@socketio.on('subscribe_device')` handler
- `udp_listener.py` already broadcasts to the correct room

---

**Fix Status:** COMPLETE

The WebSocket subscription now works correctly. Live sensor data flows from ESP32 → UDP → Backend → WebSocket → Frontend → Displays.
