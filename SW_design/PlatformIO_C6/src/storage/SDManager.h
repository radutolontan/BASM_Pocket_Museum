#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"  
#include <functional>

// Define your SDRequest structure (customize as needed)
struct SDRequest {
    enum class Type { READ, WRITE, ENSURE_FILE_EXISTS } type;
    char filename[64];
    uint8_t* data;
    size_t length;
    std::function<void(bool success, uint8_t* data, size_t length)> callback;
};

enum class SDState {
    BOOT = 0,            // Initial one-time setup & checks
    WAIT_FOR_INSERT = 1, // No card present, ready for action
    MOUNTING = 2,        // Mounting card after insertion
    READY = 3,           // Card is mounted and OPERATIONAL
    UNMOUNTING = 4,      // Clean unmount on removal
    ERROR = 5            // Mount / unexpected failure
};

class SDManager {
public:
    SDManager() = default;

    // Initialize the SDManager Task
    void setupSDManager();

    // Runs the SDManager State Machine
    void runSDManager();

    // FreeRTOS-compatible entry point
    static void runSDManagerWrapper(void* param);

    // Safely request a state change from other modules
    void setSDState(SDState new_state);

    // Public interface to enqueue SD requests
    bool enqueueRequest(const SDRequest& req);

    // Public method which tracks readiness of SDManager
    bool isReady() const;

private:
    // Helper functions
    bool mountCard();
    void unmountCard();

    // SD Card read/write protector
    void handleRequest(const SDRequest& req);

    // State handling methods
    void run_boot();
    void run_wait_for_insert();
    void run_mounting();
    void run_ready();
    void run_unmounting();
    void run_error();


    SDState current_state;
    QueueHandle_t sdQueue = nullptr;

    // Debounce variables (will be over-written at BOOT)
    unsigned long lastDetectChange = 0;
    bool stableCardInserted = false;
    // Non-blocking SD Card Detect debounce
    bool debounceCardDetect(bool rawState);
};
