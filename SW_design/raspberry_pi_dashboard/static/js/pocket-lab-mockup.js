/**
 * Pocket Lab Data Mockup - JavaScript (IMPROVED)
 * Complete interactive mockup with mock data
 */

// ============================================
// Global State
// ============================================
const AppState = {
    selectedDevice: null,
    activeDisplays: [],
    charts: {},
    updateIntervals: {},
    mockDataGenerators: {},
    statsWindows: {},
    timeScales: {}, // Store time scale for each time-graph
    vectorComponents: {} // Store selected component for vector numeric displays
};

// Mock Devices
const MOCK_DEVICES = [
    { id: 'LAB_01', name: 'ESP32-01', ip: '192.168.10.11', active: true, lastSeen: '2s ago' },
    { id: 'LAB_02', name: 'ESP32-02', ip: '192.168.10.12', active: true, lastSeen: '1s ago' },
    { id: 'LAB_03', name: 'ESP32-03', ip: '192.168.10.13', active: true, lastSeen: '3s ago' },
    { id: 'LAB_04', name: 'ESP32-04', ip: '192.168.10.14', active: true, lastSeen: '1s ago' }
];

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
    debugLog('🚀 Pocket Lab Mockup initializing...', 'info');

    // Check if Chart.js is loaded
    if (typeof Chart !== 'undefined') {
        debugLog('✅ Chart.js loaded successfully', 'success');
    } else {
        debugLog('❌ ERROR: Chart.js not loaded! Charts will not work.', 'error');
    }

    initTheme();
    renderDeviceGrid();
    setupEventListeners();
    debugLog('✅ Pocket Lab Mockup loaded successfully', 'success');
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
// Device Selection
// ============================================
function renderDeviceGrid() {
    const grid = document.getElementById('deviceGrid');
    grid.innerHTML = '';

    MOCK_DEVICES.forEach(device => {
        const card = document.createElement('div');
        card.className = 'device-card';
        card.innerHTML = `
            <div class="device-status-badge">
                <span class="status-indicator"></span>
                Active
            </div>
            <div class="device-icon">
                <i class="bi bi-cpu-fill"></i>
            </div>
            <h3>${device.name}</h3>
            <div class="device-id">${device.id}</div>
            <div class="device-meta">
                <span>${device.ip}</span>
                <span>Last seen: ${device.lastSeen}</span>
            </div>
        `;
        card.addEventListener('click', () => selectDevice(device));
        grid.appendChild(card);
    });
}

function selectDevice(device) {
    AppState.selectedDevice = device;

    // Update banner
    document.getElementById('selectedDeviceName').textContent = device.name;
    document.getElementById('selectedDeviceId').textContent = device.id;

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
function renderMeasurementGrid() {
    const grid = document.getElementById('measurementGrid');
    grid.innerHTML = '';
    debugLog('📊 Rendering measurement grid...', 'info');

    Object.entries(MEASUREMENTS).forEach(([key, measurement]) => {
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
            <button class="btn btn-outline-blue" onclick="resetStatsWindow('${displayId}')">
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
// Data Updates
// ============================================
function startDataUpdates(displayId, measurementKey, optionType) {
    console.log(`Starting data updates for ${displayId}`);

    // Generate initial value immediately
    const initialValue = generateMockValue(measurementKey);
    updateDisplay(displayId, measurementKey, optionType, initialValue);

    // Set up interval for continuous updates
    const interval = setInterval(() => {
        const value = generateMockValue(measurementKey);
        updateDisplay(displayId, measurementKey, optionType, value);
    }, 1000); // Update every second

    AppState.updateIntervals[displayId] = interval;
}

function stopDataUpdates(displayId) {
    if (AppState.updateIntervals[displayId]) {
        clearInterval(AppState.updateIntervals[displayId]);
        delete AppState.updateIntervals[displayId];
    }
}

function updateDisplay(displayId, measurementKey, optionType, value) {
    const measurement = MEASUREMENTS[measurementKey];

    if (optionType === 'Numeric Only') {
        updateNumericDisplay(displayId, value, measurement);
    } else if (optionType === 'Numeric w. Statistics') {
        updateStatisticsDisplay(displayId, value, measurement);
    } else if (optionType === 'Time-graph') {
        updateChart(displayId, value, measurement);
    } else if (optionType === 'Vector') {
        updateVectorDisplay(displayId, value, measurement);
    } else if (optionType === 'Electromagnetic Spectrum') {
        updateSpectrumChart(displayId, value);
    }
}

function updateNumericDisplay(displayId, value, measurement) {
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

    if (valueEl) valueEl.textContent = displayValue.toFixed(2);
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

function updateStatisticsDisplay(displayId, value, measurement) {
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

    // Update display
    const currentEl = document.getElementById(`${displayId}-current`);
    const avgEl = document.getElementById(`${displayId}-avg`);
    const minEl = document.getElementById(`${displayId}-min`);
    const maxEl = document.getElementById(`${displayId}-max`);
    const stdEl = document.getElementById(`${displayId}-std`);
    const countEl = document.getElementById(`${displayId}-count`);

    if (currentEl) currentEl.textContent = current.toFixed(2);
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
        console.error(`Chart data not found for ${displayId}`);
        return;
    }

    const { chart, startTime } = chartData;
    const timeScale = AppState.timeScales[displayId] || 10;
    const currentTime = (Date.now() - startTime) / 1000; // seconds

    // Add time label
    chart.data.labels.push(currentTime.toFixed(1));

    // Add data points
    if (measurement.isVector) {
        measurement.components.forEach((comp, idx) => {
            chart.data.datasets[idx].data.push(value[comp]);
        });
    } else {
        chart.data.datasets[0].data.push(value);
    }

    // Keep only data within time scale
    const maxPoints = timeScale * 1; // 1 update per second
    if (chart.data.labels.length > maxPoints) {
        chart.data.labels.shift();
        chart.data.datasets.forEach(dataset => dataset.data.shift());
    }

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
