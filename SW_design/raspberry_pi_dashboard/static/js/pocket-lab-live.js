/**
 * Pocket Lab Data LIVE - JavaScript
 * Connects to real ESP32 sensor data via WebSocket
 */

// ============================================
// Global State
// ============================================
const AppState = {
    selectedDevice: null,
    activeDisplays: [],
    charts: {},
    updateIntervals: {},
    statsWindows: {},
    timeScales: {}, // Store time scale for each time-graph
    vectorComponents: {}, // Store selected component for vector numeric displays
    socket: null, // WebSocket connection
    devices: [], // Active devices from API
    latestData: {}, // Latest sensor data per device
    updateRates: {}, // Track update rate (Hz) for each display: { displayId: { count, startTime, lastUpdate, hz } }
    debugLogCounter: 0, // Counter for rate-limited logging
    lastDebugLog: 0, // Timestamp of last debug log
    lastRenderTime: {}, // Track last render time per display for throttling
    renderThrottle: 75, // Minimum ms between renders (13 Hz) - prevents UI freezing
    cachedTimestamp: '', // Cached formatted timestamp string
    timestampCacheTime: 0 // When the timestamp was last updated
};

// Get cached timestamp (updates max 1x per second to avoid expensive toLocaleTimeString calls)
function getCachedTimestamp() {
    const now = Date.now();
    if (now - AppState.timestampCacheTime > 1000) {
        AppState.cachedTimestamp = new Date().toLocaleTimeString();
        AppState.timestampCacheTime = now;
    }
    return AppState.cachedTimestamp;
}

// Format number to specified significant figures
function toSignificantFigures(num, sigFigs) {
    if (num === 0) return '0';
    const magnitude = Math.floor(Math.log10(Math.abs(num)));
    const decimals = Math.max(0, sigFigs - magnitude - 1);
    return num.toFixed(decimals);
}

// Custom SVG Icons
const MEASUREMENT_ICONS = {
    temperature: `<svg viewBox="0 0 24 24" fill="currentColor" width="32" height="32">
        <path d="M14 4c0-1.1-.9-2-2-2s-2 .9-2 2v8.5c-1.2.7-2 2-2 3.5 0 2.2 1.8 4 4 4s4-1.8 4-4c0-1.5-.8-2.8-2-3.5V4z"/>
        <circle cx="12" cy="16" r="1.5" fill="currentColor"/>
    </svg>`,
    pressure: `<svg viewBox="0 0 24 24" fill="currentColor" width="32" height="32">
        <circle cx="12" cy="12" r="9" stroke="currentColor" stroke-width="1.5" fill="none"/>
        <circle cx="12" cy="12" r="1" fill="currentColor"/>
        <line x1="12" y1="12" x2="16" y2="8" stroke="currentColor" stroke-width="1.5"/>
    </svg>`,
    acceleration: `<svg viewBox="0 0 24 24" fill="currentColor" width="32" height="32">
        <path d="M4 12h16M4 12l3-3M4 12l3 3M20 12l-3-3M20 12l-3 3" stroke="currentColor" stroke-width="2" fill="none"/>
    </svg>`,
    gyro: `<svg viewBox="0 0 24 24" fill="currentColor" width="32" height="32">
        <path d="M12 4 L12 7 M12 17 L12 20 M20 12 L17 12 M7 12 L4 12" stroke="currentColor" stroke-width="2"/>
        <path d="M12 12 m -6 0 a 6 6 0 1 1 0 0.1" stroke="currentColor" stroke-width="2" fill="none"/>
        <polygon points="18,12 15,9 15,15" fill="currentColor"/>
    </svg>`,
    magnetometer: `<svg viewBox="0 0 24 24" fill="currentColor" width="32" height="32">
        <path d="M6 8 Q6 4 12 4 Q18 4 18 8 L18 16 L15 16 L15 8 Q15 6 12 6 Q9 6 9 8 L9 16 L6 16 Z" stroke="currentColor" stroke-width="1.5" fill="none"/>
        <text x="8" y="14" font-size="6" fill="currentColor">N</text>
        <text x="15" y="14" font-size="6" fill="currentColor">S</text>
    </svg>`,
    volume: `<svg viewBox="0 0 24 24" fill="currentColor" width="32" height="32">
        <path d="M8 6 L4 10 L4 14 L8 18 L8 6 Z M8 12 L12 6 L12 18 L8 12 Z" fill="currentColor"/>
        <path d="M14 9 Q16 10 16 12 Q16 14 14 15" stroke="currentColor" stroke-width="1.5" fill="none"/>
        <path d="M16 7 Q19 9 19 12 Q19 15 16 17" stroke="currentColor" stroke-width="1.5" fill="none"/>
    </svg>`,
    ambientLight: `<svg viewBox="0 0 24 24" fill="currentColor" width="32" height="32">
        <circle cx="12" cy="12" r="4" fill="currentColor"/>
        <path d="M12 2 L12 4 M12 20 L12 22 M20 12 L22 12 M2 12 L4 12 M17.66 6.34 L19.07 4.93 M4.93 19.07 L6.34 17.66 M17.66 17.66 L19.07 19.07 M4.93 4.93 L6.34 6.34" stroke="currentColor" stroke-width="2"/>
    </svg>`,
    spectrum: `<svg viewBox="0 0 24 24" fill="currentColor" width="32" height="32">
        <defs>
            <linearGradient id="rainbow" x1="0%" y1="0%" x2="100%" y2="0%">
                <stop offset="0%" style="stop-color:#8b00ff"/>
                <stop offset="25%" style="stop-color:#0000ff"/>
                <stop offset="50%" style="stop-color:#00ff00"/>
                <stop offset="75%" style="stop-color:#ffff00"/>
                <stop offset="100%" style="stop-color:#ff0000"/>
            </linearGradient>
        </defs>
        <path d="M4 12 Q8 6 12 12 Q16 18 20 12" stroke="url(#rainbow)" stroke-width="3" fill="none"/>
    </svg>`,
    uvSpectrum: `<svg viewBox="0 0 24 24" fill="currentColor" width="32" height="32">
        <defs>
            <linearGradient id="uvGradient" x1="0%" y1="0%" x2="100%" y2="0%">
                <stop offset="0%" style="stop-color:#4c1d95"/>
                <stop offset="50%" style="stop-color:#7c3aed"/>
                <stop offset="100%" style="stop-color:#a855f7"/>
            </linearGradient>
        </defs>
        <circle cx="12" cy="12" r="5" fill="url(#uvGradient)"/>
        <path d="M12 2 L12 6 M12 18 L12 22 M20 12 L16 12 M2 12 L6 12 M17.66 6.34 L15.24 8.76 M8.76 15.24 L6.34 17.66 M17.66 17.66 L15.24 15.24 M8.76 8.76 L6.34 6.34" stroke="currentColor" stroke-width="2"/>
    </svg>`,
    thermal: `<svg viewBox="0 0 24 24" fill="currentColor" width="32" height="32">
        <defs>
            <linearGradient id="thermalGradient" x1="0%" y1="0%" x2="0%" y2="100%">
                <stop offset="0%" style="stop-color:#ef4444"/>
                <stop offset="50%" style="stop-color:#f59e0b"/>
                <stop offset="100%" style="stop-color:#3b82f6"/>
            </linearGradient>
        </defs>
        <rect x="4" y="4" width="16" height="16" rx="2" fill="url(#thermalGradient)" stroke="currentColor" stroke-width="1.5"/>
        <path d="M7 7 h2 M7 10 h2 M7 13 h2 M7 16 h2 M11 7 h2 M11 10 h2 M11 13 h2 M11 16 h2 M15 7 h2 M15 10 h2 M15 13 h2 M15 16 h2" stroke="white" stroke-width="0.5" opacity="0.5"/>
    </svg>`
};

// Measurement Colors
const MEASUREMENT_COLORS = {
    temperature: '#bd2026',      // RED
    pressure: '#375f83',         // BLUE
    acceleration: '#10b981',     // GREEN
    gyro: '#f97316',            // ORANGE
    magnetometer: '#f8c01c',    // YELLOW
    volume: '#d782a0',          // PINK/MAGENTA
    ambientLight: '#ffffff',    // WHITE (represents light)
    spectrum: '#8b5cf6',        // PURPLE (for spectrum)
    uvSpectrum: '#a855f7',      // PURPLE (for UV spectrum)
    thermal: '#ef4444'          // RED (for thermal)
};

