#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

#include "shared_resources/globals.h"

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Display states
enum class DisplayState {
    BOOT,                   // Waiting for INIT
    INIT,                   // Sets up LEDs, shows Git SHA
    DISPLAY_PRESSURE,       // Displays pressure reading (SCALAR)
    DISPLAY_TEMP,           // Displays temperature reading (SCALAR)
    DISPLAY_LUX,            // Displays light intensity reading (SCALAR)
    DISPLAY_VOLUME,         // Displays sound volume (SCALAR)
    DISPLAY_ACCEL,          // Displays acceleration reading (VECTOR)
    DISPLAY_MAG,            // Displays magnetic field strength (VECTOR)
    DISPLAY_ROT_VEL         // Displays rotational velocity
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

    // Method to safely access DisplayTask state
    DisplayState getDisplayState() const;

private:
    Adafruit_NeoPixel strip;
    std::vector<uint32_t> colors_lib;  
    unsigned long lastStateChange;
    unsigned long lastUpdateTime;

    DisplayState current_state;

    // FOR TRACKING ACTUAL UPDATE TIME
    unsigned long lastFreqPrintTime = 0;   // for printing every 1 second
    unsigned int updateCount = 0;          // count of READ executions


    // Button debounce state
    bool stableButtonState = LOW;
    unsigned long lastButtonChange = 0;
    

    // Button helper methods
    bool debounceButton(bool rawState);
    void cycleDisplayState();

    // LED mapping (logical index → physical LED)
    static const int LED_MAPPING[NEOPIXEL_COUNT];

    // State handling methods
    void run_boot();
    void run_init();
    void run_display_pressure();
    void run_display_temp();
    void run_display_lux();
    void run_display_volume();
    void run_display_accel();
    void run_display_mag_field();
    void run_display_rot_vel();

    // Segment handling methods
    void updateModeDisplay(); // Manages the MODE DISPLAY
    void updateMagnitudeDisplay(float value, float minValue, float maxValue); // Manages the MAGNITUDE DISPLAY (currently operates a normalized VU-METER Only)

    // Helper methods
    void setLogicalPixel(int logicalIndex, uint32_t color); // Transposes logical to physical pixels
    uint32_t getMagnitudeColor(float normalized, int ledIndex); // returns a color on a green-yellow-red scale for an input between 0 and 1 and an LEDIndex
    void displayGitShaPattern();
    uint32_t applyBreathing(uint32_t baseColor, uint32_t now);
    void turnDisplayOFF();
    uint32_t getRandomColor();
    void import_colorlib();

    // Static helper function (only visible inside cpp file)
    static uint32_t hashStringToSeed(const char* str);
};

#endif // DISPLAY_TASK_H
