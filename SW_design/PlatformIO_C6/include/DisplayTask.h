#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Display states
enum class DisplayState {
    BOOT,
    INIT,
    DISPLAY_SENSE,
    DISPLAY_SHOW,
    SLEEP
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

    // State handling methods
    void run_boot();
    void run_init();
    void run_display_sense();
    void run_display_show();
    void run_display_sleep();

    // Helper methods
    void displayPressure(float pressure);
    void displayGitShaPattern();
    uint32_t getRandomColor();
    void import_colorlib();

    // Static helper function (only visible inside cpp file)
    static uint32_t hashStringToSeed(const char* str);
};

#endif // DISPLAY_TASK_H
