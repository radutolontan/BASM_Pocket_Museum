/**
 * Pocket Lab Data Tab JavaScript
 * Handles device selection and sensor display configuration
 */

// Module state
const DataTab = {
    devices: [],
    selectedDevice: null,
    charts: {},
    sensorData: {},
    statusInterval: null,
    subscriptionConfirmed: false,
    lastDataReceived: null,
    dataStarted: false
};

/**
 * Initialize the Data Tab
 */
function initDataTab() {
    console.log('Initializing Data Tab...');

    // Load active devices on tab activation
    const dataTab = document.getElementById('data-tab');
    if (dataTab) {
        dataTab.addEventListener('click', () => {
            loadActiveDevices();
        });
    }

    // Load devices on initial page load
    loadActiveDevices();

    // Refresh device list every 5 seconds
    setInterval(loadActiveDevices, 5000);

    // Listen for subscription confirmations (when socket is ready)
    setTimeout(() => {
        if (AppState.socket) {
            AppState.socket.on('subscription_response', (payload) => {
                console.log(`✅ Subscription confirmed for ${payload.node_id} (status: ${payload.status})`);
                if (payload.status === 'subscribed') {
                    DataTab.subscriptionConfirmed = true;
                }
            });
        }
    }, 1000);
}

/**
 * Load active devices from API
 */
async function loadActiveDevices() {
    try {
        const response = await apiCall('/api/devices/active');
        if (response.devices) {
            DataTab.devices = response.devices;
            renderDeviceList();
        }
    } catch (error) {
        console.error('Failed to load devices:', error);
    }
}

/**
 * Render device list as cards
 */
function renderDeviceList() {
    const deviceList = document.getElementById('device-list');
    if (!deviceList) return;

    if (DataTab.devices.length === 0) {
        deviceList.innerHTML = `
            <div class="col-12">
                <div class="alert alert-info">
                    No active Pocket Labs found. Waiting for ESP32 devices to connect...
                </div>
            </div>
        `;
        return;
    }

    deviceList.innerHTML = DataTab.devices.map(device => `
        <div class="col-md-6 col-lg-4 mb-3">
            <div class="card device-card ${DataTab.selectedDevice?.node_id === device.node_id ? 'border-primary' : ''}"
                 onclick="selectDataDevice('${device.node_id}')"
                 style="cursor: pointer;">
                <div class="card-body">
                    <h5 class="card-title">
                        ${device.node_id}
                        ${DataTab.selectedDevice?.node_id === device.node_id ? '<span class="badge bg-primary">Active</span>' : ''}
                    </h5>
                    <p class="card-text">
                        <small class="text-muted">
                            IP: ${device.ip_address || 'N/A'}<br>
                            Last seen: ${formatTimestamp(device.last_seen)}
                        </small>
                    </p>
                </div>
            </div>
        </div>
    `).join('');
}

/**
 * Subscribe to the currently selected device
 * Handles reconnection and retry logic
 */
function subscribeToSelectedDevice(retryCount = 0) {
    if (!DataTab.selectedDevice) return;

    if (!AppState.socket || !AppState.socket.connected) {
        console.warn('⏳ WebSocket not connected, will retry subscription when connected...');

        // Retry after a delay if we haven't exceeded max retries
        if (retryCount < 5) {
            setTimeout(() => subscribeToSelectedDevice(retryCount + 1), 1000);
        }
        return;
    }

    console.log(`📡 Subscribing to device ${DataTab.selectedDevice.node_id}... (attempt ${retryCount + 1})`);
    AppState.socket.emit('subscribe_device', { node_id: DataTab.selectedDevice.node_id });

    // Set up a timeout to check if we got confirmation
    if (!DataTab.subscriptionConfirmed) {
        setTimeout(() => {
            if (!DataTab.subscriptionConfirmed && retryCount < 3) {
                console.warn('⚠️ No subscription confirmation received, retrying...');
                subscribeToSelectedDevice(retryCount + 1);
            }
        }, 2000);
    }
}

/**
 * Handle WebSocket reconnection
 * Re-subscribe to the selected device if any
 */
window.onWebSocketReconnect = function() {
    console.log('♻️ WebSocket reconnected - re-subscribing to selected device...');
    if (DataTab.selectedDevice) {
        // Reset confirmation flag
        DataTab.subscriptionConfirmed = false;
        // Wait a brief moment for the server to be ready, then retry with backoff
        setTimeout(() => subscribeToSelectedDevice(0), 500);
    }
};

/**
 * Select a device for live data viewing
 */