// Measurement Definitions
const MEASUREMENTS = {
    temperature: {
        name: 'Temperature',
        unit: '°C',
        options: ['Time-graph', 'Numeric Only', 'Numeric w. Statistics'],
        baseValue: 23.5,
        variation: 1.5
    },
    pressure: {
        name: 'Pressure',
        unit: 'kPa',
        options: ['Time-graph', 'Numeric Only', 'Numeric w. Statistics'],
        baseValue: 101.3,
        variation: 0.5
    },
    acceleration: {
        name: 'Acceleration',
        unit: 'm/s²',
        options: ['Time-graph', 'Numeric Only', 'Numeric w. Statistics', 'Vector'],
        isVector: true,
        components: ['x', 'y', 'z', 'norm']
    },
    gyro: {
        name: 'Gyroscope',
        unit: 'deg/s',
        options: ['Time-graph', 'Numeric Only', 'Numeric w. Statistics', 'Vector'],
        isVector: true,
        components: ['x', 'y', 'z', 'norm']
    },
    magnetometer: {
        name: 'Magnetometer',
        unit: 'µT',
        options: ['Time-graph', 'Numeric Only', 'Numeric w. Statistics', 'Vector'],
        isVector: true,
        components: ['x', 'y', 'z', 'norm']
    },
    volume: {
        name: 'Volume',
        unit: 'dB',
        options: ['Time-graph', 'Numeric Only', 'Numeric w. Statistics'],
        baseValue: -25,
        variation: 10
    },
    ambientLight: {
        name: 'Ambient Light',
        unit: 'lux',
        options: ['Time-graph', 'Numeric Only', 'Numeric w. Statistics'],
        baseValue: 542,
        variation: 100
    },
    spectrum: {
        name: 'Electromagnetic Spectrum',
        unit: '',
        options: ['UV Spectrum', 'Visible Spectrum', 'Full Spectrum']
    },
    thermal: {
        name: 'Thermal Camera',
        unit: '°C',
        options: ['Thermal Matrix']
    }
};

// Time scale options (in seconds)
const TIME_SCALES = [5, 10, 30, 60, 120, 300];

// ============================================
// Initialization
// ============================================
document.addEventListener('DOMContentLoaded', () => {
    debugLog('🚀 Pocket Lab LIVE initializing...', 'info');

    // Check if Chart.js is loaded
    if (typeof Chart !== 'undefined') {
        debugLog('✅ Chart.js loaded successfully', 'success');
    } else {
        debugLog('❌ ERROR: Chart.js not loaded! Charts will not work.', 'error');
    }

    // Check if Socket.IO is loaded
    if (typeof io !== 'undefined') {
        debugLog('✅ Socket.IO loaded successfully', 'success');
    } else {
        debugLog('❌ ERROR: Socket.IO not loaded! Live data will not work.', 'error');
        return;
    }

    initTheme();
    setupEventListeners();
    connectWebSocket();
    fetchActiveDevices();

    // Refresh device list periodically at typical dashboard rate (5 seconds)
    // This catches new devices joining the network
    setInterval(fetchActiveDevices, 5000);

    debugLog('✅ Pocket Lab LIVE loaded successfully', 'success');
    debugLog('📊 Device list refreshes every 5 seconds', 'info');
    debugLog('⚡ Active display cards update at real-time sensor data rate (no throttling)', 'info');
});

function initTheme() {
    const savedTheme = localStorage.getItem('theme') || 'light';
    document.body.setAttribute('data-theme', savedTheme);
    updateThemeButton(savedTheme);
}

function setupEventListeners() {
    document.getElementById('themeToggle').addEventListener('click', toggleTheme);
    document.getElementById('changeDeviceBtn').addEventListener('click', showDeviceSelection);
}

// ============================================
// Theme Management
// ============================================
function toggleTheme() {
    const currentTheme = document.body.getAttribute('data-theme');
    const newTheme = currentTheme === 'light' ? 'dark' : 'light';
    document.body.setAttribute('data-theme', newTheme);
    localStorage.setItem('theme', newTheme);
    updateThemeButton(newTheme);
}

function updateThemeButton(theme) {
    const btn = document.getElementById('themeToggle');
    const icon = btn.querySelector('i');
    const text = btn.querySelector('span');

    if (theme === 'dark') {
        icon.className = 'bi bi-sun-fill';
        text.textContent = 'Light Theme';
    } else {
        icon.className = 'bi bi-moon-fill';
        text.textContent = 'Dark Theme';
    }
}

// ============================================
// WebSocket Connection
// ============================================
function connectWebSocket() {
    debugLog('🔌 Connecting to WebSocket...', 'info');

    // Connect to Socket.IO server
    AppState.socket = io();

    // Connection successful
    AppState.socket.on('connect', () => {
        debugLog('✅ WebSocket connected!', 'success');
    });

    // Connection error
    AppState.socket.on('connect_error', (error) => {
        debugLog(`❌ WebSocket connection error: ${error.message}`, 'error');
    });

    // Disconnection
    AppState.socket.on('disconnect', (reason) => {
        debugLog(`⚠️ WebSocket disconnected: ${reason}`, 'warning');
    });

    // Listen for sensor data (rate-limited logging to avoid browser slowdown)
    AppState.socket.on('sensor_data', (payload) => {
        // Only log every 25th message (once per second at 25 Hz) to avoid overwhelming browser
        AppState.debugLogCounter++;
        const shouldLog = AppState.debugLogCounter % 25 === 0;

        if (shouldLog) {
            debugLog(`📦 Received sensor_data [counter: ${AppState.debugLogCounter}] for ${payload.node_id}`, 'info');
            debugLog(`📊 Sample data: temp=${payload.data.temperature}°C, pressure=${payload.data.pressure}kPa, accel=${payload.data.accel_norm}`, 'info');
        }

        handleSensorData(payload.node_id, payload.data);
    });

    // Listen for subscription confirmation
    AppState.socket.on('subscription_response', (payload) => {
        debugLog(`✅ Subscription confirmed for ${payload.node_id}`, 'success');
    });

    // Listen for device status updates
    AppState.socket.on('device_status', (payload) => {
        updateDeviceStatus(payload.node_id, payload.status, payload.last_seen);
    });
}

// ============================================
// WebSocket Data Handlers
// ============================================
// IMPORTANT: No throttling or rate limiting on sensor data updates
// Display cards update at the FULL rate of incoming WebSocket data (e.g., 25 Hz)
function handleSensorData(node_id, data) {
    // Only process if this is the selected device
    if (!AppState.selectedDevice || AppState.selectedDevice.node_id !== node_id) {
        return;
    }

    // Check if this is the first data packet
    const isFirstPacket = !AppState.latestData[node_id];

    // Store latest data
    AppState.latestData[node_id] = data;

    // After first packet, refresh measurement grid to hide unavailable sensors
    if (isFirstPacket) {
        debugLog(`📊 First data packet received from ${node_id}, refreshing measurement grid`, 'info');
        renderMeasurementGrid();
        return; // Don't update displays yet, they don't exist
    }

    // Only log when there are active displays (to reduce console spam)
    if (AppState.activeDisplays.length > 0 && AppState.debugLogCounter % 25 === 0) {
        debugLog(`📊 Updating ${AppState.activeDisplays.length} active display(s)`, 'info');
    }

    // Update all active displays with new data
    AppState.activeDisplays.forEach(display => {
        const measurementKey = display.measurementKey;
        const optionType = display.optionType;

        // Extract value based on measurement type
        let value = extractSensorValue(measurementKey, data);

        if (value !== null && value !== undefined) {
            updateDisplay(display.id, measurementKey, optionType, value);
        }
    });
}

function extractSensorValue(measurementKey, data) {
    // Map measurement keys to sensor data fields
    switch (measurementKey) {
        case 'temperature':
            return data.temperature;

        case 'pressure':
            return data.pressure;

        case 'acceleration':
            // Scale by 1/100 and return
            return {
                x: (data.accel_x || 0) / 100,
                y: (data.accel_y || 0) / 100,
                z: (data.accel_z || 0) / 100,
                norm: (data.accel_norm || 0) / 100
            };

        case 'gyro':
            return {
                x: data.gyro_x || 0,
                y: data.gyro_y || 0,
                z: data.gyro_z || 0,
                norm: data.gyro_norm || 0
            };

        case 'magnetometer':
            return {
                x: data.mag_x || 0,
                y: data.mag_y || 0,
                z: data.mag_z || 0,
                norm: data.mag_norm || 0
            };

        case 'volume':
            return data.volume_rms;

        case 'ambientLight':
            return data.light_intensity;

        case 'spectrum':
            // Return ALL available spectral data (UV + visible)
            const channels = [];

            // DEBUG: Log UV data from incoming WebSocket message
            console.log(`🔬 extractSensorValue - UV data from WebSocket:`,
                `UVA=${data.spectral_UVA}, UVB=${data.spectral_UVB}, UVC=${data.spectral_UVC}`);

            // Add UV channels if available
            if (data.spectral_UVC !== undefined || data.spectral_UVB !== undefined || data.spectral_UVA !== undefined) {
                channels.push(
                    { name: 'UVC', start: 200, end: 280, color: '#4c1d95', value: data.spectral_UVC || 0 },
                    { name: 'UVB', start: 280, end: 320, color: '#7c3aed', value: data.spectral_UVB || 0 },
                    { name: 'UVA', start: 320, end: 400, color: '#a855f7', value: data.spectral_UVA || 0 }
                );
                console.log(`  ✅ Added UV channels to extraction:`, channels.slice(0, 3).map(c => `${c.name}=${c.value}`).join(', '));
            } else {
                console.log(`  ⚠️ No UV data available in this packet`);
            }

            // Add visible channels if available
            if (data.spectral_f1_405nm !== undefined) {
                channels.push(
                    { name: 'F1', start: 395, end: 415, color: '#8200c8', value: data.spectral_f1_405nm || 0 },
                    { name: 'F2', start: 415, end: 435, color: '#5400ff', value: data.spectral_f2_425nm || 0 },
                    { name: 'FZ', start: 440, end: 460, color: '#0046ff', value: data.spectral_fz_450nm || 0 },
                    { name: 'F3', start: 465, end: 485, color: '#00c0ff', value: data.spectral_f3_475nm || 0 },
                    { name: 'F4', start: 505, end: 525, color: '#1fff00', value: data.spectral_f4_515nm || 0 },
                    { name: 'F5', start: 540, end: 560, color: '#a3ff00', value: data.spectral_f5_550nm || 0 },
                    { name: 'FXL', start: 590, end: 610, color: '#ffbe00', value: data.spectral_fxl_600nm || 0 },
                    { name: 'F6', start: 630, end: 650, color: '#ff2100', value: data.spectral_f6_640nm || 0 },
                    { name: 'F7', start: 680, end: 700, color: '#ff0000', value: data.spectral_f7_690nm || 0 },
                    { name: 'F8', start: 735, end: 755, color: '#ab0000', value: data.spectral_f8_745nm || 0 },
                    { name: 'NIR', start: 845, end: 865, color: '#610000', value: data.spectral_nir_855nm || 0 }
                );
            }

            return { channels };

        case 'thermal':
            // Return thermal camera data (8x8 pixel array)
            return data.thermal_pixels || null;

        default:
            return null;
    }
}

