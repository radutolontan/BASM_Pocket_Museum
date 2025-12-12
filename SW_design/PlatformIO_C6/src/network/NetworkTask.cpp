#include "NetworkTask.h"
#include "../shared_resources/global_debug.h"

// Constructor
NetworkTask::NetworkTask()
    : currentState(NetworkState::BOOT),
      previousState(NetworkState::BOOT),
      ssid(NETWORK_SSID),
      password(NETWORK_PASSWORD),
      staticIP(NETWORK_STATIC_IP),
      gateway(NETWORK_GATEWAY),
      subnet(NETWORK_SUBNET),
      primaryDNS(NETWORK_PRIMARY_DNS),
      secondaryDNS(NETWORK_SECONDARY_DNS),
      serverIP(SERVER_IP_ADDRESS),
      serverPort(SERVER_UDP_PORT),
      stateEntryTime(0),
      lastConnectionAttempt(0),
      lastLEDToggle(0),
      initialConnectionAttempted(false),
      networkAvailable(false),
      ledState(false),
      lastFreqPrintTime(0),
      updateCount(0),
      state_machine_run_freq(0.0f),
      dataSendCount(0),
      data_send_freq(0.0f) {
}

// Destructor
NetworkTask::~NetworkTask() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

// Setup function
void NetworkTask::setup(BMSTask* bms) {
    // Initialize LED pin
    pinMode(LED_PIN, OUTPUT);
    ledOff();

    // Set BMS Task pointer
    this->bmsTask = bms;

    NETWORK_PRINTF("[NetworkTask] Setup complete\n");
}