window.selectDataDevice = function(nodeId) {
    const device = DataTab.devices.find(d => d.node_id === nodeId);
    if (!device) return;

    // Unsubscribe from previous device if any
    if (DataTab.selectedDevice && AppState.socket && AppState.socket.connected) {
        AppState.socket.emit('unsubscribe_device', { node_id: DataTab.selectedDevice.node_id });
    }

    // Select new device
    DataTab.selectedDevice = device;
    DataTab.subscriptionConfirmed = false;
    DataTab.lastDataReceived = null;
    console.log(`✅ Selected device: ${nodeId}`);

    // Re-render device list to show selection
    renderDeviceList();

    // Clear existing charts
    clearSensorDisplay();

    // Initialize sensor display
    initSensorDisplay();

    // Subscribe to WebSocket room for this device (start with retry count 0)
    subscribeToSelectedDevice(0);
};

/**
 * Initialize sensor display area
 */
function initSensorDisplay() {
    const sensorDisplay = document.getElementById('sensor-display');
    if (!sensorDisplay) return;

    const connectionStatus = AppState.socket && AppState.socket.connected
        ? '<span class="badge bg-success">Connected</span>'
        : '<span class="badge bg-warning">Connecting...</span>';

    sensorDisplay.innerHTML = `
        <div class="d-flex justify-content-between align-items-center mb-3">
            <h3 class="mb-0">Live Data: ${DataTab.selectedDevice.node_id}</h3>
            <div id="connectionStatus">${connectionStatus}</div>
        </div>
        <div class="row">
            <div class="col-md-6 mb-4">
                <div class="card">
                    <div class="card-body">
                        <h5 class="card-title">Temperature</h5>
                        <canvas id="tempChart"></canvas>
                    </div>
                </div>
            </div>
            <div class="col-md-6 mb-4">
                <div class="card">
                    <div class="card-body">
                        <h5 class="card-title">Humidity</h5>
                        <canvas id="humidityChart"></canvas>
                    </div>
                </div>
            </div>
            <div class="col-md-6 mb-4">
                <div class="card">
                    <div class="card-body">
                        <h5 class="card-title">Pressure</h5>
                        <canvas id="pressureChart"></canvas>
                    </div>
                </div>
            </div>
            <div class="col-md-6 mb-4">
                <div class="card">
                    <div class="card-body">
                        <h5 class="card-title">Gas Resistance</h5>
                        <canvas id="gasChart"></canvas>
                    </div>
                </div>
            </div>
        </div>
        <div class="alert alert-info" id="waitingMessage">
            Waiting for live data from ${DataTab.selectedDevice.node_id}...
        </div>
    `;

    // Initialize charts
    initCharts();

    // Update connection status periodically
    updateConnectionStatus();
    DataTab.statusInterval = setInterval(updateConnectionStatus, 1000);
}

/**
 * Update connection status badge
 */
function updateConnectionStatus() {
    const statusElement = document.getElementById('connectionStatus');
    if (!statusElement) return;

    let statusHTML = '';

    if (AppState.socket && AppState.socket.connected) {
        statusHTML = '<span class="badge bg-success">Connected</span>';

        // Check if we're receiving data
        if (DataTab.lastDataReceived) {
            const timeSinceData = (new Date() - DataTab.lastDataReceived) / 1000;
            if (timeSinceData < 2) {
                statusHTML += ' <span class="badge bg-info ms-1">Receiving Data</span>';
            } else if (timeSinceData < 10) {
                statusHTML += ' <span class="badge bg-warning ms-1">Waiting...</span>';
            } else {
                statusHTML += ' <span class="badge bg-danger ms-1">No Data</span>';
            }
        } else if (DataTab.subscriptionConfirmed) {
            statusHTML += ' <span class="badge bg-warning ms-1">Subscribed, waiting...</span>';
        } else {
            statusHTML += ' <span class="badge bg-secondary ms-1">Subscribing...</span>';
        }
    } else {
        statusHTML = '<span class="badge bg-danger">Disconnected</span>';
    }

    statusElement.innerHTML = statusHTML;
}

/**
 * Initialize Chart.js charts
 */
function initCharts() {
    const maxDataPoints = 50;

    const chartConfig = (label, color) => ({
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: label,
                data: [],
                borderColor: color,
                backgroundColor: color + '20',
                tension: 0.4,
                fill: true
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: true,
            animation: false,
            scales: {
                x: {
                    display: false
                },
                y: {
                    beginAtZero: false
                }
            },
            plugins: {
                legend: {
                    display: true
                }
            }
        }
    });

    // Create charts
    const tempCtx = document.getElementById('tempChart');
    const humidityCtx = document.getElementById('humidityChart');
    const pressureCtx = document.getElementById('pressureChart');
    const gasCtx = document.getElementById('gasChart');

    if (tempCtx) DataTab.charts.temperature = new Chart(tempCtx, chartConfig('Temperature (°C)', '#dc3545'));
    if (humidityCtx) DataTab.charts.humidity = new Chart(humidityCtx, chartConfig('Humidity (%)', '#0dcaf0'));
    if (pressureCtx) DataTab.charts.pressure = new Chart(pressureCtx, chartConfig('Pressure (hPa)', '#198754'));
    if (gasCtx) DataTab.charts.gas = new Chart(gasCtx, chartConfig('Gas Resistance (Ω)', '#ffc107'));
}

