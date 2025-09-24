#pragma once
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Preferences.h>

constexpr int MAX_NODES = 4;

struct NodePayload {
    uint8_t nodeID;
    int value;
};

// ================== Node ==================
class Node {
public:
    Node(const uint8_t hub_mac[6], uint8_t id, unsigned long sendIntervalMs);

    void begin();
    void loop();
    void setNextValue(int value);

private:
    uint8_t hubMac[6];
    uint8_t nodeID;
    unsigned long lastSend;
    unsigned long sendInterval;
    int nextValue;
    bool hasNewValue;

    void flashLED();
    static void onDataSentStatic(
    #if ESP_IDF_VERSION_MAJOR >= 5
        const wifi_tx_info_t *info,
    #else
        const uint8_t *mac_addr,
    #endif
        esp_now_send_status_t status);
};

// ================== Hub ==================
class Hub {
public:
    Hub(uint8_t maxNodes_);
    void begin();
    void loop();

private:
    uint8_t maxNodes;
    int nodeCounters[MAX_NODES];      // Final counters (what you display)
    int baseOffsets[MAX_NODES];       // Carry-over offset
    int lastSeenValues[MAX_NODES];    // Last raw values received
    bool testMode = false;            // If true → ignore carry-over

    Preferences prefs;
    static Hub* activeHub;

    void flashLED();
    void handleRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len);

    void loadFromNVM();
    void saveToNVM();
    void resetBaseOffsets();

    static void onDataRecvStatic(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len);
};