// FreeRTOS task wrapper
void NetworkTask::runNetworkTaskWrapper(void* param) {
    NetworkTask* self = static_cast<NetworkTask*>(param);
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(1000 / TASK_RATE_NETWORK);

    for (;;) {
        self->runNetworkTask();
        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}

// Main state machine dispatcher
void NetworkTask::runNetworkTask() {
    // Track state changes
    if (currentState != previousState) {
        stateEntryTime = millis();
        previousState = currentState;
        NETWORK_PRINTF("[NetworkTask] State changed to: %d\n", static_cast<int>(currentState));
    }

    // Run current state
    switch (currentState) {
        case NetworkState::BOOT:
            run_boot();
            break;
        case NetworkState::INIT:
            run_init();
            break;
        case NetworkState::CONNECTING:
            run_connecting();
            break;
        case NetworkState::CONNECTED:
            run_connected();
            break;
        case NetworkState::DISCONNECTED:
            run_disconnected();
            break;
        case NetworkState::NO_NETWORK:
            run_no_network();
            break;
    }

    // Track task frequency
    updateCount++;
    unsigned long now = millis();
    if (now - lastFreqPrintTime >= 10000) {
        state_machine_run_freq = updateCount / ((now - lastFreqPrintTime) / 1000.0f);
        data_send_freq = dataSendCount / ((now - lastFreqPrintTime) / 1000.0f);

        #if DEBUG_TASK_RATES
        RATES_PRINTF("[NetworkTask] State machine frequency: %.2f Hz\n", state_machine_run_freq);
        RATES_PRINTF("[NetworkTask] Data send frequency: %.2f Hz\n", data_send_freq);
        #endif

        updateCount = 0;
        dataSendCount = 0;
        lastFreqPrintTime = now;
    }
}

// State: BOOT - Wait for BMS to latch
void NetworkTask::run_boot() {
    // Only initialize WiFi once the BMS is confirmed latched
    if (bmsTask && bmsTask->isLatched()) {
        NETWORK_PRINTF("[NetworkTask] BMS latched, transitioning to INIT\n");
        currentState = NetworkState::INIT;
    }
}

// State: INIT - Initialize WiFi hardware
void NetworkTask::run_init() {
    NETWORK_PRINTF("[NetworkTask] Initializing WiFi...\n");

    // Set WiFi mode
    WiFi.mode(WIFI_STA);

    // Configure static IP
    configureStaticIP();

    // Set hostname
    WiFi.setHostname(NETWORK_HOSTNAME);

    NETWORK_PRINTF("[NetworkTask] WiFi initialized, transitioning to CONNECTING\n");
    currentState = NetworkState::CONNECTING;
}

// State: CONNECTING - Initial 30-second connection attempt with flashing LED
void NetworkTask::run_connecting() {
    unsigned long now = millis();
    unsigned long elapsed = now - stateEntryTime;

    // Flash LED during connection attempt
    ledFlash();

    // Check if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
        NETWORK_PRINTF("[NetworkTask] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        ledOn();
        networkAvailable = true;
        initialConnectionAttempted = true;
        currentState = NetworkState::CONNECTED;
        return;
    }

    // Try to connect if not already attempting
    if (WiFi.status() == WL_DISCONNECTED || WiFi.status() == WL_IDLE_STATUS) {
        if (now - lastConnectionAttempt >= 1000) {  // Try every second
            NETWORK_PRINTF("[NetworkTask] Attempting connection to %s...\n", ssid);
            WiFi.begin(ssid, password);
            lastConnectionAttempt = now;
        }
    }

    // Check if 30 seconds have elapsed
    if (elapsed >= NETWORK_INITIAL_CONNECTION_TIMEOUT_MS) {
        NETWORK_PRINTF("[NetworkTask] Initial connection timeout, assuming no network available\n");
        ledOff();
        initialConnectionAttempted = true;
        networkAvailable = false;
        WiFi.disconnect();
        currentState = NetworkState::NO_NETWORK;
    }
}

// State: CONNECTED - Send data every task iteration
void NetworkTask::run_connected() {
    // Keep LED solid on
    ledOn();

    // Check if still connected
    if (WiFi.status() != WL_CONNECTED) {
        NETWORK_PRINTF("[NetworkTask] Connection lost! Transitioning to DISCONNECTED\n");
        currentState = NetworkState::DISCONNECTED;
        return;
    }

    // Send data at task rate (TASK_RATE_NETWORK)
    sendSensorData();
    dataSendCount++;
}

// State: DISCONNECTED - Reconnect every 10 seconds
void NetworkTask::run_disconnected() {
    unsigned long now = millis();

    // Flash LED to indicate attempting reconnection
    ledFlash();

    // Check if reconnected
    if (WiFi.status() == WL_CONNECTED) {
        NETWORK_PRINTF("[NetworkTask] Reconnected! IP: %s\n", WiFi.localIP().toString().c_str());
        ledOn();
        currentState = NetworkState::CONNECTED;
        return;
    }

    // Try to reconnect every 10 seconds
    if (now - lastConnectionAttempt >= NETWORK_RECONNECT_INTERVAL_MS) {
        NETWORK_PRINTF("[NetworkTask] Attempting reconnection to %s...\n", ssid);
        WiFi.disconnect();
        WiFi.begin(ssid, password);
        lastConnectionAttempt = now;
    }
}

// State: NO_NETWORK - Do nothing, network assumed unavailable
void NetworkTask::run_no_network() {
    // LED is off, do nothing
    // This state is terminal - no retry logic
    ledOff();
}

// Configure static IP
void NetworkTask::configureStaticIP() {
    if (!WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS)) {
        NETWORK_PRINTF("[NetworkTask] Failed to configure static IP!\n");
    } else {
        NETWORK_PRINTF("[NetworkTask] Static IP configured: %s\n", staticIP.toString().c_str());
    }
}

// Send sensor data to server
void NetworkTask::sendSensorData() {
    // Get the most recent sensor data from SharedBuffer
    std::deque<SensorData> readings = SharedBuffer::getReadings();

    if (readings.empty()) {
        NETWORK_PRINTF("[NetworkTask] No sensor data available to send\n");
        return;
    }

    // Get the last frame (most recent)
    SensorData lastFrame = readings.back();

    // Format as JSON
    String jsonData = formatSensorDataJSON(lastFrame);

    // Send via UDP
    udp.beginPacket(serverIP, serverPort);
    udp.print(jsonData);
    udp.endPacket();

    #if DEBUG_NETWORK_PACKETS
    NETWORK_PRINTF("[NetworkTask] Sent: %s\n", jsonData.c_str());
    #endif
}