function updateDeviceStatus(node_id, status, last_seen) {
    // Update device in list if currently displayed
    const device = AppState.devices.find(d => d.node_id === node_id);
    if (device) {
        device.is_active = (status === 'active');
        device.last_seen = last_seen;
    }

    // DON'T refresh device grid constantly - causes flickering
    // Only refresh when user is actively viewing the device selection
    // and only if device wasn't in the list before (new device)
}

// ============================================
// API Functions
// ============================================
function fetchActiveDevices() {
    debugLog('📡 Fetching active devices from API...', 'info');

    fetch('/api/devices')
        .then(response => response.json())
        .then(data => {
            // API returns { "devices": [...] }
            AppState.devices = data.devices || [];
            debugLog(`✅ Found ${AppState.devices.length} active device(s)`, 'success');
            renderDeviceGrid();
        })
        .catch(error => {
            debugLog(`❌ Error fetching devices: ${error.message}`, 'error');
            AppState.devices = [];
            renderDeviceGrid();
        });
}

// ============================================
// Device Selection
// ============================================
function renderDeviceGrid() {
    const grid = document.getElementById('deviceGrid');
    grid.innerHTML = '';

    if (AppState.devices.length === 0) {
        grid.innerHTML = '<p style="text-align: center; padding: 40px; color: var(--text-secondary); font-size: 1.1rem;">⏳ No active devices found.<br><br>Make sure your ESP32 is powered on and sending data to this server.</p>';
        return;
    }

    debugLog(`📊 Rendering ${AppState.devices.length} device(s)...`, 'info');

    AppState.devices.forEach(device => {
        const card = document.createElement('div');
        card.className = 'device-card';

        // Format last seen time
        const lastSeenDate = new Date(device.last_seen);
        const now = new Date();
        const secondsAgo = Math.floor((now - lastSeenDate) / 1000);
        let lastSeenText = '';
        if (secondsAgo < 60) {
            lastSeenText = `${secondsAgo}s ago`;
        } else if (secondsAgo < 3600) {
            lastSeenText = `${Math.floor(secondsAgo / 60)}m ago`;
        } else {
            lastSeenText = `${Math.floor(secondsAgo / 3600)}h ago`;
        }

        card.innerHTML = `
            <div class="device-status-badge">
                <span class="status-indicator"></span>
                ${device.is_active ? 'Active' : 'Inactive'}
            </div>
            <div class="device-icon">
                <img src="/images/pocket-lab-icon.png" alt="Pocket Lab" style="width: 64px; height: 64px; object-fit: contain;" />
            </div>
            <h3 style="font-size: 1.5rem;">${device.hostname || device.node_id}</h3>
            <div class="device-meta">
                <span>${device.ip_address || 'Unknown IP'}</span>
                <span>Last seen: ${lastSeenText}</span>
            </div>
        `;
        card.addEventListener('click', () => selectDevice(device));
        grid.appendChild(card);
    });
}

function selectDevice(device) {
    AppState.selectedDevice = device;

    // Update banner - show device ID only (once, in big letters)
    document.getElementById('selectedDeviceName').textContent = device.node_id;

    // Subscribe to WebSocket room for this device
    if (AppState.socket && AppState.socket.connected) {
        debugLog(`📡 Subscribing to device ${device.node_id}...`, 'info');
        AppState.socket.emit('subscribe_device', { node_id: device.node_id });
    } else {
        debugLog(`❌ Cannot subscribe - WebSocket not connected`, 'error');
    }

    // Clear any active displays
    clearAllDisplays();

    // Switch views
    document.getElementById('deviceSelectionView').classList.remove('active');
    document.getElementById('measurementView').classList.add('active');

    // Render measurement cards
    renderMeasurementGrid();
}

function showDeviceSelection() {
    // Clear displays
    clearAllDisplays();

    // Switch views
    document.getElementById('measurementView').classList.remove('active');
    document.getElementById('deviceSelectionView').classList.add('active');

    AppState.selectedDevice = null;
}

// ============================================
// Measurement Selection
// ============================================
function hasSensorData(measurementKey, data) {
    // Check if sensor data is available for this measurement type
    // Returns true if we've received at least one packet with this sensor's data
    switch (measurementKey) {
        case 'temperature':
        case 'pressure':
            return data.temperature !== undefined || data.pressure !== undefined;
        case 'ambientLight':
            return data.light_intensity !== undefined;
        case 'acceleration':
            return data.accel_x !== undefined || data.accel_norm !== undefined;
        case 'gyro':
            return data.gyro_x !== undefined || data.gyro_norm !== undefined;
        case 'magnetometer':
            return data.mag_x !== undefined || data.mag_norm !== undefined;
        case 'volume':
            return data.volume_rms !== undefined;
        case 'spectrum':
            // Show spectrum card if ANY spectral data is available (UV or visible)
            return data.spectral_f1_405nm !== undefined ||
                   data.spectral_UVA !== undefined ||
                   data.spectral_UVB !== undefined ||
                   data.spectral_UVC !== undefined;
        case 'thermal':
            return data.thermal_pixels !== undefined && data.thermal_pixels !== null;
        default:
            return true; // Show by default if unknown
    }
}

function renderMeasurementGrid() {
    const grid = document.getElementById('measurementGrid');
    grid.innerHTML = '';
    debugLog('📊 Rendering measurement grid...', 'info');

    // Get latest data for selected device to check available sensors
    const latestData = AppState.latestData[AppState.selectedDevice?.node_id] || {};
    const hasReceivedData = Object.keys(latestData).length > 0;

    Object.entries(MEASUREMENTS).forEach(([key, measurement]) => {
        // If we've received data, only show sensors that have data
        // Otherwise show all (will update after first packet)
        if (hasReceivedData) {
            const hasData = hasSensorData(key, latestData);
            if (!hasData) {
                debugLog(`⏭️ Skipping ${measurement.name} - no sensor data available`, 'info');
                return;
            }
        }

        const card = document.createElement('div');
        card.className = 'measurement-card';

        // Get color and icon for this measurement
        const color = MEASUREMENT_COLORS[key];
        const icon = MEASUREMENT_ICONS[key];

        // Apply color to card border
        card.style.borderColor = color;

        card.innerHTML = `
            <div class="measurement-header">
                <div class="measurement-icon" style="background-color: ${color}20; color: ${color};">
                    ${icon}
                </div>
                <div class="measurement-info">
                    <h4>${measurement.name}</h4>
                    <span class="unit">${measurement.unit}</span>
                </div>
            </div>
            <div class="measurement-options" id="options-${key}">
                ${measurement.options.map(option => `
                    <button class="btn btn-outline-blue"
                            data-measurement="${key}"
                            data-option="${option}">
                        ${option}
                    </button>
                `).join('')}
            </div>
        `;
        grid.appendChild(card);
    });

    // Add click handlers to option buttons
    document.querySelectorAll('.measurement-options button').forEach(btn => {
        btn.addEventListener('click', handleOptionClick);
    });

    debugLog(`✅ Rendered ${Object.keys(MEASUREMENTS).length} measurement cards`, 'success');
}

function handleOptionClick(event) {
    const btn = event.currentTarget;
    const measurement = btn.dataset.measurement;
    const option = btn.dataset.option;

    // Toggle button state
    if (btn.classList.contains('btn-outline-blue')) {
        btn.classList.remove('btn-outline-blue');
        btn.classList.add('btn-primary-blue');

        // Add display card
        addDisplayCard(measurement, option);
    } else {
        btn.classList.remove('btn-primary-blue');
        btn.classList.add('btn-outline-blue');

        // Remove display card
        removeDisplayCard(measurement, option);
    }
}

