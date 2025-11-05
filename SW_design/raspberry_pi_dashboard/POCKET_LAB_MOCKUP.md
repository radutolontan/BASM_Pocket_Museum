# Pocket Lab Data Tab - Mockup

## Quick Access

**URL:** http://localhost:8080/pocket-lab-mockup.html

(Or from network: http://192.168.10.2:8080/pocket-lab-mockup.html)

---

## What This Mockup Demonstrates

This is a **fully interactive mockup** of the Pocket Lab Data tab with **mock data generation**. You can interact with all features without connecting to actual ESP32 devices.

### Flow:

1. **Device Selection (Step 1)**
   - View 4 mock ESP32 devices
   - Click on any device to select it
   - Device cards show: name, ID, IP, active status, last seen

2. **Measurement Selection (Step 2)**
   - After selecting a device, see 8 measurement cards:
     - Temperature
     - Pressure
     - Acceleration (vector)
     - Gyroscope (vector)
     - Magnetometer (vector)
     - Volume
     - Ambient Light
     - Light Spectrum

   - Each card shows available display options as **outline buttons**:
     - **Regular sensors:** Time-graph, Numeric Only, Numeric w. Statistics
     - **Vector quantities:** Time-graph, Numeric Only, Numeric w. Statistics, Vector
     - **Light Spectrum:** Electromagnetic Spectrum only

3. **Data Display (Step 3)**
   - Click any button → it becomes a **primary button** (filled)
   - A new display card appears below
   - Click multiple buttons → multiple cards are created
   - Each card shows live mock data

---

## Display Types Explained

### 🔢 Numeric Only
- Shows the **last received value** in large text
- Updates every second with mock data
- Timestamp shows when last updated

### 📊 Numeric w. Statistics
- Shows **6 statistics**:
  - Current value
  - Average
  - Minimum
  - Maximum
  - Standard Deviation
  - Sample Count
- Rolling statistics (all data since display created)
- **Reset button** to clear statistics window

### 📈 Time-graph
- **10-second scrolling window** (shows last ~50 data points)
- **Auto-scaling Y-axis**
- For vectors (Accel, Gyro, Mag):
  - Shows **all 4 components** on same graph: X, Y, Z, Norm
  - Color-coded with legend
  - Example colors:
    - X: Yellow
    - Y: Blue
    - Z: Pink
    - Norm: Red

### 🎯 Vector (Vector quantities only)
- **2D projection** of 3D vector (X-Y magnitude vs Z)
- Shows scatter plot with origin
- **Component breakdown**:
  - X component
  - Y component
  - Z component
  - Magnitude (norm)
- Updates in real-time

### 🌈 Electromagnetic Spectrum (Light spectrum only)
- **Bar chart** showing intensity at each wavelength
- **12 spectral channels**:
  - F1 (405nm) - Violet/UV
  - F2 (425nm) - Dark Blue
  - F3 (475nm) - Light Blue
  - F4 (515nm) - Blue
  - FZ (450nm) - Blue (alternate)
  - FY (555nm) - Green
  - F5 (550nm) - Green (narrow)
  - F6 (640nm) - Orange
  - FXL (600nm) - Orange (extended)
  - F7 (690nm) - Red
  - F8 (745nm) - Dark Red
  - NIR (855nm) - Near Infrared
- Color-coded bars matching actual spectrum

---

## Interactive Features

### Selecting/Deselecting Displays
- **Click outline button** → becomes primary, adds display
- **Click primary button** → becomes outline, removes display
- **Click X button** on display card → removes display and resets button

### Changing Device
- Click **"Change Device"** button in top banner
- Returns to device selection
- **Clears all active displays**

### Theme Switching
- Click **theme toggle** in top-right
- Switch between light and dark mode
- Preference saved in browser

### Statistics Reset
- Click **"Reset Statistics"** button on stats displays
- Clears all accumulated data
- Starts fresh from next update

---

## Mock Data Behavior

All data is **randomly generated** to simulate real sensors:

- **Temperature:** 23.5°C ± 1.5°C
- **Pressure:** 101.3 kPa ± 0.5 kPa
- **Volume:** -25 dB ± 10 dB
- **Ambient Light:** 542 lux ± 100 lux
- **Acceleration/Gyro/Mag:** Random 3D vectors
- **Spectrum:** Random intensities for each wavelength

**Update rate:** 1 Hz (1 update per second)

---

## Testing Checklist

Please test and provide feedback on:

### Layout & Organization
- [ ] Is the device selection grid clear?
- [ ] Are measurement cards easy to understand?
- [ ] Is the selected device banner prominent enough?
- [ ] Do display cards have good spacing?
- [ ] Does the layout work on different screen sizes?

### Button Behavior
- [ ] Is the outline → primary button transition clear?
- [ ] Are button labels descriptive?
- [ ] Is it obvious which options are selected?

### Display Cards
- [ ] **Numeric Only:** Is the value readable? Font size OK?
- [ ] **Statistics:** Are all 6 stats clearly labeled?
- [ ] **Time-graph:** Is the 10-second window clear? Legend readable?
- [ ] **Vector:** Does the 2D projection make sense? Are components clear?
- [ ] **Spectrum:** Are wavelengths and colors distinguishable?

### User Flow
- [ ] Is it intuitive to select a device?
- [ ] Is it clear how to add displays?
- [ ] Is it easy to remove displays?
- [ ] Is the "Change Device" button findable?
- [ ] Can you easily compare multiple displays?

### Visual Design
- [ ] Do colors match brand (yellow, blue, pink, red)?
- [ ] Is Work Sans font rendering well?
- [ ] Do both light and dark themes look good?
- [ ] Are cards and borders visually appealing?
- [ ] Is there good contrast for readability?

### Performance
- [ ] Are charts updating smoothly?
- [ ] Can you add 10+ displays without lag?
- [ ] Do statistics calculate correctly?
- [ ] Are animations smooth?

---

## Known Limitations (Mockup Only)

- No real ESP32 connection
- Data is randomly generated
- No actual device discovery
- Statistics are in-memory only (reset on page reload)
- Vector display is 2D projection of 3D space

These will be addressed when connecting to real backend.

---

## Providing Feedback

Please review and let me know:

1. **What looks good?**
2. **What needs improvement?**
3. **Are there any confusing elements?**
4. **Should any layouts change?**
5. **Are the display types what you envisioned?**

Specific feedback appreciated:
- "The time-graph needs a bigger legend"
- "Statistics should be in 3 columns instead of 2"
- "Vector display is confusing, needs labels"
- "Love the spectrum colors!"
- etc.

---

## Next Steps After Approval

Once you approve the layout:
1. Connect to real WebSocket backend
2. Integrate with ESP32 UDP data
3. Add user preference persistence (save selections)
4. Implement advanced features (export data, etc.)
5. Optimize performance for Raspberry Pi

---

**Enjoy exploring the mockup!** 🚀
