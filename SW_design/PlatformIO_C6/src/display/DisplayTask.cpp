#include "display/DisplayTask.h"
#include "shared_resources/SharedDataBuffer.h"
#include "shared_resources/globals.h"
#include "shared_resources/global_functions.h"
#include "shared_resources/global_debug.h"

#include <Adafruit_NeoPixel.h>
#include <random>
#include <bitset>
#include <cmath>
#include <cstdint>

#define GPIO_DEBOUNCE_DELAY 50 // [ms]

// CLASS Constructor
DisplayTask::DisplayTask() 
: strip(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800) // initialize SK6805-EC20 strip
{

}

// Logical → physical LED mapping
// ================= NORMAL CONFIG ================= //
// MODE_DISPLAY = LEDs #0, #1
// DIRECTION_DISPLAY = LEDs #3 [X], #2 [Y], #4 [Z] 
// MAGNITUDE_DISPLAY = LEDs #8, #7, #6, #5
// ================== LITE CONFIG ================== //
// MODE_DISPLAY = LEDs #0, #1
// DIRECTION_DISPLAY = INOP
// MAGNITUDE_DISPLAY = LEDs #8, #7, #6, #5, #4, #3
    const int DisplayTask::LED_MAPPING[NEOPIXEL_COUNT] = {    
    0,  // logical 0 → physical LED 0  (MODE)
    1,  // logical 1 → physical LED 1  (MODE)
    2,  // logical 2 → physical LED 2  (INOP - DIRECTION)
    8,  // logical 3 → physical LED 8  (MAGNITUDE)
    7,  // logical 4 → physical LED 7  (MAGNITUDE)
    6,  // logical 5 → physical LED 6  (MAGNITUDE)
    5,  // logical 6 → physical LED 5  (MAGNITUDE)
    4,  // logical 7 → physical LED 4  (MAGNITUDE)
    3,  // logical 8 → physical LED 3  (MAGNITUDE)
};

void DisplayTask::setupDisplayTask() {
    strip.begin();  // Initialize the NeoPixel library
    setDisplayState(DisplayState::BOOT);
    // GPIO for Push-button which toggles display mode
    pinMode(DISPLAY_MODE_PUSHBUTTON_PIN, INPUT);
}

void DisplayTask::setDisplayState(DisplayState new_state) {
    // Future: Add safety checks or mutex here
    current_state = new_state;
}

DisplayState DisplayTask::getDisplayState() const {
    // Return the current_state
    return current_state;
}

void DisplayTask::runDisplayTaskWrapper(void* param) {
    DisplayTask* self = static_cast<DisplayTask*>(param);
    for (;;) {
        self->runDisplayTask();
        // DISPLAYTASK STATE MACHINE TIMING
        vTaskDelay(pdMS_TO_TICKS(1000 / TASK_RATE_DISPLAY));
    }
}

void DisplayTask::runDisplayTask() {
    // Increment the read count
    updateCount++;
    // Check if BMS is still latched
    if (!g_bmsLatched) {
        // Power not latched → shut off display
        turnDisplayOFF();
        // Set DisplayMode to Boot to avoid other states overwriting the shutdown command
        setDisplayState(DisplayState::BOOT);
    }
    // Check the status of the Mode Display pushbutton
    bool rawState = digitalRead(DISPLAY_MODE_PUSHBUTTON_PIN); 
    if (debounceButton(rawState)) {  // rising edge detected
        cycleDisplayState();
    }
    // Run the state machine
    switch (current_state) {
        case DisplayState::BOOT:{
            run_boot();
            break;
        }
        case DisplayState::INIT:{
            run_init();
            break;
        }
        case DisplayState::DISPLAY_PRESSURE:{
            run_display_pressure();
            break;
        }
        case DisplayState::DISPLAY_TEMP:{
            run_display_temp();
            break;
        }
        case DisplayState::DISPLAY_LUX:{
            run_display_lux();
            break;
        }
        case DisplayState::DISPLAY_VOLUME:{
            run_display_volume();
            break;
        }
        case DisplayState::DISPLAY_ACCEL:{
            run_display_accel();
            break;
        }
        case DisplayState::DISPLAY_MAG:{
            run_display_mag_field();
            break;
        }
        case DisplayState::DISPLAY_ROT_VEL:{
            run_display_rot_vel();
            break;
        }
    }
    lastUpdateTime = millis();
    // Print frequency every 1 second
    unsigned long now = millis();
    if (now - lastFreqPrintTime >= 1000) {
        float freq = updateCount / ((now - lastFreqPrintTime) / 1000.0f); // Hz
        RATES_PRINT("[DisplayTask] Actual update frequency: ");
        RATES_PRINT(freq);
        RATES_PRINTLN(" Hz");

        // Reset counters
        updateCount = 0;
        lastFreqPrintTime = now;
    }
}

