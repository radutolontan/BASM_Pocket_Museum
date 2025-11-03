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
    updateRates: {} // Track update rate (Hz) for each display: { displayId: { count, startTime, lastUpdate, hz } }
};

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
    ambientLight: '#fbbf24',    // BRIGHT AMBER/GOLD (for visibility, represents light)
    spectrum: '#8b5cf6'         // PURPLE (for spectrum)
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
        name: 'Light Spectrum',
        unit: '',
        options: ['Electromagnetic Spectrum']
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

    // Listen for sensor data
    AppState.socket.on('sensor_data', (payload) => {
        debugLog(`📦 Received sensor_data event for ${payload.node_id}`, 'info');
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
    debugLog(`🔍 handleSensorData called: node_id=${node_id}, selectedDevice=${AppState.selectedDevice?.node_id}`, 'info');

    // Only process if this is the selected device
    if (!AppState.selectedDevice || AppState.selectedDevice.node_id !== node_id) {
        debugLog(`⏭️ Skipping data for ${node_id} (selected: ${AppState.selectedDevice?.node_id})`, 'warning');
        return;
    }

    // Log RAW JSON data
    debugLog(`📦 RAW JSON DATA: ${JSON.stringify(data)}`, 'info');
    debugLog(`📊 Sensor data keys: ${Object.keys(data).join(', ')}`, 'info');

    // Check if this is the first data packet
    const isFirstPacket = !AppState.latestData[node_id];

    // Store latest data
    AppState.latestData[node_id] = data;

    // After first packet, refresh measurement grid to hide unavailable sensors
    if (isFirstPacket) {
        debugLog(`📊 First data packet received, refreshing measurement grid`, 'info');
        renderMeasurementGrid();
        return; // Don't update displays yet, they don't exist
    }

    debugLog(`📊 Processing sensor data for ${AppState.activeDisplays.length} active displays`, 'info');

    // Update all active displays with new data
    AppState.activeDisplays.forEach(display => {
        const measurementKey = display.measurementKey;
        const optionType = display.optionType;

        // Extract value based on measurement type
        let value = extractSensorValue(measurementKey, data);

        debugLog(`  → EXTRACTED ${measurementKey} (${optionType}): ${JSON.stringify(value)}`, 'info');

        if (value !== null && value !== undefined) {
            updateDisplay(display.id, measurementKey, optionType, value);
        } else {
            debugLog(`  ⚠️ No value extracted for ${measurementKey}`, 'warning');
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
            return {
                x: data.accel_x || 0,
                y: data.accel_y || 0,
                z: data.accel_z || 0,
                norm: data.accel_norm || 0
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
            // Return spectral data in format expected by chart
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
                <i class="bi bi-cpu-fill"></i>
            </div>
            <h3>${device.hostname || device.node_id}</h3>
            <div class="device-id">${device.node_id}</div>
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

    // Update banner
    document.getElementById('selectedDeviceName').textContent = device.hostname || device.node_id;
    document.getElementById('selectedDeviceId').textContent = device.node_id;

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
            return data.spectral_f1_405nm !== undefined;
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

    header.innerHTML = `
        <div class="display-title">
            <div style="color: ${color}; width: 32px; height: 32px; display: inline-flex; align-items: center; justify-content: center;">
                ${icon}
            </div>
            <h4>${measurement.name}</h4>
        </div>
        <div class="display-controls">
            <span class="display-badge" style="background-color: ${color};">${optionType}</span>
            <span class="display-hz" id="${displayId}-hz" style="font-size: 0.75rem; color: var(--text-tertiary); margin-right: 8px;">-- Hz</span>
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
    } else if (optionType === 'Electromagnetic Spectrum') {
        content.innerHTML = createSpectrumDisplay(displayId);
        setTimeout(() => initializeSpectrumChart(displayId), 200);
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
            <span id="${displayId}-window-info">Last 10 seconds</span>
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

function createSpectrumDisplay(displayId) {
    return `
        <div class="spectrum-container">
            <canvas id="${displayId}-spectrum"></canvas>
        </div>
        <div class="chart-info">
            <span>14 spectral channels</span>
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
    AppState.mockDataGenerators = {};
    AppState.statsWindows = {};
    AppState.timeScales = {};
    AppState.vectorComponents = {};

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

    // Clear chart data to start fresh with new scale
    const chartData = AppState.charts[displayId];
    if (chartData && chartData.chart) {
        chartData.chart.data.labels = [];
        chartData.chart.data.datasets.forEach(dataset => {
            dataset.data = [];
        });
        chartData.startTime = Date.now();
        chartData.chart.update();
    }
}

// ============================================
// Mock Data Generation
// ============================================
function generateMockValue(measurementKey) {
    const measurement = MEASUREMENTS[measurementKey];

    if (measurement.isVector) {
        // Generate vector components
        const x = (Math.random() - 0.5) * 4;
        const y = (Math.random() - 0.5) * 4;
        const z = (Math.random() - 0.5) * 4;
        const norm = Math.sqrt(x*x + y*y + z*z);
        return { x, y, z, norm };
    } else if (measurementKey === 'spectrum') {
        // Generate spectrum data
        return {
            wavelengths: [405, 425, 475, 515, 450, 555, 550, 640, 600, 690, 745, 855],
            names: ['F1', 'F2', 'F3', 'F4', 'FZ', 'FY', 'F5', 'F6', 'FXL', 'F7', 'F8', 'NIR'],
            values: Array(12).fill(0).map(() => Math.random() * 1000 + 200)
        };
    } else {
        // Generate scalar value
        const base = measurement.baseValue;
        const variation = measurement.variation;
        return base + (Math.random() - 0.5) * variation * 2;
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
    // Track update rate
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

    const timeSinceLastUpdate = (now - rate.lastUpdate);
    rate.lastUpdate = now;

    debugLog(`🔄 updateDisplay: ${displayId}, ${measurementKey}, ${optionType} (Δt=${timeSinceLastUpdate}ms, rate=${rate.hz}Hz)`, 'info');
    const measurement = MEASUREMENTS[measurementKey];

    if (optionType === 'Numeric Only') {
        updateNumericDisplay(displayId, value, measurement, measurementKey);
    } else if (optionType === 'Numeric w. Statistics') {
        updateStatisticsDisplay(displayId, value, measurement, measurementKey);
    } else if (optionType === 'Time-graph') {
        updateChart(displayId, value, measurement);
    } else if (optionType === 'Vector') {
        updateVectorDisplay(displayId, value, measurement);
    } else if (optionType === 'Electromagnetic Spectrum') {
        updateSpectrumChart(displayId, value);
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
    let color = MEASUREMENT_COLORS[measurementKey];

    // Special handling for ambient light in light theme
    // Use black text for better contrast on light backgrounds
    if (measurementKey === 'ambientLight') {
        const theme = document.documentElement.getAttribute('data-theme');
        if (!theme || theme === 'light') {
            color = '#1a1a1a'; // Black text in light theme
        }
    }

    if (valueEl) {
        valueEl.textContent = displayValue.toFixed(2);
        valueEl.style.color = color; // Apply measurement color
    }
    if (unitEl) unitEl.textContent = measurement.unit;
    if (timestampEl) timestampEl.textContent = `Updated: ${new Date().toLocaleTimeString()}`;
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
    let color = MEASUREMENT_COLORS[measurementKey];

    // Special handling for ambient light in light theme
    // Use black text for better contrast on light backgrounds
    if (measurementKey === 'ambientLight') {
        const theme = document.documentElement.getAttribute('data-theme');
        if (!theme || theme === 'light') {
            color = '#1a1a1a'; // Black text in light theme
        }
    }

    // Update display
    const currentEl = document.getElementById(`${displayId}-current`);
    const avgEl = document.getElementById(`${displayId}-avg`);
    const minEl = document.getElementById(`${displayId}-min`);
    const maxEl = document.getElementById(`${displayId}-max`);
    const stdEl = document.getElementById(`${displayId}-std`);
    const countEl = document.getElementById(`${displayId}-count`);

    if (currentEl) {
        currentEl.textContent = current.toFixed(2);
        currentEl.style.color = color; // Apply measurement color to current value
    }
    if (avgEl) avgEl.textContent = avg.toFixed(2);
    if (minEl) minEl.textContent = min.toFixed(2);
    if (maxEl) maxEl.textContent = max.toFixed(2);
    if (stdEl) stdEl.textContent = std.toFixed(2);
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
        debugLog(`❌ Chart data not found for ${displayId}`, 'error');
        return;
    }

    debugLog(`📈 Updating chart ${displayId} with value: ${JSON.stringify(value)}`, 'info');

    const { chart, startTime } = chartData;
    const timeScale = AppState.timeScales[displayId] || 10;
    const currentTime = (Date.now() - startTime) / 1000; // seconds

    // Add time label
    chart.data.labels.push(currentTime.toFixed(1));

    // Add data points
    if (measurement.isVector) {
        measurement.components.forEach((comp, idx) => {
            const componentValue = value[comp];
            debugLog(`  → Adding ${comp}=${componentValue} to dataset ${idx}`, 'info');
            chart.data.datasets[idx].data.push(componentValue);
        });
    } else {
        debugLog(`  → Adding value=${value} to chart`, 'info');
        chart.data.datasets[0].data.push(value);
    }

    // Keep only data within time scale
    const maxPoints = timeScale * 1; // 1 update per second
    if (chart.data.labels.length > maxPoints) {
        chart.data.labels.shift();
        chart.data.datasets.forEach(dataset => dataset.data.shift());
    }

    debugLog(`📈 Chart now has ${chart.data.labels.length} data points`, 'info');

    // Update chart
    chart.update('none'); // Use 'none' mode for better performance

    // Update timestamp
    const timeEl = document.getElementById(`${displayId}-update-time`);
    if (timeEl) {
        timeEl.textContent = `Updated: ${new Date().toLocaleTimeString()}`;
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
function initializeSpectrumChart(displayId) {
    debugLog(`🌈 Initializing spectrum chart for ${displayId}...`, 'info');

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

    const chart = new Chart(ctx, {
        type: 'bar',
        data: {
            labels: [],
            datasets: [{
                label: 'Intensity',
                data: [],
                backgroundColor: [
                    '#8b00ff', '#4b0082', '#0000ff', '#0080ff',
                    '#00ffff', '#00ff00', '#80ff00', '#ffff00',
                    '#ffa500', '#ff0000', '#8b0000', '#400000'
                ],
                borderWidth: 0
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: {
                duration: 0
            },
            scales: {
                x: {
                    title: {
                        display: true,
                        text: 'Wavelength (nm)'
                    }
                },
                y: {
                    beginAtZero: true,
                    title: {
                        display: true,
                        text: 'Intensity'
                    }
                }
            },
            plugins: {
                legend: {
                    display: false
                }
            }
        }
    });

    AppState.charts[displayId] = { chart };
    debugLog(`✅ Spectrum chart successfully initialized for ${displayId}`, 'success');
}

function updateSpectrumChart(displayId, value) {
    const chartData = AppState.charts[displayId];
    if (!chartData || !chartData.chart) {
        console.error(`Spectrum chart not found for ${displayId}`);
        return;
    }

    const { chart } = chartData;

    chart.data.labels = value.wavelengths.map((w, i) => `${value.names[i]}\n${w}nm`);
    chart.data.datasets[0].data = value.values;
    chart.update('none');

    // Update timestamp
    const timeEl = document.getElementById(`${displayId}-update-time`);
    if (timeEl) {
        timeEl.textContent = `Updated: ${new Date().toLocaleTimeString()}`;
    }
}

console.log('Pocket Lab Mockup JavaScript loaded');