/**
 * Clear sensor display
 */
function clearSensorDisplay() {
    // Destroy existing charts
    Object.values(DataTab.charts).forEach(chart => {
        if (chart) chart.destroy();
    });
    DataTab.charts = {};

    // Reset data tracking
    DataTab.dataStarted = false;
    DataTab.lastDataReceived = null;
    DataTab.subscriptionConfirmed = false;

    // Clear status update interval
    if (DataTab.statusInterval) {
        clearInterval(DataTab.statusInterval);
        DataTab.statusInterval = null;
    }

    const sensorDisplay = document.getElementById('sensor-display');
    if (sensorDisplay) {
        sensorDisplay.innerHTML = '';
    }
}

/**
 * Update sensor display with incoming data
 * Called by app.js when sensor_data event is received
 */
window.updateSensorDisplay = function(payload) {
    if (!DataTab.selectedDevice) return;
    if (payload.node_id !== DataTab.selectedDevice.node_id) return;

    const data = payload.data;
    const timestamp = new Date().toLocaleTimeString();

    // Track data reception
    DataTab.lastDataReceived = new Date();

    // Log first data packet or periodically
    if (!DataTab.dataStarted) {
        console.log('🎉 First sensor data received! Live updates active.');
        DataTab.dataStarted = true;
    }

    // Debug log (only occasionally to avoid spam)
    if (Math.random() < 0.05) {  // 5% of packets
        console.log('📊 Sensor data:', data);
    }

    const maxDataPoints = 50;

    // Update temperature chart
    if (data.temperature !== undefined && DataTab.charts.temperature) {
        const chart = DataTab.charts.temperature;
        chart.data.labels.push(timestamp);
        chart.data.datasets[0].data.push(data.temperature);

        if (chart.data.labels.length > maxDataPoints) {
            chart.data.labels.shift();
            chart.data.datasets[0].data.shift();
        }

        chart.update('none');
    }

    // Update humidity chart
    if (data.humidity !== undefined && DataTab.charts.humidity) {
        const chart = DataTab.charts.humidity;
        chart.data.labels.push(timestamp);
        chart.data.datasets[0].data.push(data.humidity);

        if (chart.data.labels.length > maxDataPoints) {
            chart.data.labels.shift();
            chart.data.datasets[0].data.shift();
        }

        chart.update('none');
    }

    // Update pressure chart
    if (data.pressure !== undefined && DataTab.charts.pressure) {
        const chart = DataTab.charts.pressure;
        chart.data.labels.push(timestamp);
        chart.data.datasets[0].data.push(data.pressure);

        if (chart.data.labels.length > maxDataPoints) {
            chart.data.labels.shift();
            chart.data.datasets[0].data.shift();
        }

        chart.update('none');
    }

    // Update gas resistance chart
    if (data.gas_resistance !== undefined && DataTab.charts.gas) {
        const chart = DataTab.charts.gas;
        chart.data.labels.push(timestamp);
        chart.data.datasets[0].data.push(data.gas_resistance);

        if (chart.data.labels.length > maxDataPoints) {
            chart.data.labels.shift();
            chart.data.datasets[0].data.shift();
        }

        chart.update('none');
    }

    // Remove "waiting for data" message if present
    const alerts = document.querySelectorAll('#sensor-display .alert-info');
    alerts.forEach(alert => alert.remove());
};

/**
 * Update device status
 * Called by app.js when device_status event is received
 */
window.updateDeviceStatus = function(data) {
    console.log('Device status update:', data);

    // Update device in the list if it exists
    const deviceIndex = DataTab.devices.findIndex(d => d.node_id === data.node_id);
    if (deviceIndex >= 0) {
        DataTab.devices[deviceIndex].last_seen = data.last_seen;
        if (data.status === 'active') {
            DataTab.devices[deviceIndex].status = 'active';
        }
    } else if (data.status === 'active') {
        // New device appeared, reload device list
        loadActiveDevices();
    }
};

// Initialize when DOM is ready
document.addEventListener('DOMContentLoaded', initDataTab);
