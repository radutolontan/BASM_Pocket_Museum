#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

#include "shared_resources/globals.h"

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Display states
enum class DisplayState {
    BOOT,             // Waiting for INIT
    INIT,             // Sets up LEDs, shows Git SHA
    DISPLAY_PRESSURE, // Displays pressure reading via NeoPixels
    DISPLAY_ACCEL     // (Planned) display accel vector or status
};

// DisplayTask class handles the display state machine
class DisplayTask {
public:
    DisplayTask();

    // Initializes the Display Task (setup)
    void setupDisplayTask();

    // FreeRTOS-compatible entry point
    static void runDisplayTaskWrapper(void* param); 

    // Runs the Display Task state machine
    void runDisplayTask();

    // Safely request a state change from other modules
    void setDisplayState(DisplayState newState);

    // Public Variables
    DisplayState current_state;

private:
    Adafruit_NeoPixel strip;
    std::vector<uint32_t> colors_lib;  
    unsigned long lastStateChange;
    unsigned long lastUpdateTime;

    // Button debounce state
    bool stableButtonState = LOW;
    unsigned long lastButtonChange = 0;

    // Button helper methods
    bool debounceButton(bool rawState);
    void cycleDisplayState();

    // State handling methods
    void run_boot();
    void run_init();
    void run_display_pressure();
    void run_display_accel();

    // Helper methods
    void displayPressure(float pressure);
    void displayGitShaPattern();
    uint32_t getRandomColor();
    void import_colorlib();

    // Static helper function (only visible inside cpp file)
    static uint32_t hashStringToSeed(const char* str);
};

#endif // DISPLAY_TASK_H
