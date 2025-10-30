#ifndef NETWORK_TASK_H
#define NETWORK_TASK_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "../shared_resources/globals.h"
#include "../shared_resources/SharedDataBuffer.h"

/**
 * @brief Network Task State Machine States
 */
enum class NetworkState {
    BOOT,           // Waiting for system initialization
    INIT,           // Initialize WiFi hardware
    CONNECTING,     // Initial 30-second connection attempt
    CONNECTED,      // Successfully connected, sending data
    DISCONNECTED,   // Connection lost, attempting to reconnect
    NO_NETWORK      // No network available, stopped trying
};

/**
 * @brief Network Task - Manages WiFi connectivity and data transmission
 *
 * This task handles:
 * - WiFi connection management with configurable static IP
 * - Connection status indication via LED on pin 15
 * - Periodic transmission of sensor data to RPi server
 * - Automatic reconnection logic
 */
class NetworkTask {
public:
    NetworkTask();
    ~NetworkTask();

    /**
     * @brief Setup function to initialize the network task
     * Called from main.cpp setup()
     */
    void setup();

    /**
     * @brief FreeRTOS task wrapper function
     * @param param Pointer to NetworkTask instance
     */
    static void runNetworkTaskWrapper(void* param);

    /**
     * @brief Get current network state
     */
    NetworkState getState() const { return currentState; }

    /**
     * @brief Check if currently connected to WiFi
     */
    bool isConnected() const { return currentState == NetworkState::CONNECTED; }

    /**
     * @brief Get the local IP address (if connected)
     */
    IPAddress getLocalIP() const;

private:
    /**
     * @brief Main state machine dispatcher
     */
    void runNetworkTask();

    /**
     * @brief State machine functions
     */
    void run_boot();
    void run_init();
    void run_connecting();
    void run_connected();
    void run_disconnected();
    void run_no_network();

    /**
     * @brief Helper functions
     */
    void updateConnectionLED();
    void configureStaticIP();
    bool attemptConnection();
    void sendSensorData();
    String formatSensorDataJSON(const SensorData& data);

    /**
     * @brief LED control for connection status
     */
    void ledOff();
    void ledOn();
    void ledFlash();

    // State tracking
    NetworkState currentState;
    NetworkState previousState;

    // WiFi configuration
    const char* ssid;
    const char* password;
    IPAddress staticIP;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress primaryDNS;
    IPAddress secondaryDNS;

    // Server configuration
    IPAddress serverIP;
    uint16_t serverPort;

    // UDP client for data transmission
    WiFiUDP udp;

    // Timing variables
    unsigned long stateEntryTime;          // Time when current state was entered
    unsigned long lastConnectionAttempt;   // Time of last connection attempt
    unsigned long lastDataSend;            // Time of last data transmission
    unsigned long lastLEDToggle;           // For LED flashing

    // Connection management
    bool initialConnectionAttempted;       // Track if initial 30s attempt completed
    bool networkAvailable;                 // Assume network exists after first success

    // LED state
    const int LED_PIN = 15;
    bool ledState;

    // Rate tracking (for debug)
    unsigned long lastFreqPrintTime;
    unsigned int updateCount;
    float state_machine_run_freq;
    unsigned int dataSendCount;
    float data_send_freq;
};

#endif // NETWORK_TASK_H