// ============================================
// Display Cards
// ============================================
function addDisplayCard(measurementKey, optionType) {
    const measurement = MEASUREMENTS[measurementKey];
    const displayId = `display-${measurementKey}-${optionType.replace(/\s+/g, '-')}`;

    debugLog(`➕ Adding display card: ${displayId}`, 'info');

    // Check if already exists
    if (document.getElementById(displayId)) {
        debugLog(`⚠️ Display card ${displayId} already exists, skipping`, 'warning');
        return;
    }

    const container = document.getElementById('activeDisplays');
    const displaySection = document.getElementById('displaySection');

    // Show display section
    displaySection.style.display = 'block';

    // Create card
    const card = document.createElement('div');
    card.className = 'display-card';
    card.id = displayId;

    // Apply color to card border
    const color = MEASUREMENT_COLORS[measurementKey];
    card.style.borderColor = color;

    // Create header
    const header = createDisplayHeader(measurementKey, measurement, optionType, displayId);
    card.appendChild(header);

    // Create content based on option type
    const content = createDisplayContent(measurementKey, measurement, optionType, displayId);
    card.appendChild(content);

    container.appendChild(card);

    // Track in state
    AppState.activeDisplays.push({ id: displayId, measurementKey, optionType });

    // Initialize default time scale for time-graphs
    if (optionType === 'Time-graph') {
        AppState.timeScales[displayId] = 10; // Default 10 seconds
    }

    // Initialize default component for vector numeric displays
    if (measurement.isVector && (optionType === 'Numeric Only' || optionType === 'Numeric w. Statistics')) {
        AppState.vectorComponents[displayId] = 'norm'; // Default to norm
    }

    // Start data updates
    startDataUpdates(displayId, measurementKey, optionType);

    debugLog(`✅ Display card ${displayId} created successfully`, 'success');
}

function createDisplayHeader(measurementKey, measurement, optionType, displayId) {
    const header = document.createElement('div');
    header.className = 'display-header';

    // Get color and icon for this measurement
    const color = MEASUREMENT_COLORS[measurementKey];
    const icon = MEASUREMENT_ICONS[measurementKey];

    // For ambient light (white background), use black text on badge
    const badgeTextColor = measurementKey === 'ambientLight' ? 'color: #1a1a1a;' : '';

    header.innerHTML = `
        <div class="display-title">
            <div style="color: ${color}; width: 32px; height: 32px; display: inline-flex; align-items: center; justify-content: center;">
                ${icon}
            </div>
            <h4>${measurement.name}</h4>
        </div>
        <div class="display-controls">
            <span class="display-badge" style="background-color: ${color}; ${badgeTextColor}">${optionType}</span>
            <button class="btn-icon close-btn" onclick="closeDisplay('${displayId}')">
                <i class="bi bi-x-lg"></i>
            </button>
        </div>
    `;

    return header;
}

function createDisplayContent(measurementKey, measurement, optionType, displayId) {
    const content = document.createElement('div');
    content.className = 'display-content';

    if (optionType === 'Numeric Only') {
        content.innerHTML = createNumericDisplay(displayId, measurement.isVector);
    } else if (optionType === 'Numeric w. Statistics') {
        content.innerHTML = createStatisticsDisplay(displayId, measurement.isVector);
        initializeStatsWindow(displayId);
    } else if (optionType === 'Time-graph') {
        content.innerHTML = createChartDisplay(displayId);
        setTimeout(() => initializeChart(displayId, measurementKey, measurement), 200);
    } else if (optionType === 'Vector') {
        content.innerHTML = createVectorDisplay(displayId);
        setTimeout(() => initializeVectorDisplay(displayId, measurementKey), 200);
    } else if (optionType === 'UV Spectrum' || optionType === 'Visible Spectrum' || optionType === 'Full Spectrum') {
        content.innerHTML = createSpectrumDisplay(displayId, optionType);
        setTimeout(() => initializeSpectrumChart(displayId, measurementKey, optionType), 200);
    } else if (optionType === 'Thermal Matrix') {
        content.innerHTML = createThermalDisplay(displayId);
        setTimeout(() => initializeThermalDisplay(displayId), 200);
    }

    return content;
}

function createNumericDisplay(displayId, isVector) {
    const componentSelector = isVector ? `
        <div class="component-selector">
            <label for="${displayId}-component">Component:</label>
            <select id="${displayId}-component" onchange="changeVectorComponent('${displayId}')">
                <option value="norm">Norm (Magnitude)</option>
                <option value="x">X Component</option>
                <option value="y">Y Component</option>
                <option value="z">Z Component</option>
            </select>
        </div>
    ` : '';

    return `
        ${componentSelector}
        <div class="numeric-display">
            <div>
                <span class="numeric-value" id="${displayId}-value">--</span>
                <span class="numeric-unit" id="${displayId}-unit"></span>
            </div>
            <div class="numeric-timestamp" id="${displayId}-timestamp">Waiting for data...</div>
        </div>
    `;
}

function createStatisticsDisplay(displayId, isVector) {
    const componentSelector = isVector ? `
        <div class="component-selector">
            <label for="${displayId}-component">Component:</label>
            <select id="${displayId}-component" onchange="changeVectorComponent('${displayId}', true)">
                <option value="norm">Norm (Magnitude)</option>
                <option value="x">X Component</option>
                <option value="y">Y Component</option>
                <option value="z">Z Component</option>
            </select>
        </div>
    ` : '';

    return `
        ${componentSelector}
        <div class="stats-grid">
            <div class="stat-item">
                <div class="stat-label">Current</div>
                <div>
                    <span class="stat-value" id="${displayId}-current">--</span>
                    <span class="stat-unit" id="${displayId}-unit"></span>
                </div>
            </div>
            <div class="stat-item">
                <div class="stat-label">Average</div>
                <div>
                    <span class="stat-value" id="${displayId}-avg">--</span>
                    <span class="stat-unit" id="${displayId}-unit-avg"></span>
                </div>
            </div>
            <div class="stat-item">
                <div class="stat-label">Minimum</div>
                <div>
                    <span class="stat-value" id="${displayId}-min">--</span>
                    <span class="stat-unit" id="${displayId}-unit-min"></span>
                </div>
            </div>
            <div class="stat-item">
                <div class="stat-label">Maximum</div>
                <div>
                    <span class="stat-value" id="${displayId}-max">--</span>
                    <span class="stat-unit" id="${displayId}-unit-max"></span>
                </div>
            </div>
            <div class="stat-item">
                <div class="stat-label">Std Deviation</div>
                <div>
                    <span class="stat-value" id="${displayId}-std">--</span>
                    <span class="stat-unit" id="${displayId}-unit-std"></span>
                </div>
            </div>
            <div class="stat-item">
                <div class="stat-label">Sample Count</div>
                <div>
                    <span class="stat-value" id="${displayId}-count">0</span>
                </div>
            </div>
        </div>
        <div class="stats-controls">
            <button class="btn btn-outline-theme-reset" onclick="resetStatsWindow('${displayId}')">
                <i class="bi bi-arrow-clockwise"></i>
                Reset Statistics
            </button>
        </div>
    `;
}

function createChartDisplay(displayId) {
    return `
        <div class="chart-controls">
            <label for="${displayId}-timescale">Time Window:</label>
            <select id="${displayId}-timescale" onchange="changeTimeScale('${displayId}')">
                <option value="5">5 seconds</option>
                <option value="10" selected>10 seconds</option>
                <option value="30">30 seconds</option>
                <option value="60">1 minute</option>
                <option value="120">2 minutes</option>
                <option value="300">5 minutes</option>
            </select>
        </div>
        <div class="chart-container">
            <canvas id="${displayId}-chart"></canvas>
        </div>
        <div class="chart-info">
            <span class="display-hz" id="${displayId}-hz">-- Hz</span>
            <span id="${displayId}-update-time">Updated: --</span>
        </div>
    `;
}

function createVectorDisplay(displayId) {
    return `
        <div class="vector-display">
            <div class="vector-canvas-container">
                <canvas id="${displayId}-vector-canvas"></canvas>
            </div>
            <div class="vector-info">
                <div class="vector-component">
                    <div class="vector-component-label">X Component</div>
                    <div class="vector-component-value" id="${displayId}-x">0.00</div>
                </div>
                <div class="vector-component">
                    <div class="vector-component-label">Y Component</div>
                    <div class="vector-component-value" id="${displayId}-y">0.00</div>
                </div>
                <div class="vector-component">
                    <div class="vector-component-label">Z Component</div>
                    <div class="vector-component-value" id="${displayId}-z">0.00</div>
                </div>
                <div class="vector-component">
                    <div class="vector-component-label">Magnitude</div>
                    <div class="vector-component-value" id="${displayId}-magnitude">0.00</div>
                </div>
            </div>
        </div>
    `;
}

