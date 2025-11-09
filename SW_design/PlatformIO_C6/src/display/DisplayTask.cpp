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
// ================= VU DISPLAY MODE ================= //
// MODE_DISPLAY = LEDs #0, #1
// DIRECTION_DISPLAY = LED #2 (INOP)
// MAGNITUDE_DISPLAY = LEDs #8, #7, #6, #5, #4, #3
    const int DisplayTask::LED_MAPPING_VU[NEOPIXEL_COUNT] = {
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

// ================= BINARY DISPLAY MODE ================= //
// MODE_DISPLAY = LEDs #0, #1
// DIRECTION_DISPLAY = LEDs #3 [X], #2 [Y], #4 [Z]
// MAGNITUDE_DISPLAY = LEDs #8, #7, #6, #5
    const int DisplayTask::LED_MAPPING_BINARY[BINARY_NEOPIXEL_COUNT] = {
    0,  // logical 0 → physical LED 0  (MODE)
    1,  // logical 1 → physical LED 1  (MODE)
    3,  // logical 2 → physical LED 3  (DIRECTION X)
    2,  // logical 3 → physical LED 2  (DIRECTION Y)
    4,  // logical 4 → physical LED 4  (DIRECTION Z)
    8,  // logical 5 → physical LED 8  (MAGNITUDE)
    7,  // logical 6 → physical LED 7  (MAGNITUDE)
    6,  // logical 7 → physical LED 6  (MAGNITUDE)
    5,  // logical 8 → physical LED 5  (MAGNITUDE)
};

void DisplayTask::setupDisplayTask(BMSTask* bms) {
    strip.begin();  // Initialize the NeoPixel library
    setDisplayState(DisplayState::BOOT);
    // GPIO for Push-button which toggles display mode
    pinMode(DISPLAY_MODE_PUSHBUTTON_PIN, INPUT);
    // Store BMS Task Pointer
    this->bmsTask = bms;
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

    // Initialize the tick count for periodic execution
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(1000 / TASK_RATE_DISPLAY);

    for (;;) {
        self->runDisplayTask();
        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}


void DisplayTask::runDisplayTask() {
    // Check if BMS is still latched
    if (bmsTask && !bmsTask->isLatched()) {
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
    // Print frequency every 1 second
    #if DEBUG_TASK_RATES
        // Increment the read count
        updateCount++;
        unsigned long now = millis();
        if (now - lastFreqPrintTime >= 10000) {
            state_machine_run_freq = updateCount / ((now - lastFreqPrintTime) / 1000.0f); // Hz
            RATES_PRINT("[DisplayTask] Actual update frequency: ");
            RATES_PRINT(state_machine_run_freq);
            RATES_PRINTLN(" Hz");

            // Reset counters
            updateCount = 0;
            lastFreqPrintTime = now;
        }
    #endif
}

// ================================================== //
// ============= STATE HANDLING METHODS ============= //
// ================================================== //

void DisplayTask::run_boot(){
    // Check if BMS is Ready
    if (bmsTask && bmsTask->isLatched()) {
        // Check DISPLAY_MODE_PUSHBUTTON_PIN to select display mode
        // HIGH = BINARY_DISPLAY, LOW = VU_DISPLAY
        bool buttonState = digitalRead(DISPLAY_MODE_PUSHBUTTON_PIN);
        if (buttonState == HIGH) {
            display_mode = DisplayMode::BINARY_DISPLAY;
        } else {
            display_mode = DisplayMode::VU_DISPLAY;
        }

        // Transition to INIT
        setDisplayState(DisplayState::INIT);
        // Import color-lib for other methods to use
        import_colorlib();                          // Import color library
        // Initialize button state
        stableButtonState = buttonState;
        lastButtonChange = millis();
    }
};

void DisplayTask::run_init(){
    strip.setBrightness(NEOPIXEL_BRIGHTNESS);   // Set brightness
    strip.show();                               // Update strip to apply brightness and clear LEDs
    // Display the GIT SHA Pattern on the display to confirm correct SW version
    displayGitShaPattern();                     // Display GIT SHA to confirm correct SW version
    vTaskDelay(pdMS_TO_TICKS(3000));            // To view the GITSHA

    // Cycle display state to initialize at first sensor state and load coefficients
    cycleDisplayState();

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
        updateMagnitudeDisplay(latest.pressure);
        // Update Direction Display (scalar quantity - turn off direction display)
        updateDirectionDisplay(0, 0, 0);
    }
    // Send All Data to LED Strip
    strip.show();
};

void DisplayTask::run_display_temp(){
    // Update Mode Display
    updateModeDisplay();
    // Get Temperature Reading
    auto readings = SharedBuffer::getReadings();
    if (!readings.empty()) {
        const SensorData& latest = readings.back();
        // Update Magnitude Display
        updateMagnitudeDisplay(latest.temperature);
        // Update Direction Display (scalar quantity - turn off direction display)
        updateDirectionDisplay(0, 0, 0);
    }
    // Send All Data to LED Strip
    strip.show();
};

void DisplayTask::run_display_lux(){
    // Update Mode Display
    updateModeDisplay();
    // Get Light Intensity Reading
    auto readings = SharedBuffer::getReadings();
    if (!readings.empty()) {
        const SensorData& latest = readings.back();
        // Update Magnitude Display
        updateMagnitudeDisplay(latest.light_intensity);
        // Update Direction Display (scalar quantity - turn off direction display)
        updateDirectionDisplay(0, 0, 0);
    }
    // Send All Data to LED Strip
    strip.show();
};

void DisplayTask::run_display_volume(){
    // Update Mode Display
    updateModeDisplay();
    // Get Volume Reading
    auto readings = SharedBuffer::getReadings();
    if (!readings.empty()) {
        const SensorData& latest = readings.back();
        // Update Magnitude Display
        updateMagnitudeDisplay(latest.volume_rms);
        // Update Direction Display (scalar quantity - turn off direction display)
        updateDirectionDisplay(0, 0, 0);
    }
    // Send All Data to LED Strip
    strip.show();
};

void DisplayTask::run_display_accel(){
    // Update Mode Display
    updateModeDisplay();
    // Get Acceleration Reading
    auto readings = SharedBuffer::getReadings();
    if (!readings.empty()) {
        const SensorData& latest = readings.back();
        // Update Magnitude Display
        updateMagnitudeDisplay(latest.accel_norm);
        // Update Direction Display (vector quantity - show x, y, z components)
        updateDirectionDisplay(latest.accel_x, latest.accel_y, latest.accel_z);
    }
    // Send All Data to LED Strip
    strip.show();
};

void DisplayTask::run_display_mag_field(){
    // Update Mode Display
    updateModeDisplay();
    // Get Magnetic Field Reading
    auto readings = SharedBuffer::getReadings();
    if (!readings.empty()) {
        const SensorData& latest = readings.back();
        // Update Magnitude Display
        updateMagnitudeDisplay(latest.mag_norm);
        // Update Direction Display (vector quantity - show x, y, z components)
        updateDirectionDisplay(latest.mag_x, latest.mag_y, latest.mag_z);
    }
    // Send All Data to LED Strip
    strip.show();
};

void DisplayTask::run_display_rot_vel(){
    // Update Mode Display
    updateModeDisplay();
    // Get Angular Velocity Reading
    auto readings = SharedBuffer::getReadings();
    if (!readings.empty()) {
        const SensorData& latest = readings.back();
        // Update Magnitude Display
        updateMagnitudeDisplay(latest.gyro_norm);
        // Update Direction Display (vector quantity - show x, y, z components)
        updateDirectionDisplay(latest.gyro_x, latest.gyro_y, latest.gyro_z);
    }
    // Send All Data to LED Strip
    strip.show();
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
    // Load scaling coefficients for the new state
    loadScalingCoefficients();
    // Reset the aggregate data
    SharedBuffer::resetAggregates();
}

void DisplayTask::loadScalingCoefficients() {
    // Load VU mode and binary mode coefficients based on current_state
    switch (current_state) {
        case DisplayState::DISPLAY_PRESSURE:
            vu_min_value = VU_MIN_PRESS;
            vu_max_value = VU_MAX_PRESS;
            binary_mag_order = BINARY_MAG_ORDER_PRESS;
            binary_dir_norm = 1.0f; // Not used for scalar
            break;
        case DisplayState::DISPLAY_TEMP:
            vu_min_value = VU_MIN_TEMP;
            vu_max_value = VU_MAX_TEMP;
            binary_mag_order = BINARY_MAG_ORDER_TEMP;
            binary_dir_norm = 1.0f; // Not used for scalar
            break;
        case DisplayState::DISPLAY_LUX:
            vu_min_value = VU_MIN_LUX;
            vu_max_value = VU_MAX_LUX;
            binary_mag_order = BINARY_MAG_ORDER_LUX;
            binary_dir_norm = 1.0f; // Not used for scalar
            break;
        case DisplayState::DISPLAY_VOLUME:
            vu_min_value = VU_MIN_VOL;
            vu_max_value = VU_MAX_VOL;
            binary_mag_order = BINARY_MAG_ORDER_VOL;
            binary_dir_norm = 1.0f; // Not used for scalar
            break;
        case DisplayState::DISPLAY_ACCEL:
            vu_min_value = VU_MIN_ACCEL;
            vu_max_value = VU_MAX_ACCEL;
            binary_mag_order = BINARY_MAG_ORDER_ACCEL;
            binary_dir_norm = BINARY_DIR_NORM_ACCEL;
            break;
        case DisplayState::DISPLAY_MAG:
            vu_min_value = VU_MIN_MAG;
            vu_max_value = VU_MAX_MAG;
            binary_mag_order = BINARY_MAG_ORDER_MAG;
            binary_dir_norm = BINARY_DIR_NORM_MAG;
            break;
        case DisplayState::DISPLAY_ROT_VEL:
            vu_min_value = VU_MIN_ROT;
            vu_max_value = VU_MAX_ROT;
            binary_mag_order = BINARY_MAG_ORDER_ROT;
            binary_dir_norm = BINARY_DIR_NORM_ROT;
            break;
        default:
            vu_min_value = 0.0f;
            vu_max_value = 1.0f;
            binary_mag_order = 1.0f;
            binary_dir_norm = 1.0f;
            break;
    }
}

// ================================================== //
// ====== DISPLAY SEGMENT MANAGEMENT METHODS ======== //
// ================================================== //

void DisplayTask::updateModeDisplay() {
    uint32_t baseColor = 0;
    // Pick the color based on the ModeDisplay
    switch (current_state) {
        case DisplayState::DISPLAY_PRESSURE:{
            baseColor = colors_lib[1]; // Blue
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
        // I
        if (i == 1 && bmsTask) {
            auto state = bmsTask->getChargeControllerState();
            // Default: keep color as-is
            int colorIndex = -1; 
            // Check if any of the special ChargeController states is active
            switch (state) {
                case ChargeControllerState::LOW_BATTERY:   colorIndex = 6; break; // ORANGE
                case ChargeControllerState::CHARGING:      colorIndex = 3; break; // e.g. GREEN
                case ChargeControllerState::DONE_CHARGING: colorIndex = 1; break; // e.g. BLUE
                default: break; // UNKNOWN / BATTERY_ONLY → no override
            }
            // Only apply override if the colorIndex had been changed
            if (colorIndex >= 0) {
                // Blink between chosen color and OFF
                color = ((now / 300) % 2 == 0) ? colors_lib[colorIndex] : colors_lib[0];
            }
        }

    setLogicalPixel(MODE_DISPLAY_OFFSET + i, color);
}

}

void DisplayTask::updateMagnitudeDisplay(float value) {
    if (display_mode == DisplayMode::VU_DISPLAY) {
        // VU-meter mode: traditional bar graph display
        // Use cached min/max values (loaded in loadScalingCoefficients)
        float normalized = (value - vu_min_value) / (vu_max_value - vu_min_value);
        normalized = fmax(0.0f, fmin(1.0f, normalized));

        // For ALL LEDs in the MAGNITUDE_DISPLAY, get a magnitude color, and send it to the right index
        for (int i = 0; i < MAGNITUDE_DISPLAY_COUNT; i++) {
            uint32_t color = getMagnitudeColor(normalized, i);
            setLogicalPixel(MAGNITUDE_DISPLAY_OFFSET + i, color);
        }
    } else if (display_mode == DisplayMode::BINARY_DISPLAY) {
        // Binary mode: encode actual integer value as binary across LEDs
        // Use cached order_of_magnitude (loaded in loadScalingCoefficients)
        float scaledValue = value * binary_mag_order;
        int32_t intValue = (int32_t)round(scaledValue);

        // Clamp to valid range [0, 4095] for 12-bit encoding
        uint32_t maxBinaryValue = (1 << (BINARY_MAGNITUDE_DISPLAY_COUNT * 3)) - 1;  // 4095
        uint32_t binaryValue = (uint32_t)fmax(0, fmin(intValue, (int32_t)maxBinaryValue));

        // Display each LED's portion of the binary value
        for (int i = 0; i < BINARY_MAGNITUDE_DISPLAY_COUNT; i++) {
            uint32_t color = getBinaryMagnitudeColor(binaryValue, i);
            setLogicalPixel(BINARY_MAGNITUDE_DISPLAY_OFFSET + i, color);
        }
    }
}

void DisplayTask::updateDirectionDisplay(float x, float y, float z) {
    if (display_mode == DisplayMode::VU_DISPLAY) {
        // In VU mode, turn off direction display
        for (int i = 0; i < DIRECTION_DISPLAY_COUNT; i++) {
            setLogicalPixel(DIRECTION_DISPLAY_OFFSET + i, colors_lib[0]); // OFF
        }
    } else if (display_mode == DisplayMode::BINARY_DISPLAY) {
        // In binary mode, display vector components
        // Direction display has 3 LEDs: [0]=X, [1]=Y, [2]=Z
        // Color mapping: RED (very negative) → GREEN (very positive)
        // Use cached normalization value (loaded in loadScalingCoefficients)

        if (BINARY_DIRECTION_DISPLAY_COUNT >= 3) {
            setLogicalPixel(BINARY_DIRECTION_DISPLAY_OFFSET + 0, getDirectionColor(x, binary_dir_norm)); // X
            setLogicalPixel(BINARY_DIRECTION_DISPLAY_OFFSET + 1, getDirectionColor(y, binary_dir_norm)); // Y
            setLogicalPixel(BINARY_DIRECTION_DISPLAY_OFFSET + 2, getDirectionColor(z, binary_dir_norm)); // Z
        }
    }
}

// ================================================== //
// ================== HELPER METHODS ================ //
// ================================================== //

void DisplayTask::setLogicalPixel(int logicalIndex, uint32_t color) {
    if (display_mode == DisplayMode::VU_DISPLAY) {
        if (logicalIndex >= 0 && logicalIndex < NEOPIXEL_COUNT) {
            int physicalIndex = LED_MAPPING_VU[logicalIndex];
            strip.setPixelColor(physicalIndex, color);
        }
    } else if (display_mode == DisplayMode::BINARY_DISPLAY) {
        if (logicalIndex >= 0 && logicalIndex < BINARY_NEOPIXEL_COUNT) {
            int physicalIndex = LED_MAPPING_BINARY[logicalIndex];
            strip.setPixelColor(physicalIndex, color);
        }
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

uint32_t DisplayTask::getBinaryMagnitudeColor(uint32_t value, int ledIndex) {
    // Each LED displays 3 bits from the value
    // LED 0 shows bits 0-2, LED 1 shows bits 3-5, etc.
    // Bit encoding: YELLOW = bit m, RED = bit m+1, BLUE = bit m+2
    // where m = ledIndex * 3

    int m = ledIndex * 3;

    // Extract the 3 bits for this LED
    bool yellowBit = (value >> m) & 1;        // bit m (LSB for this LED)
    bool redBit = (value >> (m + 1)) & 1;     // bit m+1
    bool blueBit = (value >> (m + 2)) & 1;    // bit m+2 (MSB for this LED)

    // If no bits are active, return OFF
    if (!yellowBit && !redBit && !blueBit) {
        return colors_lib[0]; // OFF
    }

    // Combine RGB components based on active bits
    uint8_t r = redBit ? 255 : 0;
    uint8_t g = yellowBit ? 255 : 0;  // Yellow requires both R and G
    uint8_t b = blueBit ? 255 : 0;

    // For YELLOW, we need both R and G
    if (yellowBit && !redBit && !blueBit) {
        // Pure YELLOW
        r = 250;
        g = 255;
        b = 0;
    } else if (yellowBit && redBit && !blueBit) {
        // ORANGE (YELLOW + RED)
        r = 255;
        g = 80;
        b = 0;
    } else if (yellowBit && !redBit && blueBit) {
        // GREEN (YELLOW + BLUE)
        r = 0;
        g = 255;
        b = 255;
    } else if (!yellowBit && redBit && blueBit) {
        // PURPLE (RED + BLUE)
        r = 255;
        g = 0;
        b = 255;
    } else if (yellowBit && redBit && blueBit) {
        // WHITE (all bits active)
        r = 255;
        g = 255;
        b = 255;
    } else if (!yellowBit && redBit && !blueBit) {
        // Pure RED
        r = 255;
        g = 0;
        b = 0;
    } else if (!yellowBit && !redBit && blueBit) {
        // Pure BLUE
        r = 0;
        g = 0;
        b = 255;
    }

    return strip.Color(r, g, b);
}

uint32_t DisplayTask::getDirectionColor(float component, float normValue) {
    // Map component value to color: RED (very negative) → GREEN (very positive)
    // Normalize component to [-1, 1] range
    float normalized = component / normValue;
    normalized = fmax(-1.0f, fmin(1.0f, normalized));

    // Map to [0, 1] for color interpolation
    float t = (normalized + 1.0f) / 2.0f; // -1 → 0, 0 → 0.5, 1 → 1

    uint8_t r, g, b;

    if (t < 0.5f) {
        // RED (t=0) → YELLOW (t=0.5)
        float f = t / 0.5f; // 0..1
        r = 255;
        g = (uint8_t)(f * 255);
        b = 0;
    } else {
        // YELLOW (t=0.5) → GREEN (t=1.0)
        float f = (t - 0.5f) / 0.5f; // 0..1
        r = (uint8_t)((1.0f - f) * 255);
        g = 255;
        b = 0;
    }

    return strip.Color(r, g, b);
}

void DisplayTask::turnDisplayOFF() {
    // Set all pixels to OFF
    // Use the larger of the two counts to ensure all LEDs are turned off
    int maxCount = (NEOPIXEL_COUNT > BINARY_NEOPIXEL_COUNT) ? NEOPIXEL_COUNT : BINARY_NEOPIXEL_COUNT;
    for (int LED = 0; LED < maxCount; LED++) {
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