// Format sensor data as JSON
String NetworkTask::formatSensorDataJSON(const SensorData& data) {
    String json = "{";

    // Add node ID
    json += "\"node_id\":\"" + String(NETWORK_NODE_ID) + "\",";

    // Add timestamp
    json += "\"timestamp\":" + String(millis()) + ",";

    // Pressure sensor data
    if (data.hasPressure()) {
        json += "\"temperature\":" + String(data.temperature, 2) + ",";
        json += "\"pressure\":" + String(data.pressure, 2) + ",";
    }

    // Light sensor data
    if (data.hasLight()) {
        json += "\"light_intensity\":" + String(data.light_intensity, 2) + ",";
    }

    // IMU data
    if (data.hasIMU()) {
        json += "\"accel_x\":" + String(data.accel_x, 3) + ",";
        json += "\"accel_y\":" + String(data.accel_y, 3) + ",";
        json += "\"accel_z\":" + String(data.accel_z, 3) + ",";
        json += "\"accel_norm\":" + String(data.accel_norm, 3) + ",";
        json += "\"gyro_x\":" + String(data.gyro_x, 3) + ",";
        json += "\"gyro_y\":" + String(data.gyro_y, 3) + ",";
        json += "\"gyro_z\":" + String(data.gyro_z, 3) + ",";
        json += "\"gyro_norm\":" + String(data.gyro_norm, 3) + ",";
        json += "\"mag_x\":" + String(data.mag_x, 3) + ",";
        json += "\"mag_y\":" + String(data.mag_y, 3) + ",";
        json += "\"mag_z\":" + String(data.mag_z, 3) + ",";
        json += "\"mag_norm\":" + String(data.mag_norm, 3) + ",";
    }

    // Audio data
    if (data.hasAudio() && !isnan(data.volume_rms)) {
        json += "\"volume_rms\":" + String(data.volume_rms, 2) + ",";
    }

    // Spectral data (formatted as integers - ADC counts)
    if (data.hasSpectral()) {
        json += "\"spectral_f1\":" + String((int)data.spectral_f1_405nm) + ",";
        json += "\"spectral_f2\":" + String((int)data.spectral_f2_425nm) + ",";
        json += "\"spectral_f3\":" + String((int)data.spectral_f3_475nm) + ",";
        json += "\"spectral_f4\":" + String((int)data.spectral_f4_515nm) + ",";
        json += "\"spectral_fz\":" + String((int)data.spectral_fz_450nm) + ",";
        json += "\"spectral_fy\":" + String((int)data.spectral_fy_555nm) + ",";
        json += "\"spectral_f5\":" + String((int)data.spectral_f5_550nm) + ",";
        json += "\"spectral_f6\":" + String((int)data.spectral_f6_640nm) + ",";
        json += "\"spectral_fxl\":" + String((int)data.spectral_fxl_600nm) + ",";
        json += "\"spectral_f7\":" + String((int)data.spectral_f7_690nm) + ",";
        json += "\"spectral_f8\":" + String((int)data.spectral_f8_745nm) + ",";
        json += "\"spectral_nir\":" + String((int)data.spectral_nir_855nm) + ",";
        json += "\"spectral_vis\":" + String((int)data.spectral_vis) + ",";
        json += "\"spectral_fd\":" + String((int)data.spectral_fd) + ",";
    }

    // Spectral UV data (formatted as integers - ADC counts)
    if (data.hasSpectralUV()) {
        json += "\"spectral_uva\":" + String((int)data.spectral_UVA) + ",";
        json += "\"spectral_uvb\":" + String((int)data.spectral_UVB) + ",";
        json += "\"spectral_uvc\":" + String((int)data.spectral_UVC) + ",";
    }

    // Thermal array data (8x8 grid, 64 pixels)
    if (data.hasThermal()) {
        json += "\"thermal_pixels\":[";
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                json += String(data.thermal_pixels[row][col], 1);
                if (row < 7 || col < 7) {  // Not the last pixel
                    json += ",";
                }
            }
        }
        json += "],";
    }

    // Remove trailing comma if present
    if (json.endsWith(",")) {
        json.remove(json.length() - 1);
    }

    json += "}";
    return json;
}

// LED control functions
void NetworkTask::ledOff() {
    digitalWrite(LED_PIN, LOW);
    ledState = false;
}

void NetworkTask::ledOn() {
    digitalWrite(LED_PIN, HIGH);
    ledState = true;
}

void NetworkTask::ledFlash() {
    unsigned long now = millis();
    if (now - lastLEDToggle >= NETWORK_LED_FLASH_INTERVAL_MS) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? HIGH : LOW);
        lastLEDToggle = now;
    }
}

// Get local IP address
IPAddress NetworkTask::getLocalIP() const {
    return WiFi.localIP();
}