function createSpectrumDisplay(displayId, mode) {
    let modeInfo = '';
    if (mode === 'UV Spectrum') {
        modeInfo = '3 UV channels (200-400 nm)';
    } else if (mode === 'Visible Spectrum') {
        modeInfo = '11 visible channels (395-865 nm)';
    } else if (mode === 'Full Spectrum') {
        modeInfo = '14 channels: UV + Visible (200-865 nm)';
    }

    return `
        <div class="spectrum-container">
            <canvas id="${displayId}-spectrum"></canvas>
        </div>
        <div class="chart-info">
            <span>${modeInfo}</span>
            <span id="${displayId}-update-time">Updated: --</span>
        </div>
    `;
}

function createThermalDisplay(displayId) {
    return `
        <div class="thermal-container">
            <canvas id="${displayId}-thermal"></canvas>
        </div>
        <div class="thermal-legend">
            <div class="legend-title">Temperature Scale</div>
            <div class="legend-gradient"></div>
            <div class="legend-labels">
                <span>0°C</span>
                <span>40°C</span>
                <span>80°C</span>
            </div>
        </div>
        <div class="chart-info">
            <span>8×8 thermal array</span>
            <span id="${displayId}-update-time">Updated: --</span>
        </div>
    `;
}

function removeDisplayCard(measurementKey, optionType) {
    const displayId = `display-${measurementKey}-${optionType.replace(/\s+/g, '-')}`;
    const card = document.getElementById(displayId);

    if (card) {
        // Stop updates
        stopDataUpdates(displayId);

        // Remove card
        card.remove();

        // Remove from state
        AppState.activeDisplays = AppState.activeDisplays.filter(d => d.id !== displayId);
        delete AppState.timeScales[displayId];
        delete AppState.vectorComponents[displayId];
        delete AppState.statsWindows[displayId];

        // Hide section if no displays
        if (AppState.activeDisplays.length === 0) {
            document.getElementById('displaySection').style.display = 'none';
        }
    }
}

function closeDisplay(displayId) {
    // Find the display in state
    const display = AppState.activeDisplays.find(d => d.id === displayId);

    if (display) {
        // Find and reset the button
        const optionName = display.optionType;
        const button = document.querySelector(
            `button[data-measurement="${display.measurementKey}"][data-option="${optionName}"]`
        );

        if (button) {
            button.classList.remove('btn-primary-blue');
            button.classList.add('btn-outline-blue');
        }

        // Remove the display
        removeDisplayCard(display.measurementKey, display.optionType);
    }
}

function clearAllDisplays() {
    // Stop all intervals
    Object.keys(AppState.updateIntervals).forEach(key => {
        clearInterval(AppState.updateIntervals[key]);
    });

    // Clear charts
    Object.values(AppState.charts).forEach(chartData => {
        if (chartData.chart && chartData.chart.destroy) {
            chartData.chart.destroy();
        }
    });

    // Clear state
    AppState.activeDisplays = [];
    AppState.charts = {};
    AppState.updateIntervals = {};
    AppState.statsWindows = {};
    AppState.timeScales = {};
    AppState.vectorComponents = {};
    AppState.updateRates = {};

    // Clear DOM
    document.getElementById('activeDisplays').innerHTML = '';
    document.getElementById('displaySection').style.display = 'none';

    // Reset all buttons
    document.querySelectorAll('.measurement-options button').forEach(btn => {
        btn.classList.remove('btn-primary-blue');
        btn.classList.add('btn-outline-blue');
    });
}

// ============================================
// Component & Time Scale Changes
// ============================================
function changeVectorComponent(displayId, resetStats = false) {
    const selector = document.getElementById(`${displayId}-component`);
    const newComponent = selector.value;
    AppState.vectorComponents[displayId] = newComponent;

    // Reset statistics if requested
    if (resetStats) {
        resetStatsWindow(displayId);
    }
}

function changeTimeScale(displayId) {
    const selector = document.getElementById(`${displayId}-timescale`);
    const newScale = parseInt(selector.value);
    AppState.timeScales[displayId] = newScale;

    // Update info text
    const infoEl = document.getElementById(`${displayId}-window-info`);
    if (infoEl) {
        if (newScale < 60) {
            infoEl.textContent = `Last ${newScale} seconds`;
        } else {
            infoEl.textContent = `Last ${newScale / 60} minute${newScale > 60 ? 's' : ''}`;
        }
    }

    // DON'T clear data - let it accumulate to fill the new window
    // The updateChart function will handle pruning old data beyond the window
    const chartData = AppState.charts[displayId];
    if (chartData && chartData.chart) {
        // Update the chart's X-axis max to show the new time scale
        chartData.chart.options.scales.x.max = newScale;
        chartData.chart.update('none');
    }
}

// ============================================
// Data Updates (Live WebSocket Only)
// ============================================
function startDataUpdates(displayId, measurementKey, optionType) {
    debugLog(`📊 Display ${displayId} ready for live data`, 'info');
    // NO MOCK DATA - displays will be updated by WebSocket events in handleSensorData()
    // Initialize with waiting message
    const measurement = MEASUREMENTS[measurementKey];

    if (optionType === 'Numeric Only' || optionType === 'Numeric w. Statistics') {
        const valueEl = document.getElementById(`${displayId}-value`) || document.getElementById(`${displayId}-current`);
        if (valueEl) valueEl.textContent = '--';
        const timestampEl = document.getElementById(`${displayId}-timestamp`);
        if (timestampEl) timestampEl.textContent = 'Waiting for live data...';
    }
}

function stopDataUpdates(displayId) {
    // No intervals to stop - data comes from WebSocket
    debugLog(`📊 Display ${displayId} closed`, 'info');

    // Destroy chart if it exists (works for both time-series and spectrum charts)
    if (AppState.charts[displayId]) {
        debugLog(`🗑️ Destroying chart for ${displayId}`, 'info');
        if (AppState.charts[displayId].chart && AppState.charts[displayId].chart.destroy) {
            AppState.charts[displayId].chart.destroy();
        }
        delete AppState.charts[displayId];
    }

    // Clean up other state
    delete AppState.timeScales[displayId];
    delete AppState.vectorComponents[displayId];
    delete AppState.statsWindows[displayId];
    delete AppState.updateRates[displayId];
}

function updateDisplay(displayId, measurementKey, optionType, value) {
    // Track update rate (data arrival rate - always updated)
    const now = Date.now();
    if (!AppState.updateRates[displayId]) {
        AppState.updateRates[displayId] = {
            count: 0,
            startTime: now,
            lastUpdate: now,
            hz: 0
        };
    }

    const rate = AppState.updateRates[displayId];
    rate.count++;
    const timeSinceStart = (now - rate.startTime) / 1000; // seconds

    // Calculate Hz (updates per second)
    if (timeSinceStart >= 1) {
        rate.hz = (rate.count / timeSinceStart).toFixed(1);

        // Update Hz display if element exists
        const hzEl = document.getElementById(`${displayId}-hz`);
        if (hzEl) {
            hzEl.textContent = `${rate.hz} Hz`;
        }
    }

    // ALWAYS update data - no throttling here to prevent data loss
    const measurement = MEASUREMENTS[measurementKey];

    if (optionType === 'Numeric Only') {
        updateNumericDisplay(displayId, value, measurement, measurementKey);
    } else if (optionType === 'Numeric w. Statistics') {
        updateStatisticsDisplay(displayId, value, measurement, measurementKey);
    } else if (optionType === 'Time-graph') {
        updateChart(displayId, value, measurement);
    } else if (optionType === 'Vector') {
        updateVectorDisplay(displayId, value, measurement);
    } else if (optionType === 'UV Spectrum' || optionType === 'Visible Spectrum' || optionType === 'Full Spectrum') {
        updateSpectrumChart(displayId, value);
    } else if (optionType === 'Thermal Matrix') {
        updateThermalDisplay(displayId, value);
    }
}

function updateNumericDisplay(displayId, value, measurement, measurementKey) {
    let displayValue;

    if (measurement.isVector) {
        const component = AppState.vectorComponents[displayId] || 'norm';
        displayValue = value[component];
    } else {
        displayValue = value;
    }

    const valueEl = document.getElementById(`${displayId}-value`);
    const unitEl = document.getElementById(`${displayId}-unit`);
    const timestampEl = document.getElementById(`${displayId}-timestamp`);

    // Get measurement-specific color
    const color = MEASUREMENT_COLORS[measurementKey];

    if (valueEl) {
        // Use 4 significant figures for acceleration, 2 decimal places for others
        const formattedValue = measurementKey === 'acceleration'
            ? toSignificantFigures(displayValue, 4)
            : displayValue.toFixed(2);
        valueEl.textContent = formattedValue;
        valueEl.style.color = color; // Apply measurement color
    }
    if (unitEl) unitEl.textContent = measurement.unit;
    if (timestampEl) timestampEl.textContent = `Updated: ${getCachedTimestamp()}`;
}

// ============================================
// Statistics Management
// ============================================
function initializeStatsWindow(displayId) {
    AppState.statsWindows[displayId] = [];
}

function resetStatsWindow(displayId) {
    AppState.statsWindows[displayId] = [];

    // Reset display
    ['current', 'avg', 'min', 'max', 'std'].forEach(stat => {
        const el = document.getElementById(`${displayId}-${stat}`);
        if (el) el.textContent = '--';
    });

    const countEl = document.getElementById(`${displayId}-count`);
    if (countEl) countEl.textContent = '0';
}