// ================================================== //
// ============= STATE HANDLING METHODS ============= //
// ================================================== //

void DisplayTask::run_boot(){
    // Check if BMS is Ready
    if (g_bmsLatched) {
        // Transition to INIT
        setDisplayState(DisplayState::INIT);
        // Import color-lib for otehr methods to use
        import_colorlib();                          // Import color library
        // Initialize button state
        stableButtonState = digitalRead(DISPLAY_MODE_PUSHBUTTON_PIN);
        lastButtonChange = millis();
    }
};

void DisplayTask::run_init(){
    strip.setBrightness(NEOPIXEL_BRIGHTNESS);   // Set brightness 
    strip.show();                               // Update strip to apply brightness and clear LEDs
    // Display the GIT SHA Pattern on the display to confirm correct SW version
    displayGitShaPattern();                     // Display GIT SHA to confirm correct SW version
    vTaskDelay(pdMS_TO_TICKS(3000));            // To view the GITSHA
    // When done, trandisition to DisplayState::DISPLAY_SENSOR
    setDisplayState(DisplayState::DISPLAY_PRESSURE);

    //  ====================== DEBUG ===========================
    // TURN OFF PIXEL #2
    strip.setPixelColor(2, colors_lib[0]);
};

void DisplayTask::run_display_pressure(){
    // Update Mode Display
    updateModeDisplay();
    // Get Pressure Reading
    auto readings = SharedBuffer::getReadings();
    if (!readings.empty()) {
        const SensorData& latest = readings.back();
        // Update Magnitude Display
        updateMagnitudeDisplay(latest.pressure, VU_MIN_PRESS, VU_MAX_PRESS);
        // Send All Data to LED Strip
        strip.show();
    }
};

void DisplayTask::run_display_temp(){
    // Update Mode Display
    updateModeDisplay();
    // Get Temperature Reading
    auto readings = SharedBuffer::getReadings();
    // Send All Data to LED Strip
    strip.show();
        if (!readings.empty()) {
        const SensorData& latest = readings.back();
        // Update Magnitude Display
        updateMagnitudeDisplay(latest.temperature, VU_MIN_TEMP, VU_MAX_TEMP);
        // Send All Data to LED Strip
        strip.show();
    }
};

void DisplayTask::run_display_lux(){
    // Update Mode Display
    updateModeDisplay();
    // Get Light Intensity Reading
    auto readings = SharedBuffer::getReadings();
    // Send All Data to LED Strip
    strip.show();
        if (!readings.empty()) {
        const SensorData& latest = readings.back();
        // Update Magnitude Display
        updateMagnitudeDisplay(latest.light_intensity, VU_MIN_LUX, VU_MAX_LUX);
        // Send All Data to LED Strip
        strip.show();
    }
};

void DisplayTask::run_display_volume(){
    // Update Mode Display
    updateModeDisplay();
    // Get Volume Reading
    auto readings = SharedBuffer::getReadings();
    // Send All Data to LED Strip
    strip.show();
        if (!readings.empty()) {
        const SensorData& latest = readings.back();
        // Update Magnitude Display
        updateMagnitudeDisplay(0, VU_MIN_VOL, VU_MAX_VOL);
        // Send All Data to LED Strip
        strip.show();
    }
};

void DisplayTask::run_display_accel(){
    // Update Mode Display
    updateModeDisplay();
    // Get Acceleration Reading
    auto readings = SharedBuffer::getReadings();
    // Send All Data to LED Strip
    strip.show();
        if (!readings.empty()) {
        const SensorData& latest = readings.back();
        // Update Magnitude Display
        updateMagnitudeDisplay(latest.accel_norm, VU_MIN_ACCEL, VU_MAX_ACCEL);
        // Send All Data to LED Strip
        strip.show();
    }
};

void DisplayTask::run_display_mag_field(){
    // Update Mode Display
    updateModeDisplay();
    // Get Magnetic Field Reading
    auto readings = SharedBuffer::getReadings();
    // Send All Data to LED Strip
    strip.show();
        if (!readings.empty()) {
        const SensorData& latest = readings.back();
        // Update Magnitude Display
        updateMagnitudeDisplay(latest.mag_norm, VU_MIN_MAG, VU_MAX_MAG);
        // Send All Data to LED Strip
        strip.show();
    }
};

void DisplayTask::run_display_rot_vel(){
    // Update Mode Display
    updateModeDisplay();
    // Get Angular Velocity Reading
    auto readings = SharedBuffer::getReadings();
    // Send All Data to LED Strip
    strip.show();
        if (!readings.empty()) {
        const SensorData& latest = readings.back();
        // Update Magnitude Display
        updateMagnitudeDisplay(latest.gyro_norm, VU_MIN_ROT, VU_MAX_ROT);
        // Send All Data to LED Strip
        strip.show();
    }
};

