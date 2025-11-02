# ESP32 JSON Field Mapping to Dashboard

This document shows how ESP32 sensor data fields map to the dashboard measurement displays.

## Data Flow

```
ESP32 Sensor
    ↓ UDP JSON Packet (port 5000)
Backend (udp_listener.py)
    ↓ Store in Database
    ↓ WebSocket Broadcast (Socket.IO)
Frontend (pocket-lab-live.js)
    ↓ handleSensorData()
    ↓ extractSensorValue()
    ↓ updateDisplay()
Live Dashboard Display
```

## Field Mappings

All mappings are implemented in `extractSensorValue()` function in `pocket-lab-live.js`.

### 1. Temperature
**JSON Field:** `temperature`
**Measurement Card:** Temperature
**Unit:** °C
**Color:** RED (#bd2026)
**Code:**
```javascript
case 'temperature':
    return data.temperature;
```

### 2. Pressure
**JSON Field:** `pressure`
**Measurement Card:** Pressure
**Unit:** kPa
**Color:** BLUE (#375f83)
**Code:**
```javascript
case 'pressure':
    return data.pressure;
```

### 3. Ambient Light
**JSON Field:** `light_intensity`
**Measurement Card:** Ambient Light
**Unit:** lux
**Color:** AMBER/GOLD (#fbbf24)
**Code:**
```javascript
case 'ambientLight':
    return data.light_intensity;
```

### 4. Acceleration (Vector)
**JSON Fields:** `accel_x`, `accel_y`, `accel_z`, `accel_norm`
**Measurement Card:** Acceleration
**Unit:** m/s²
**Color:** GREEN (#10b981)
**Display Types:**
- Time-graph: Shows all 4 components (X, Y, Z, Norm)
- Numeric Only: User selects component (dropdown)
- Numeric w. Statistics: User selects component (dropdown)
- Vector: 3D visualization with all components

**Code:**
```javascript
case 'acceleration':
    return {
        x: data.accel_x || 0,
        y: data.accel_y || 0,
        z: data.accel_z || 0,
        norm: data.accel_norm || 0
    };
```

### 5. Gyroscope (Vector)
**JSON Fields:** `gyro_x`, `gyro_y`, `gyro_z`, `gyro_norm`
**Measurement Card:** Gyroscope
**Unit:** deg/s
**Color:** ORANGE (#f97316)
**Display Types:** Same as Acceleration

**Code:**
```javascript
case 'gyro':
    return {
        x: data.gyro_x || 0,
        y: data.gyro_y || 0,
        z: data.gyro_z || 0,
        norm: data.gyro_norm || 0
    };
```

### 6. Magnetometer (Vector)
**JSON Fields:** `mag_x`, `mag_y`, `mag_z`, `mag_norm`
**Measurement Card:** Magnetometer
**Unit:** µT
**Color:** YELLOW (#f8c01c)
**Display Types:** Same as Acceleration

**Code:**
```javascript
case 'magnetometer':
    return {
        x: data.mag_x || 0,
        y: data.mag_y || 0,
        z: data.mag_z || 0,
        norm: data.mag_norm || 0
    };
```

### 7. Volume
**JSON Field:** `volume_rms`
**Measurement Card:** Volume
**Unit:** dB
**Color:** PINK/MAGENTA (#d782a0)
**Code:**
```javascript
case 'volume':
    return data.volume_rms;
```

### 8. Light Spectrum (Optional)
**JSON Fields:** `spectral_f1_405nm`, `spectral_f2_425nm`, ..., `spectral_nir_855nm`
**Measurement Card:** Light Spectrum (only shows if data present)
**Unit:** Intensity (raw values)
**Color:** PURPLE (#8b5cf6)
**Note:** This card auto-hides if no spectral data is sent

**Code:**
```javascript
case 'spectrum':
    return {
        wavelengths: [405, 425, 475, 515, 450, 555, 550, 640, 600, 690, 745, 855],
        names: ['F1', 'F2', 'F3', 'F4', 'FZ', 'FY', 'F5', 'F6', 'FXL', 'F7', 'F8', 'NIR'],
        values: [
            data.spectral_f1_405nm || 0,
            data.spectral_f2_425nm || 0,
            data.spectral_f3_475nm || 0,
            data.spectral_f4_515nm || 0,
            data.spectral_fz_450nm || 0,
            data.spectral_fy_555nm || 0,
            data.spectral_f5_550nm || 0,
            data.spectral_f6_640nm || 0,
            data.spectral_fxl_600nm || 0,
            data.spectral_f7_690nm || 0,
            data.spectral_f8_745nm || 0,
            data.spectral_nir_855nm || 0
        ]
    };
```

## Color Usage

### Numeric Displays
The large numeric value now displays in the measurement's specific color:
- Temperature: RED text
- Pressure: BLUE text
- Acceleration: GREEN text
- Gyroscope: ORANGE text
- Magnetometer: YELLOW text
- Volume: PINK text
- Ambient Light: AMBER/GOLD text

### Cards
- **Card Border:** Measurement color
- **Icon Background:** Measurement color at 20% opacity
- **Icon:** Measurement color
- **Display Badge:** Measurement color background

## Sensor Detection

The dashboard automatically detects which sensors are available:

1. **Initially:** Shows all measurement cards
2. **After first packet:** Hides cards for sensors without data
3. **Checking logic:** `hasSensorData()` function checks if fields exist in JSON

Example: If your ESP32 doesn't send `spectral_f1_405nm`, the Light Spectrum card will automatically disappear.

## Display Types

### Time-graph
- Rolling window (5s to 5min configurable)
- For vectors: plots all 4 components (X, Y, Z, Norm) on same graph
- For scalars: single line
- Updates in real-time as data arrives

### Numeric Only
- Shows current value only
- For vectors: dropdown to select X/Y/Z/Norm
- Color matches measurement type
- Shows timestamp

### Numeric w. Statistics
- Current value (colored), Average, Min, Max, Std Deviation, Sample Count
- For vectors: dropdown to select component (auto-resets stats when changed)
- Reset button to clear statistics
- Cumulative statistics since display opened or reset

### Vector (IMU only)
- 3D isometric visualization
- Shows X (yellow), Y (blue), Z (pink) axes
- Red vector arrow with magnitude
- Component breakdown below visualization

### Electromagnetic Spectrum (AS7343 only)
- Bar chart with 12 channels
- Rainbow gradient colors
- Wavelength labels (405nm - 855nm)
- Only appears if spectral data is sent

## Troubleshooting

### Data not updating
1. Check debug console for "📊 Processing sensor data from..." messages
2. Verify ESP32 is sending all required fields in JSON
3. Check field names match exactly (case-sensitive)
4. Open browser console (F12) for JavaScript errors

### Measurement card not showing
1. Card auto-hides if no data is sent for that sensor
2. Check ESP32 JSON includes the required field
3. Wait for first data packet after selecting device
4. Check debug console for "⏭️ Skipping..." messages

### Wrong values displayed
1. Verify ESP32 sends correct units (°C, kPa, m/s², deg/s, µT, dB, lux)
2. Check for NaN or null values in JSON
3. Vector fields must all be present (x, y, z, norm)
4. Numeric displays show 2 decimal places

## Example ESP32 JSON Packet

```json
{
  "node_id": "LAB_01",
  "timestamp": 1234567890,
  "temperature": 23.45,
  "pressure": 101.32,
  "light_intensity": 542.18,
  "accel_x": 0.123,
  "accel_y": -0.056,
  "accel_z": 9.812,
  "accel_norm": 9.813,
  "gyro_x": 0.023,
  "gyro_y": -0.011,
  "gyro_z": 0.002,
  "gyro_norm": 0.026,
  "mag_x": 25.34,
  "mag_y": -12.78,
  "mag_z": 48.92,
  "mag_norm": 56.43,
  "volume_rms": -28.5
}
```

**Note:** Spectral fields omitted = Light Spectrum card automatically hidden.

---

**All connections are live and working!** Simply select a device, pick measurements, and watch real-time ESP32 data flow into the displays.
