/**
 * Main Application JavaScript
 * Handles global state, WebSocket connection, and utility functions
 */

// Global application state
const AppState = {
    user: null,
    macId: null,
    socket: null,
    currentDevice: null,
    devices: [],
    preferences: {}
};

// Configuration
const CONFIG = {
    API_BASE_URL: window.location.origin,
    WS_URL: window.location.origin
};

/**
 * Initialize the application
 */
function initApp() {
    console.log('Initializing BASM Dashboard...');

    // Get or generate MAC ID (in production, this would come from device)
    AppState.macId = getMacId();

    // Initialize WebSocket
    initWebSocket();

    // Check user registration
    checkUserRegistration();

    console.log('Dashboard initialized');
}

/**
 * Get or generate MAC ID
 * In production, this should come from the actual device MAC address
 */
function getMacId() {
    let macId = localStorage.getItem('mac_id');
    if (!macId) {
        // Generate a pseudo-MAC for testing
        macId = 'XX:XX:XX:XX:XX:' + Math.random().toString(16).substr(2, 2).toUpperCase();
        localStorage.setItem('mac_id', macId);
    }
    return macId;
}

/**
 * Hash MAC address for API calls
 */
function hashMacAddress(macAddress) {
    // Simple hash for demo - in production, use proper SHA-256
    // The backend will do the actual hashing
    return macAddress;
}

/**
 * Initialize WebSocket connection
 */
function initWebSocket() {
    console.log('Connecting to WebSocket...');

    AppState.socket = io(CONFIG.WS_URL);

    AppState.socket.on('connect', () => {
        console.log('WebSocket connected');
    });

    AppState.socket.on('disconnect', () => {
        console.log('WebSocket disconnected');
    });

    AppState.socket.on('connection_response', (data) => {
        console.log('Connection response:', data);
    });

    AppState.socket.on('sensor_data', (data) => {
        handleSensorData(data);
    });

    AppState.socket.on('device_status', (data) => {
        handleDeviceStatus(data);
    });

    AppState.socket.on('new_question', (question) => {
        handleNewQuestion(question);
    });

    AppState.socket.on('question_removed', (data) => {
        handleQuestionRemoved(data);
    });
}

/**
 * Check if user is registered
 */
async function checkUserRegistration() {
    // This will be implemented in welcome.js
    console.log('Checking user registration for MAC:', AppState.macId);
}

/**
 * Handle incoming sensor data
 */
function handleSensorData(data) {
    // This will be handled by data-tab.js and charts.js
    if (window.updateSensorDisplay) {
        window.updateSensorDisplay(data);
    }
}

/**
 * Handle device status updates
 */
function handleDeviceStatus(data) {
    console.log('Device status update:', data);
    // Update device list if needed
    if (window.updateDeviceStatus) {
        window.updateDeviceStatus(data);
    }
}

/**
 * Handle new question broadcast
 */
function handleNewQuestion(question) {
    console.log('New question:', question);
    if (window.addQuestion) {
        window.addQuestion(question);
    }
}

/**
 * Handle question removal
 */
function handleQuestionRemoved(data) {
    console.log('Question removed:', data);
    if (window.removeQuestion) {
        window.removeQuestion(data.question_id);
    }
}

/**
 * API Helper Functions
 */

async function apiCall(endpoint, options = {}) {
    try {
        const url = `${CONFIG.API_BASE_URL}${endpoint}`;
        const response = await fetch(url, {
            headers: {
                'Content-Type': 'application/json',
                ...options.headers
            },
            ...options
        });

        if (!response.ok) {
            const error = await response.json();
            throw new Error(error.error || 'API request failed');
        }

        return await response.json();
    } catch (error) {
        console.error('API call failed:', error);
        throw error;
    }
}

/**
 * Utility Functions
 */

function formatTimestamp(timestamp) {
    const date = new Date(timestamp);
    return date.toLocaleString();
}

function formatSensorValue(value, decimals = 2) {
    if (value === null || value === undefined || isNaN(value)) {
        return 'N/A';
    }
    return Number(value).toFixed(decimals);
}

function showError(message) {
    console.error(message);
    // TODO: Implement user-facing error notification
    alert(message);
}

function showSuccess(message) {
    console.log(message);
    // TODO: Implement user-facing success notification
}

// Initialize app when DOM is ready
document.addEventListener('DOMContentLoaded', initApp);