function updateStatisticsDisplay(displayId, value, measurement, measurementKey) {
    let displayValue;

    if (measurement.isVector) {
        const component = AppState.vectorComponents[displayId] || 'norm';
        displayValue = value[component];
    } else {
        displayValue = value;
    }

    // Add to window
    if (!AppState.statsWindows[displayId]) {
        AppState.statsWindows[displayId] = [];
    }
    AppState.statsWindows[displayId].push(displayValue);

    // Calculate statistics
    const data = AppState.statsWindows[displayId];
    const current = displayValue;
    const avg = data.reduce((a, b) => a + b, 0) / data.length;
    const min = Math.min(...data);
    const max = Math.max(...data);
    const variance = data.reduce((sum, val) => sum + Math.pow(val - avg, 2), 0) / data.length;
    const std = Math.sqrt(variance);

    // Get measurement-specific color
    const color = MEASUREMENT_COLORS[measurementKey];

    // Update display
    const currentEl = document.getElementById(`${displayId}-current`);
    const avgEl = document.getElementById(`${displayId}-avg`);
    const minEl = document.getElementById(`${displayId}-min`);
    const maxEl = document.getElementById(`${displayId}-max`);
    const stdEl = document.getElementById(`${displayId}-std`);
    const countEl = document.getElementById(`${displayId}-count`);

    // Format function: use 4 sig figs for acceleration, 2 decimal places for others
    const formatValue = (val) => measurementKey === 'acceleration'
        ? toSignificantFigures(val, 4)
        : val.toFixed(2);

    if (currentEl) {
        currentEl.textContent = formatValue(current);
        currentEl.style.color = color; // Apply measurement color to current value
    }
    if (avgEl) avgEl.textContent = formatValue(avg);
    if (minEl) minEl.textContent = formatValue(min);
    if (maxEl) maxEl.textContent = formatValue(max);
    if (stdEl) stdEl.textContent = formatValue(std);
    if (countEl) countEl.textContent = data.length;

    // Set units
    ['unit', 'unit-avg', 'unit-min', 'unit-max', 'unit-std'].forEach(suffix => {
        const el = document.getElementById(`${displayId}-${suffix}`);
        if (el) el.textContent = measurement.unit;
    });
}

// ============================================
// Chart Initialization & Updates
// ============================================
function initializeChart(displayId, measurementKey, measurement) {
    debugLog(`📈 Initializing chart for ${displayId}...`, 'info');

    // Destroy existing chart if it exists
    if (AppState.charts[displayId]) {
        debugLog(`⚠️ Chart already exists for ${displayId}, destroying it first`, 'warning');
        if (AppState.charts[displayId].chart && AppState.charts[displayId].chart.destroy) {
            AppState.charts[displayId].chart.destroy();
        }
        delete AppState.charts[displayId];
    }

    const canvas = document.getElementById(`${displayId}-chart`);
    if (!canvas) {
        debugLog(`❌ ERROR: Canvas element not found: ${displayId}-chart`, 'error');
        return;
    }

    debugLog(`✅ Canvas found for ${displayId}`, 'success');

    if (typeof Chart === 'undefined') {
        debugLog(`❌ ERROR: Chart.js not available when initializing ${displayId}`, 'error');
        return;
    }

    debugLog(`Creating Chart.js instance for ${displayId}...`, 'info');
    const ctx = canvas.getContext('2d');

    // Prepare datasets
    let datasets = [];

    if (measurement.isVector) {
        // Create dataset for each component
        const colors = {
            x: '#f8c01c',      // YELLOW
            y: '#375f83',      // BLUE
            z: '#d782a0',      // PINK
            norm: '#bd2026'    // RED
        };

        measurement.components.forEach(comp => {
            datasets.push({
                label: comp.toUpperCase(),
                data: [],
                borderColor: colors[comp],
                backgroundColor: colors[comp] + '20',
                borderWidth: 2,
                tension: 0.4,
                pointRadius: 0
            });
        });
    } else {
        // Single dataset - use measurement-specific color
        const color = MEASUREMENT_COLORS[measurementKey];
        datasets.push({
            label: measurement.name,
            data: [],
            borderColor: color,
            backgroundColor: color + '20',
            borderWidth: 2,
            tension: 0.4,
            pointRadius: 0
        });
    }

    const chart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: datasets
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: {
                duration: 0
            },
            scales: {
                x: {
                    display: true,
                    type: 'linear',
                    min: 0,
                    max: AppState.timeScales[displayId] || 10,
                    title: {
                        display: true,
                        text: 'Time (s)'
                    },
                    ticks: {
                        maxTicksLimit: 10
                    }
                },
                y: {
                    display: true,
                    title: {
                        display: true,
                        text: measurement.unit
                    }
                }
            },
            plugins: {
                legend: {
                    display: measurement.isVector,
                    position: 'top'
                }
            }
        }
    });

    AppState.charts[displayId] = {
        chart: chart,
        startTime: Date.now()
    };

    debugLog(`✅ Chart successfully initialized for ${displayId}`, 'success');
}

function updateChart(displayId, value, measurement) {
    const chartData = AppState.charts[displayId];
    if (!chartData) {
        return;
    }

    const { chart, startTime } = chartData;
    const timeScale = AppState.timeScales[displayId] || 10;
    const currentTime = (Date.now() - startTime) / 1000; // seconds

    // Add time label (absolute time since start)
    chart.data.labels.push(currentTime);

    // Add data points
    if (measurement.isVector) {
        measurement.components.forEach((comp, idx) => {
            chart.data.datasets[idx].data.push(value[comp]);
        });
    } else {
        chart.data.datasets[0].data.push(value);
    }

    // Windowing behavior:
    // Phase 1 (currentTime <= timeScale): Accumulation phase
    //   - X-axis fixed at [0, timeScale]
    //   - Data grows from time 0 onwards
    // Phase 2 (currentTime > timeScale): Sliding window phase
    //   - X-axis slides to show [currentTime - timeScale, currentTime]
    //   - Remove data points outside the window

    if (currentTime > timeScale) {
        // Sliding window: show most recent timeScale seconds
        const windowStart = currentTime - timeScale;

        // Update X-axis to slide with the data
        chart.options.scales.x.min = windowStart;
        chart.options.scales.x.max = currentTime;

        // Remove data points older than windowStart
        while (chart.data.labels.length > 0 && chart.data.labels[0] < windowStart) {
            chart.data.labels.shift();
            chart.data.datasets.forEach(dataset => dataset.data.shift());
        }
    } else {
        // Accumulation phase: X-axis stays at [0, timeScale]
        // Data accumulates, axis stays fixed
        chart.options.scales.x.min = 0;
        chart.options.scales.x.max = timeScale;
    }

    // THROTTLE RENDERING: Only call chart.update() every 75ms (13 Hz)
    // Data points are ALWAYS added above, we just throttle the expensive render
    const now = Date.now();
    const lastRender = AppState.lastRenderTime[displayId] || 0;
    const timeSinceRender = now - lastRender;

    if (timeSinceRender >= AppState.renderThrottle) {
        // Enough time has passed - render the chart
        chart.update('none');
        AppState.lastRenderTime[displayId] = now;

        // Update timestamp
        const timeEl = document.getElementById(`${displayId}-update-time`);
        if (timeEl) {
            timeEl.textContent = `Updated: ${getCachedTimestamp()}`;
        }
    }
}

// ============================================
// Vector Display (3D)
// ============================================
function initializeVectorDisplay(displayId, measurementKey) {
    debugLog(`🎯 Initializing 3D vector display for ${displayId}...`, 'info');

    const canvas = document.getElementById(`${displayId}-vector-canvas`);
    if (!canvas) {
        debugLog(`❌ ERROR: Vector canvas not found: ${displayId}-vector-canvas`, 'error');
        return;
    }

    debugLog(`✅ Vector canvas found for ${displayId}`, 'success');

    // Store canvas in state
    AppState.charts[displayId] = {
        canvas: canvas,
        ctx: canvas.getContext('2d')
    };

    // Draw initial empty 3D axes
    draw3DVector(displayId, { x: 0, y: 0, z: 0 });

    debugLog(`✅ 3D vector display successfully initialized for ${displayId}`, 'success');
}

