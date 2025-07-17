#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"  
#include <functional>

// Define your SDRequest structure (customize as needed)
struct SDRequest {
    enum class Type { READ, WRITE } type;
    const char* filename;
    uint8_t* data;
    size_t length;
    std::function<void(bool success, uint8_t* data, size_t length)> callback;
};

enum class SDState {
    BOOT,            // Initial one-time setup & checks
    WAIT_FOR_INSERT, // No card present, ready for action
    MOUNTING,        // Mounting card after insertion
    READY,           // Card is mounted and OPERATIONAL
    UNMOUNTING,      // Clean unmount on removal
    ERROR            // Mount / unexpected failure
};

class SDManager {
public:
    SDManager() = default;

    // Initialize the SDManager Task
    void setupSDTask();

    // Runs the SDManager State Machine
    void runSDTask();

    // FreeRTOS-compatible entry point
    static void runSDTaskWrapper(void* param);

    // Safely request a state change from other modules
    void setSDState(SDState new_state);

    // Public interface to enqueue SD requests
    bool enqueueRequest(const SDRequest& req);

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
