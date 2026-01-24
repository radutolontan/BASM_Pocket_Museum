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

    // Detect iOS/Safari for special handling
    const isIOS = /iPad|iPhone|iPod/.test(navigator.userAgent) && !window.MSStream;
    const isSafari = /^((?!chrome|android).)*safari/i.test(navigator.userAgent);

    console.log('Browser detection - iOS:', isIOS, 'Safari:', isSafari);

    // Configure Socket.IO with mobile-friendly settings
    // For iOS, try WebSocket first as polling can have issues
    const socketConfig = {
        // iOS: try WebSocket first, fallback to polling
        // Others: try polling first (more reliable), then upgrade to WebSocket
        transports: isIOS ? ['websocket', 'polling'] : ['polling', 'websocket'],
        // More aggressive reconnection for mobile browsers
        reconnection: true,
        reconnectionDelay: 500,
        reconnectionDelayMax: 3000,
        reconnectionAttempts: 15,
        // Longer timeout for mobile networks
        timeout: 20000,
        // Upgrade transport automatically when possible
        upgrade: true,
        // Allow reconnection even on transport errors
        rememberUpgrade: true,
        // Force new connection to avoid cached state issues on iOS
        forceNew: isIOS
    };

    console.log('Socket.IO config:', socketConfig);
    AppState.socket = io(CONFIG.WS_URL, socketConfig);

    AppState.socket.on('connect', () => {
        console.log('✅ WebSocket connected!');
        console.log('Transport:', AppState.socket.io.engine.transport.name);
        console.log('Session ID:', AppState.socket.id);

        // Notify other modules that WebSocket reconnected
        if (window.onWebSocketReconnect) {
            window.onWebSocketReconnect();
        }
    });

    AppState.socket.on('disconnect', (reason) => {
        console.log('❌ WebSocket disconnected:', reason);
    });

    AppState.socket.on('connect_error', (error) => {
        console.error('🔴 WebSocket connection error:', error.message);
        console.error('Error details:', error);
    });

    AppState.socket.on('reconnect_attempt', (attemptNumber) => {
        console.log(`🔄 Reconnecting... (attempt ${attemptNumber})`);
    });

    AppState.socket.on('reconnect', (attemptNumber) => {
        console.log(`✅ Reconnected after ${attemptNumber} attempts`);
    });

    AppState.socket.on('reconnect_failed', () => {
        console.error('❌ Reconnection failed after all attempts');
    });

    // Listen for transport upgrades
    AppState.socket.io.engine.on('upgrade', (transport) => {
        console.log('🚀 Transport upgraded to:', transport.name);
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