function draw3DVector(displayId, vector) {
    const chartData = AppState.charts[displayId];
    if (!chartData) return;

    const { canvas, ctx } = chartData;
    const width = canvas.width = canvas.offsetWidth;
    const height = canvas.height = canvas.offsetHeight;

    // Clear canvas
    ctx.clearRect(0, 0, width, height);

    // Center point
    const cx = width / 2;
    const cy = height / 2;

    // Scale factor
    const scale = Math.min(width, height) / 8;

    // 3D to 2D projection (isometric)
    function project3D(x, y, z) {
        const angle = Math.PI / 6; // 30 degrees
        const px = cx + (x - z * Math.cos(angle)) * scale;
        const py = cy + (y - z * Math.sin(angle)) * scale;
        return { x: px, y: py };
    }

    // Draw axes
    ctx.strokeStyle = '#6b7280';
    ctx.lineWidth = 1;

    // X axis (red)
    ctx.beginPath();
    ctx.strokeStyle = '#f8c01c';
    let p = project3D(0, 0, 0);
    ctx.moveTo(p.x, p.y);
    p = project3D(3, 0, 0);
    ctx.lineTo(p.x, p.y);
    ctx.stroke();
    ctx.fillStyle = '#f8c01c';
    ctx.font = '12px Work Sans';
    ctx.fillText('X', p.x + 10, p.y);

    // Y axis (blue)
    ctx.beginPath();
    ctx.strokeStyle = '#375f83';
    p = project3D(0, 0, 0);
    ctx.moveTo(p.x, p.y);
    p = project3D(0, 3, 0);
    ctx.lineTo(p.x, p.y);
    ctx.stroke();
    ctx.fillStyle = '#375f83';
    ctx.fillText('Y', p.x, p.y - 10);

    // Z axis (pink)
    ctx.beginPath();
    ctx.strokeStyle = '#d782a0';
    p = project3D(0, 0, 0);
    ctx.moveTo(p.x, p.y);
    p = project3D(0, 0, 3);
    ctx.lineTo(p.x, p.y);
    ctx.stroke();
    ctx.fillStyle = '#d782a0';
    ctx.fillText('Z', p.x - 15, p.y);

    // Draw vector
    ctx.beginPath();
    ctx.strokeStyle = '#bd2026';
    ctx.lineWidth = 3;
    p = project3D(0, 0, 0);
    ctx.moveTo(p.x, p.y);
    const vp = project3D(vector.x, vector.y, vector.z);
    ctx.lineTo(vp.x, vp.y);
    ctx.stroke();

    // Draw arrowhead
    const angle = Math.atan2(vp.y - p.y, vp.x - p.x);
    ctx.beginPath();
    ctx.fillStyle = '#bd2026';
    ctx.moveTo(vp.x, vp.y);
    ctx.lineTo(vp.x - 10 * Math.cos(angle - Math.PI / 6), vp.y - 10 * Math.sin(angle - Math.PI / 6));
    ctx.lineTo(vp.x - 10 * Math.cos(angle + Math.PI / 6), vp.y - 10 * Math.sin(angle + Math.PI / 6));
    ctx.closePath();
    ctx.fill();

    // Draw vector endpoint
    ctx.beginPath();
    ctx.fillStyle = '#bd2026';
    ctx.arc(vp.x, vp.y, 5, 0, 2 * Math.PI);
    ctx.fill();

    // Draw origin
    p = project3D(0, 0, 0);
    ctx.beginPath();
    ctx.fillStyle = '#6b7280';
    ctx.arc(p.x, p.y, 4, 0, 2 * Math.PI);
    ctx.fill();
}

function updateVectorDisplay(displayId, value, measurement) {
    // Draw 3D vector
    draw3DVector(displayId, value);

    // Update component displays
    const xEl = document.getElementById(`${displayId}-x`);
    const yEl = document.getElementById(`${displayId}-y`);
    const zEl = document.getElementById(`${displayId}-z`);
    const magEl = document.getElementById(`${displayId}-magnitude`);

    if (xEl) xEl.textContent = value.x.toFixed(2);
    if (yEl) yEl.textContent = value.y.toFixed(2);
    if (zEl) zEl.textContent = value.z.toFixed(2);
    if (magEl) magEl.textContent = value.norm.toFixed(2);
}

// ============================================
// Spectrum Display
// ============================================
function initializeSpectrumChart(displayId, measurementKey, mode) {
    debugLog(`🌈 Initializing spectrum chart for ${displayId} (${mode})...`, 'info');

    // Destroy existing chart if it exists
    if (AppState.charts[displayId]) {
        debugLog(`⚠️ Spectrum chart already exists for ${displayId}, destroying it first`, 'warning');
        if (AppState.charts[displayId].chart && AppState.charts[displayId].chart.destroy) {
            AppState.charts[displayId].chart.destroy();
        }
        delete AppState.charts[displayId];
    }

    const canvas = document.getElementById(`${displayId}-spectrum`);
    if (!canvas) {
        debugLog(`❌ ERROR: Spectrum canvas not found: ${displayId}-spectrum`, 'error');
        return;
    }

    debugLog(`✅ Spectrum canvas found for ${displayId}`, 'success');

    if (typeof Chart === 'undefined') {
        debugLog(`❌ ERROR: Chart.js not available when initializing spectrum ${displayId}`, 'error');
        return;
    }

    debugLog(`Creating Chart.js bar chart for spectrum ${displayId}...`, 'info');
    const ctx = canvas.getContext('2d');

    // Define ALL possible channels
    const uvChannels = [
        { name: 'UVC', start: 200, end: 280, color: '#4c1d95' },
        { name: 'UVB', start: 280, end: 320, color: '#7c3aed' },
        { name: 'UVA', start: 320, end: 400, color: '#a855f7' }
    ];

    const visibleChannels = [
        { name: 'F1', start: 395, end: 415, color: '#8200c8' },
        { name: 'F2', start: 415, end: 435, color: '#5400ff' },
        { name: 'FZ', start: 440, end: 460, color: '#0046ff' },
        { name: 'F3', start: 465, end: 485, color: '#00c0ff' },
        { name: 'F4', start: 505, end: 525, color: '#1fff00' },
        { name: 'F5', start: 540, end: 560, color: '#a3ff00' },
        { name: 'FXL', start: 590, end: 610, color: '#ffbe00' },
        { name: 'F6', start: 630, end: 650, color: '#ff2100' },
        { name: 'F7', start: 680, end: 700, color: '#ff0000' },
        { name: 'F8', start: 735, end: 755, color: '#ab0000' },
        { name: 'NIR', start: 845, end: 865, color: '#610000' }
    ];

    // Configure channels and X-axis based on mode
    let spectrumChannels;
    let xMin, xMax;

    if (mode === 'UV Spectrum') {
        spectrumChannels = uvChannels;
        xMin = 180;
        xMax = 420;
    } else if (mode === 'Visible Spectrum') {
        spectrumChannels = visibleChannels;
        xMin = 380;
        xMax = 880;
    } else if (mode === 'Full Spectrum') {
        spectrumChannels = [...uvChannels, ...visibleChannels];
        xMin = 180;
        xMax = 880;
    } else {
        // Default to visible spectrum
        spectrumChannels = visibleChannels;
        xMin = 380;
        xMax = 880;
    }

    // Create datasets - one per channel for proper x-axis positioning
    const datasets = spectrumChannels.map(channel => ({
        label: channel.name,
        data: [{ x: (channel.start + channel.end) / 2, y: 0 }], // Position at center
        backgroundColor: channel.color,
        borderWidth: 0,
        categoryPercentage: 1.0,
        barPercentage: 1.0,
        // Store wavelength info for dynamic width calculation
        minBarLength: 2,
        wavelengthStart: channel.start,
        wavelengthEnd: channel.end
    }));

    const chart = new Chart(ctx, {
        type: 'bar',
        data: {
            datasets: datasets
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: {
                duration: 0
            },
            scales: {
                x: {
                    type: 'linear',
                    min: xMin,
                    max: xMax,
                    title: {
                        display: true,
                        text: 'Wavelength (nm)',
                        font: {
                            size: 14,
                            weight: 'bold'
                        }
                    },
                    ticks: {
                        stepSize: 50
                    }
                },
                y: {
                    beginAtZero: true,
                    suggestedMax: 100,  // Ensure reasonable scale even when all values are 0
                    title: {
                        display: true,
                        text: 'Intensity',
                        font: {
                            size: 14,
                            weight: 'bold'
                        }
                    }
                }
            },
            plugins: {
                legend: {
                    display: false
                },
                tooltip: {
                    callbacks: {
                        title: function(context) {
                            const dataset = context[0].dataset;
                            const start = dataset.wavelengthStart;
                            const end = dataset.wavelengthEnd;
                            return `${dataset.label}: ${start}-${end} nm`;
                        },
                        label: function(context) {
                            return `Intensity: ${context.parsed.y.toFixed(2)}`;
                        }
                    }
                }
            },
            // Calculate bar widths dynamically based on wavelength ranges
            onResize: function(chart) {
                updateBarWidths(chart, xMin, xMax);
            }
        }
    });

    // Store chart data
    AppState.charts[displayId] = { chart, spectrumChannels, measurementKey, xMin, xMax };

    // Calculate initial bar widths after a short delay to ensure chart has rendered
    setTimeout(() => {
        updateBarWidths(chart, xMin, xMax);
        chart.update('none');
        debugLog(`✅ Spectrum chart bar widths calculated for ${displayId}`, 'success');
    }, 150);

    debugLog(`✅ Spectrum chart initialized for ${displayId}: ${spectrumChannels.length} channels, X-axis ${xMin}-${xMax}nm`, 'success');
    console.log(`📊 [${displayId}] Initialized ${mode} with datasets:`, chart.data.datasets.map(ds => ds.label).join(', '));

    // DEBUG: Highlight UV datasets
    const uvDatasets = chart.data.datasets.filter(ds => ['UVA', 'UVB', 'UVC'].includes(ds.label));
    if (uvDatasets.length > 0) {
        console.log(`  🔬 UV datasets available: ${uvDatasets.map(ds => ds.label).join(', ')}`);
    }
}

