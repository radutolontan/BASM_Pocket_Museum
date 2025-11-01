/**
 * Pocket Lab Data Mockup - JavaScript
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
    statsWindows: {}
};

// Mock Devices
const MOCK_DEVICES = [
    { id: 'LAB_01', name: 'ESP32-01', ip: '192.168.10.11', active: true, lastSeen: '2s ago' },
    { id: 'LAB_02', name: 'ESP32-02', ip: '192.168.10.12', active: true, lastSeen: '1s ago' },
    { id: 'LAB_03', name: 'ESP32-03', ip: '192.168.10.13', active: true, lastSeen: '3s ago' },
    { id: 'LAB_04', name: 'ESP32-04', ip: '192.168.10.14', active: true, lastSeen: '1s ago' }
];

// Measurement Definitions
const MEASUREMENTS = {
    temperature: {
        name: 'Temperature',
        icon: 'bi-thermometer-half',
        unit: '°C',
        options: ['Time-graph', 'Numeric Only', 'Numeric w. Statistics'],
        color: '#f8c01c',
        baseValue: 23.5,
        variation: 1.5
    },
    pressure: {
        name: 'Pressure',
        icon: 'bi-speedometer2',
        unit: 'kPa',
        options: ['Time-graph', 'Numeric Only', 'Numeric w. Statistics'],
        color: '#375f83',
        baseValue: 101.3,
        variation: 0.5
    },
    acceleration: {
        name: 'Acceleration',
        icon: 'bi-arrow-up-right',
        unit: 'm/s²',
        options: ['Time-graph', 'Numeric Only', 'Numeric w. Statistics', 'Vector'],
        color: '#d782a0',
        isVector: true,
        components: ['x', 'y', 'z', 'norm']
    },
    gyro: {
        name: 'Gyroscope',
        icon: 'bi-arrow-clockwise',
        unit: 'deg/s',
        options: ['Time-graph', 'Numeric Only', 'Numeric w. Statistics', 'Vector'],
        color: '#bd2026',
        isVector: true,
        components: ['x', 'y', 'z', 'norm']
    },
    magnetometer: {
        name: 'Magnetometer',
        icon: 'bi-magnet',
        unit: 'µT',
        options: ['Time-graph', 'Numeric Only', 'Numeric w. Statistics', 'Vector'],
        color: '#375f83',
        isVector: true,
        components: ['x', 'y', 'z', 'norm']
    },
    volume: {
        name: 'Volume',
        icon: 'bi-soundwave',
        unit: 'dB',
        options: ['Time-graph', 'Numeric Only', 'Numeric w. Statistics'],
        color: '#d782a0',
        baseValue: -25,
        variation: 10
    },
    ambientLight: {
        name: 'Ambient Light',
        icon: 'bi-brightness-high',
        unit: 'lux',
        options: ['Time-graph', 'Numeric Only', 'Numeric w. Statistics'],
        color: '#f8c01c',
        baseValue: 542,
        variation: 100
    },
    spectrum: {
        name: 'Light Spectrum',
        icon: 'bi-palette',
        unit: '',
        options: ['Electromagnetic Spectrum'],
        color: '#bd2026'
    }
};

// ============================================
// Initialization
// ============================================
document.addEventListener('DOMContentLoaded', () => {
    initTheme();
    renderDeviceGrid();
    setupEventListeners();
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

    Object.entries(MEASUREMENTS).forEach(([key, measurement]) => {
        const card = document.createElement('div');
        card.className = 'measurement-card';
        card.innerHTML = `
            <div class="measurement-header">
                <div class="measurement-icon">
                    <i class="${measurement.icon}"></i>
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

    // Check if already exists
    if (document.getElementById(displayId)) {
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

    // Create header
    const header = createDisplayHeader(measurement, optionType, displayId);
    card.appendChild(header);

    // Create content based on option type
    const content = createDisplayContent(measurementKey, measurement, optionType, displayId);
    card.appendChild(content);

    container.appendChild(card);

    // Track in state
    AppState.activeDisplays.push({ id: displayId, measurementKey, optionType });

    // Start data updates
    startDataUpdates(displayId, measurementKey, optionType);
}

function createDisplayHeader(measurement, optionType, displayId) {
    const header = document.createElement('div');
    header.className = 'display-header';
    header.innerHTML = `
        <div class="display-title">
            <i class="${measurement.icon}"></i>
            <h4>${measurement.name}</h4>
        </div>
        <div class="display-controls">
            <span class="display-badge">${optionType}</span>
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
        content.innerHTML = createNumericDisplay(displayId);
    } else if (optionType === 'Numeric w. Statistics') {
        content.innerHTML = createStatisticsDisplay(displayId);
        initializeStatsWindow(displayId);
    } else if (optionType === 'Time-graph') {
        content.innerHTML = createChartDisplay(displayId);
        setTimeout(() => initializeChart(displayId, measurementKey, measurement), 100);
    } else if (optionType === 'Vector') {
        content.innerHTML = createVectorDisplay(displayId);
        setTimeout(() => initializeVectorDisplay(displayId, measurementKey), 100);
    } else if (optionType === 'Electromagnetic Spectrum') {
        content.innerHTML = createSpectrumDisplay(displayId);
        setTimeout(() => initializeSpectrumChart(displayId), 100);
    }

    return content;
}

function createNumericDisplay(displayId) {
    return `
        <div class="numeric-display">
            <div>
                <span class="numeric-value" id="${displayId}-value">--</span>
                <span class="numeric-unit" id="${displayId}-unit"></span>
            </div>
            <div class="numeric-timestamp" id="${displayId}-timestamp">Waiting for data...</div>
        </div>
    `;
}

function createStatisticsDisplay(displayId) {
    return `
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
            <button class="btn btn-outline-pink" onclick="resetStatsWindow('${displayId}')">
                <i class="bi bi-arrow-clockwise"></i>
                Reset Statistics
            </button>
        </div>
    `;
}

function createChartDisplay(displayId) {
    return `
        <div class="chart-container">
            <canvas id="${displayId}-chart"></canvas>
        </div>
        <div class="chart-info">
            <span>Last 10 seconds</span>
            <span id="${displayId}-update-time">Updated: --</span>
        </div>
    `;
}

function createVectorDisplay(displayId) {
    return `
        <div class="vector-display">
            <div class="chart-container" style="height: 350px;">
                <canvas id="${displayId}-vector-chart"></canvas>
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

    // Clear state
    AppState.activeDisplays = [];
    AppState.charts = {};
    AppState.updateIntervals = {};
    AppState.mockDataGenerators = {};
    AppState.statsWindows = {};

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
// Mock Data Generation
// ============================================
function generateMockValue(measurementKey) {
    const measurement = MEASUREMENTS[measurementKey];

    if (measurement.isVector) {
        // Generate vector components
        const x = (Math.random() - 0.5) * 2;
        const y = (Math.random() - 0.5) * 2;
        const z = (Math.random() - 0.5) * 2;
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
    const measurement = MEASUREMENTS[measurementKey];

    // Initial data
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
    const displayValue = measurement.isVector ? value.norm : value;

    document.getElementById(`${displayId}-value`).textContent = displayValue.toFixed(2);
    document.getElementById(`${displayId}-unit`).textContent = measurement.unit;
    document.getElementById(`${displayId}-timestamp`).textContent =
        `Updated: ${new Date().toLocaleTimeString()}`;
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
    document.getElementById(`${displayId}-current`).textContent = '--';
    document.getElementById(`${displayId}-avg`).textContent = '--';
    document.getElementById(`${displayId}-min`).textContent = '--';
    document.getElementById(`${displayId}-max`).textContent = '--';
    document.getElementById(`${displayId}-std`).textContent = '--';
    document.getElementById(`${displayId}-count`).textContent = '0';
}

function updateStatisticsDisplay(displayId, value, measurement) {
    const displayValue = measurement.isVector ? value.norm : value;

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
    document.getElementById(`${displayId}-current`).textContent = current.toFixed(2);
    document.getElementById(`${displayId}-avg`).textContent = avg.toFixed(2);
    document.getElementById(`${displayId}-min`).textContent = min.toFixed(2);
    document.getElementById(`${displayId}-max`).textContent = max.toFixed(2);
    document.getElementById(`${displayId}-std`).textContent = std.toFixed(2);
    document.getElementById(`${displayId}-count`).textContent = data.length;

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
    const canvas = document.getElementById(`${displayId}-chart`);
    if (!canvas) return;

    const ctx = canvas.getContext('2d');

    // Prepare datasets
    let datasets = [];

    if (measurement.isVector) {
        // Create dataset for each component
        const colors = {
            x: '#f8c01c',
            y: '#375f83',
            z: '#d782a0',
            norm: '#bd2026'
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
        // Single dataset
        datasets.push({
            label: measurement.name,
            data: [],
            borderColor: measurement.color,
            backgroundColor: measurement.color + '20',
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
            animation: false,
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
        startTime: Date.now(),
        maxPoints: 50
    };
}

function updateChart(displayId, value, measurement) {
    const chartData = AppState.charts[displayId];
    if (!chartData) return;

    const { chart, startTime, maxPoints } = chartData;
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

    // Keep only last 10 seconds of data (assuming 1 update per second)
    if (chart.data.labels.length > maxPoints) {
        chart.data.labels.shift();
        chart.data.datasets.forEach(dataset => dataset.data.shift());
    }

    // Update chart
    chart.update();

    // Update timestamp
    document.getElementById(`${displayId}-update-time`).textContent =
        `Updated: ${new Date().toLocaleTimeString()}`;
}

// ============================================
// Vector Display
// ============================================
function initializeVectorDisplay(displayId, measurementKey) {
    const canvas = document.getElementById(`${displayId}-vector-chart`);
    if (!canvas) return;

    const ctx = canvas.getContext('2d');

    // Create a 3D-looking scatter plot
    const chart = new Chart(ctx, {
        type: 'scatter',
        data: {
            datasets: [{
                label: 'Vector',
                data: [{x: 0, y: 0}],
                backgroundColor: '#f8c01c',
                pointRadius: 8,
                pointHoverRadius: 10
            }, {
                label: 'Origin',
                data: [{x: 0, y: 0}],
                backgroundColor: '#bd2026',
                pointRadius: 5
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                x: {
                    min: -3,
                    max: 3,
                    title: {
                        display: true,
                        text: 'X-Y Plane'
                    }
                },
                y: {
                    min: -3,
                    max: 3,
                    title: {
                        display: true,
                        text: 'Z Axis'
                    }
                }
            },
            plugins: {
                legend: {
                    display: false
                },
                tooltip: {
                    callbacks: {
                        label: function(context) {
                            return `(${context.parsed.x.toFixed(2)}, ${context.parsed.y.toFixed(2)})`;
                        }
                    }
                }
            }
        }
    });

    AppState.charts[displayId] = { chart };
}

function updateVectorDisplay(displayId, value, measurement) {
    const chartData = AppState.charts[displayId];
    if (!chartData) return;

    const { chart } = chartData;

    // Update scatter plot (project 3D to 2D: use X-Y plane, Z as vertical)
    const magnitude = Math.sqrt(value.x * value.x + value.y * value.y);
    chart.data.datasets[0].data = [{x: magnitude, y: value.z}];
    chart.update();

    // Update component displays
    document.getElementById(`${displayId}-x`).textContent = value.x.toFixed(2);
    document.getElementById(`${displayId}-y`).textContent = value.y.toFixed(2);
    document.getElementById(`${displayId}-z`).textContent = value.z.toFixed(2);
    document.getElementById(`${displayId}-magnitude`).textContent = value.norm.toFixed(2);
}

// ============================================
// Spectrum Display
// ============================================
function initializeSpectrumChart(displayId) {
    const canvas = document.getElementById(`${displayId}-spectrum`);
    if (!canvas) return;

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
}

function updateSpectrumChart(displayId, value) {
    const chartData = AppState.charts[displayId];
    if (!chartData) return;

    const { chart } = chartData;

    chart.data.labels = value.wavelengths.map((w, i) => `${value.names[i]}\n${w}nm`);
    chart.data.datasets[0].data = value.values;
    chart.update();

    // Update timestamp
    const timeEl = document.getElementById(`${displayId}-update-time`);
    if (timeEl) {
        timeEl.textContent = `Updated: ${new Date().toLocaleTimeString()}`;
    }
}

console.log('Pocket Lab Mockup loaded successfully');