// ================================================== //
// ============ BUTTON HANDLING METHODS ============= //
// ================================================== //

bool DisplayTask::debounceButton(bool rawState) {
    // Save the current stableButtonState (since the function passes it by reference)
    bool prevState = stableButtonState;
    bool debouncedState = helperdebounceButton(rawState, stableButtonState, lastButtonChange, GPIO_DEBOUNCE_DELAY);
    // Return true only if we have a rising edge (LOW -> HIGH)
    return (prevState == LOW && debouncedState == HIGH);
}

void DisplayTask::cycleDisplayState() {
    // Before switching, compute the aggregate sensor data for the last time
    SensorStats stats = SharedBuffer::getAggregatedStats();
    switch (current_state) {
        // If in PRESSURE DISPLAY -> TEMPERATURE DISPLAY
        case DisplayState::DISPLAY_PRESSURE:
            setDisplayState(DisplayState::DISPLAY_TEMP);
            break;
        // If in TEMPERATURE DISPLAY -> LIGHT INTENSITY (LUX) DISPLAY
        case DisplayState::DISPLAY_TEMP:
            setDisplayState(DisplayState::DISPLAY_LUX);
            break;
        // If in LIGHT INTENSITY (LUX) DISPLAY -> VOLUME DISPLAY
        case DisplayState::DISPLAY_LUX:
            setDisplayState(DisplayState::DISPLAY_VOLUME);
            break;
        // If in VOLUME DISPLAY -> ACCELERATION DISPLAY
        case DisplayState::DISPLAY_VOLUME:
            setDisplayState(DisplayState::DISPLAY_ACCEL);
            break;
        // If in ACCEL DISPLAY -> MAGNETIC FIELD DISPLAY
        case DisplayState::DISPLAY_ACCEL:
            setDisplayState(DisplayState::DISPLAY_MAG);
            break;
        // If in MAGNETIC FIELD DISPLAY -> ROTATION VELOCITY DISPLAY
        case DisplayState::DISPLAY_MAG:
            setDisplayState(DisplayState::DISPLAY_ROT_VEL);
            break;
        // If in ROTATION VELOCITY DISPLAY -> PRESSURE DISPLAY
        case DisplayState::DISPLAY_ROT_VEL:
            setDisplayState(DisplayState::DISPLAY_PRESSURE);
            break;
        default:
            // Skip state change in BOOT or INIT
            break;
    }
    // Reset the aggregate data
    SharedBuffer::resetAggregates();

    Serial.printf("[DisplayTask] - Switched to state: %d\n", static_cast<int>(current_state));
}

// ================================================== //
// ====== DISPLAY SEGMENT MANAGEMENT METHODS ======== //
// ================================================== //

void DisplayTask::updateModeDisplay() {
    uint32_t baseColor = 0;
    // Pick the color based on the ModeDisplay
    switch (current_state) {
        case DisplayState::DISPLAY_PRESSURE:{
            baseColor = colors_lib[2]; // Yellow
            break;
        }
        case DisplayState::DISPLAY_TEMP:{
            baseColor = colors_lib[4]; // Red
            break;
        }
        case DisplayState::DISPLAY_LUX:{
            baseColor = colors_lib[7]; // White
            break;
        }
        case DisplayState::DISPLAY_VOLUME:{
            baseColor = colors_lib[5]; // Purple
            break;
        }
        case DisplayState::DISPLAY_ACCEL:{
            baseColor = colors_lib[3]; // Green
            break;
        }
        case DisplayState::DISPLAY_MAG:{
            baseColor = colors_lib[2]; // Yellow
            break;
        }
        case DisplayState::DISPLAY_ROT_VEL:{
            baseColor = colors_lib[6]; // Orange
            break;
        }
        default:
            baseColor = colors_lib[0]; // OFF fallback
            break;
        }
    // Show base color (for Display Mode) (w. Breathing Effect)
    uint32_t now = millis();
    for (int i = 0; i < MODE_DISPLAY_COUNT; i++) {
        uint32_t color = applyBreathing(baseColor, now);
        setLogicalPixel(MODE_DISPLAY_OFFSET + i, color);
    }
    // TO-DO: ADD OVERRIDE FOR MONITORING OTHER STATE MACHINES
}

void DisplayTask::updateMagnitudeDisplay(float value, float minValue, float maxValue) {
    // Clamp and normalize 0..1
    float normalized = (value - minValue) / (maxValue - minValue);
    normalized = fmax(0.0f, fmin(1.0f, normalized));
    // For ALL LEDs in the MAGNITUDE_DISPLAY, get a magnitude color, and send it to the right index
    for (int i = 0; i < MAGNITUDE_DISPLAY_COUNT; i++) {
        uint32_t color = getMagnitudeColor(normalized, i);
        setLogicalPixel(MAGNITUDE_DISPLAY_OFFSET + i, color);
    }
}