// Helper function to calculate bar widths in pixels based on wavelength ranges
function updateBarWidths(chart, xMin, xMax) {
    const xScale = chart.scales.x;
    if (!xScale || !xScale.width) {
        console.warn('X-scale not ready, skipping bar width calculation');
        return;
    }

    const pixelRange = xScale.width;
    const dataRange = xMax - xMin;
    const pixelsPerNm = pixelRange / dataRange;

    console.log(`Bar width calculation: ${pixelRange}px / ${dataRange}nm = ${pixelsPerNm}px/nm`);

    chart.data.datasets.forEach((dataset, index) => {
        if (dataset.wavelengthStart && dataset.wavelengthEnd) {
            const wavelengthRange = dataset.wavelengthEnd - dataset.wavelengthStart;
            const barWidth = wavelengthRange * pixelsPerNm;
            dataset.barThickness = Math.max(barWidth, 3); // Minimum 3 pixels
            console.log(`  ${dataset.label}: ${wavelengthRange}nm = ${barWidth.toFixed(1)}px (set to ${dataset.barThickness}px)`);
        }
    });
}

function updateSpectrumChart(displayId, value) {
    const chartData = AppState.charts[displayId];
    if (!chartData || !chartData.chart) {
        console.error(`Spectrum chart not found for ${displayId}`);
        return;
    }

    const { chart, measurementKey } = chartData;

    // DEBUG: Log incoming channel data
    console.log(`📊 [${displayId}] Updating spectrum chart with ${value.channels ? value.channels.length : 0} channels`);
    if (value && value.channels) {
        const uvChannels = value.channels.filter(c => ['UVA', 'UVB', 'UVC'].includes(c.name));
        if (uvChannels.length > 0) {
            console.log(`  🔬 UV channels:`, uvChannels.map(c => `${c.name}=${c.value}`).join(', '));
        }
    }

    // Update chart values
    if (value && value.channels) {
        // For each channel in the data, find the matching dataset and update it
        value.channels.forEach((channel) => {
            // Find the dataset with matching channel name
            const datasetIndex = chart.data.datasets.findIndex(ds => ds.label === channel.name);
            if (datasetIndex !== -1) {
                // Update the y-value while keeping x-position at wavelength center
                chart.data.datasets[datasetIndex].data = [{
                    x: (channel.start + channel.end) / 2,
                    y: channel.value
                }];

                // DEBUG: Log UV channel updates
                if (['UVA', 'UVB', 'UVC'].includes(channel.name)) {
                    console.log(`  ✅ Updated dataset ${datasetIndex} (${channel.name}) with value ${channel.value}`);
                }
            } else {
                // DEBUG: Log if channel not found
                if (['UVA', 'UVB', 'UVC'].includes(channel.name)) {
                    console.log(`  ❌ No dataset found for ${channel.name}`);
                }
            }
        });
    }

    // Recalculate bar widths in case chart has been resized
    const { xMin, xMax } = chartData;
    updateBarWidths(chart, xMin, xMax);

    chart.update('none');

    // Update timestamp
    const timeEl = document.getElementById(`${displayId}-update-time`);
    if (timeEl) {
        timeEl.textContent = `Updated: ${getCachedTimestamp()}`;
    }
}

// ============================================
// Thermal Camera Display
// ============================================
function initializeThermalDisplay(displayId) {
    debugLog(`🌡️ Initializing thermal display for ${displayId}...`, 'info');

    const canvas = document.getElementById(`${displayId}-thermal`);
    if (!canvas) {
        debugLog(`❌ ERROR: Thermal canvas not found: ${displayId}-thermal`, 'error');
        return;
    }

    // Set canvas size for 8x8 grid
    canvas.width = 400;
    canvas.height = 400;

    // Store canvas context in AppState
    AppState.charts[displayId] = {
        canvas: canvas,
        ctx: canvas.getContext('2d')
    };

    debugLog(`✅ Thermal display initialized for ${displayId}`, 'success');
}

function updateThermalDisplay(displayId, thermalData) {
    const chartData = AppState.charts[displayId];
    if (!chartData || !chartData.ctx) {
        console.error(`Thermal display not found for ${displayId}`);
        return;
    }

    const { ctx, canvas } = chartData;

    if (!thermalData || !Array.isArray(thermalData)) {
        console.error(`Invalid thermal data for ${displayId}:`, thermalData);
        return;
    }

    // Convert flat array to 2D array if needed
    let thermalData2D;
    if (thermalData.length === 64) {
        // Flat array of 64 values - convert to 8x8
        console.log(`Converting flat array to 2D for ${displayId}`);
        thermalData2D = [];
        for (let row = 0; row < 8; row++) {
            thermalData2D[row] = [];
            for (let col = 0; col < 8; col++) {
                thermalData2D[row][col] = thermalData[row * 8 + col];
            }
        }
    } else if (thermalData.length === 8 && Array.isArray(thermalData[0])) {
        // Already a 2D array
        console.log(`Using 2D array for ${displayId}`);
        thermalData2D = thermalData;
    } else {
        console.error(`Invalid thermal data dimensions for ${displayId}:`, thermalData.length, thermalData);
        return;
    }

    // Clear canvas
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Cell size (50x50 pixels for 8x8 grid in 400x400 canvas)
    const cellSize = canvas.width / 8;

    // Render each pixel in the 8x8 array (rotated 90 degrees clockwise)
    for (let row = 0; row < 8; row++) {
        for (let col = 0; col < 8; col++) {
            // Rotate 90 degrees clockwise: (row, col) reads from (7-col, row)
            const sourceRow = 7 - col;
            const sourceCol = row;
            const temp = thermalData2D[sourceRow][sourceCol];

            // Skip invalid temperatures (NaN, null, undefined)
            if (temp === null || temp === undefined || isNaN(temp)) {
                continue;
            }

            // Get color for temperature (0-80°C range)
            const color = getThermalColor(temp);

            // Draw cell
            ctx.fillStyle = color;
            ctx.fillRect(col * cellSize, row * cellSize, cellSize, cellSize);

            // Draw cell border
            ctx.strokeStyle = 'rgba(0, 0, 0, 0.2)';
            ctx.lineWidth = 1;
            ctx.strokeRect(col * cellSize, row * cellSize, cellSize, cellSize);

            // Draw temperature value
            ctx.fillStyle = temp > 40 ? 'white' : 'black';
            ctx.font = '12px Work Sans, sans-serif';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText(
                temp.toFixed(1),
                col * cellSize + cellSize / 2,
                row * cellSize + cellSize / 2
            );
        }
    }

    // Update timestamp
    const timeEl = document.getElementById(`${displayId}-update-time`);
    if (timeEl) {
        timeEl.textContent = `Updated: ${getCachedTimestamp()}`;
    }
}

function getThermalColor(temp) {
    // Temperature range: 0-80°C
    // Color gradient: Blue (cold) -> Cyan -> Green -> Yellow -> Orange -> Red (hot)
    const minTemp = 0;
    const maxTemp = 80;

    // Clamp temperature to range
    const clampedTemp = Math.max(minTemp, Math.min(maxTemp, temp));

    // Normalize to 0-1
    const normalized = (clampedTemp - minTemp) / (maxTemp - minTemp);

    // Define color stops (RGB values)
    const colorStops = [
        { pos: 0.0, r: 59, g: 130, b: 246 },   // Blue (#3b82f6)
        { pos: 0.2, r: 34, g: 211, b: 238 },   // Cyan (#22d3ee)
        { pos: 0.4, r: 34, g: 197, b: 94 },    // Green (#22c55e)
        { pos: 0.6, r: 234, g: 179, b: 8 },    // Yellow (#eab308)
        { pos: 0.8, r: 249, g: 115, b: 22 },   // Orange (#f97316)
        { pos: 1.0, r: 239, g: 68, b: 68 }     // Red (#ef4444)
    ];

    // Find the two color stops to interpolate between
    let lowerStop = colorStops[0];
    let upperStop = colorStops[colorStops.length - 1];

    for (let i = 0; i < colorStops.length - 1; i++) {
        if (normalized >= colorStops[i].pos && normalized <= colorStops[i + 1].pos) {
            lowerStop = colorStops[i];
            upperStop = colorStops[i + 1];
            break;
        }
    }

    // Interpolate between the two stops
    const range = upperStop.pos - lowerStop.pos;
    const rangeNormalized = range === 0 ? 0 : (normalized - lowerStop.pos) / range;

    const r = Math.round(lowerStop.r + (upperStop.r - lowerStop.r) * rangeNormalized);
    const g = Math.round(lowerStop.g + (upperStop.g - lowerStop.g) * rangeNormalized);
    const b = Math.round(lowerStop.b + (upperStop.b - lowerStop.b) * rangeNormalized);

    return `rgb(${r}, ${g}, ${b})`;
}

console.log('Pocket Lab Mockup JavaScript loaded');