// ================================================== //
// ================== HELPER METHODS ================ //
// ================================================== //

void DisplayTask::setLogicalPixel(int logicalIndex, uint32_t color) {
    if (logicalIndex >= 0 && logicalIndex < NEOPIXEL_COUNT) {
        int physicalIndex = LED_MAPPING[logicalIndex];
        strip.setPixelColor(physicalIndex, color);
    }
}

uint32_t DisplayTask::getMagnitudeColor(float normalized, int ledIndex) {
    // LED threshold
    float threshold = (float)(ledIndex + 1) / MAGNITUDE_DISPLAY_COUNT;

    if (normalized < threshold) return colors_lib[0]; // OFF

    // Determine gradient position 0..1 across the LEDs
    float t = (float)ledIndex / (MAGNITUDE_DISPLAY_COUNT - 1); 

    // Interpolate RGB
    uint8_t r, g, b;

    if (t < 0.5f) {
        // Green → Yellow
        float f = t / 0.5f;
        r = (uint8_t)(f * 255);
        g = 255;
        b = 0;
    } else {
        // Yellow → Red
        float f = (t - 0.5f) / 0.5f;
        r = 255;
        g = (uint8_t)((1.0f - f) * 255);
        b = 0;
    }

    return strip.Color(r, g, b);
}

void DisplayTask::turnDisplayOFF() {
    // Set all pixels to OFF
    for (int LED = 0; LED < NEOPIXEL_COUNT; LED++) {
        strip.setPixelColor(LED, 0, 0, 0); // RGB = 0,0,0 → off
    }
    strip.show(); 
    strip.clear();
}

void DisplayTask::import_colorlib() {
// Initialize Color library
    colors_lib.push_back(Adafruit_NeoPixel::Color(0, 0, 0));        // 0 OFF 
    colors_lib.push_back(Adafruit_NeoPixel::Color(0, 0, 255));      // 1 Blue
    colors_lib.push_back(Adafruit_NeoPixel::Color(250, 255, 0));    // 2 Yellow
    colors_lib.push_back(Adafruit_NeoPixel::Color(0, 255, 0));      // 3 Green
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 0, 0));      // 4 Red
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 0, 255));    // 5 Purple
    colors_lib.push_back(Adafruit_NeoPixel::Color(255,  80, 0));    // 6 Orange
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 255, 255));  // 7 White
}

// ================================================== //
// ===================== EFFECTS ==================== //
// ================================================== //

uint32_t DisplayTask::applyBreathing(uint32_t baseColor, uint32_t now) {
    // Breathing period (ms)
    const uint32_t period = 4000; // 3 seconds for full in/out cycle
    // Compute phase 0..2π
    float phase = (2.0f * M_PI * (now % period)) / period;
    // Compute scale
    float minLevel = 0.1f;  // 10% brightness (instead of 65%)
    float maxLevel = 1.0f;  // full brightness
    float scale = minLevel + (maxLevel - minLevel) * (sin(phase) * 0.5f + 0.5f);
    // Extract RGB
    uint8_t r = (uint8_t)((baseColor >> 16) & 0xFF);
    uint8_t g = (uint8_t)((baseColor >>  8) & 0xFF);
    uint8_t b = (uint8_t)((baseColor >>  0) & 0xFF);
    // Apply brightness scale
    r = (uint8_t)(r * scale);
    g = (uint8_t)(g * scale);
    b = (uint8_t)(b * scale);
    return strip.Color(r, g, b);
}


/* Takes a string - GIT_SHA ;
 Takes strip length - NEOPIXEL_COUNT ; 
 Always produces the same color sequence for the same string and strip length
*/

void DisplayTask::displayGitShaPattern() {
    uint32_t seed = DisplayTask::hashStringToSeed(GIT_SHA);
    randomSeed(seed);
    for (uint16_t i = 0; i < NEOPIXEL_COUNT; i++) {
        strip.setPixelColor(i, getRandomColor());
    }
    strip.show();
}

uint32_t DisplayTask::getRandomColor() {
    uint8_t step = 85;
    // Generate R, G, B components between 0 and 255, discretized to the closest multiple of 85
    uint8_t r = (random(0, 256) / step) * step;
    uint8_t g = (random(0, 256) / step) * step;
    uint8_t b = (random(0, 256) / step) * step;
    return strip.Color(r, g, b);
}

// FREE HELPER FUNCTIONS
uint32_t  DisplayTask::hashStringToSeed(const char* str) {
    uint32_t seed = 5381;
    int c;
    while ((c = *str++)) {
        seed = ((seed << 5) + seed) + c;
    }
    return seed;
}
