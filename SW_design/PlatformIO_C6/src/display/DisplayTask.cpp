#include "display/DisplayTask.h"
#include "shared_resources/SharedDataBuffer.h"
#include "shared_resources/globals.h"

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

void DisplayTask::setupDisplayTask() {
    setDisplayState(DisplayState::BOOT);
    // GPIO for Push-button which toggles display mode
    pinMode(DISPLAY_MODE_PUSHBUTTON_PIN, INPUT);
}

void DisplayTask::setDisplayState(DisplayState new_state) {
    // Optional: Add safety checks or mutex here
    current_state = new_state;
}

void DisplayTask::runDisplayTaskWrapper(void* param) {
    DisplayTask* self = static_cast<DisplayTask*>(param);
    self->runDisplayTask();
}

void DisplayTask::runDisplayTask() {
    while (true) {
        // Check for a debounced signal and a rising edge
        bool rawState = digitalRead(DISPLAY_MODE_PUSHBUTTON_PIN); 
        if (debounceButton(rawState) && stableButtonState == HIGH) {
            cycleDisplayState();  // Trigger mode change
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
            case DisplayState::DISPLAY_ACCEL:{
                run_display_accel();
                break;
            }
        }

        // Wait until running the next step of the state machine
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_INTERVAL));
    }
}

void DisplayTask::run_boot(){
    // ✅ DEBUG: Print StateMachine State Change
    Serial.println("[DisplayTask] - Waiting for INIT command...");
    // Import color-lib for otehr methods to use
    import_colorlib();                          // Import color library
    // Initialize button state
    stableButtonState = digitalRead(DISPLAY_MODE_PUSHBUTTON_PIN);
    lastButtonChange = millis();
};

void DisplayTask::run_init(){
    // ✅ DEBUG: Print StateMachine State Change    
    // Configure Display
    Serial.println("[DisplayTask] - Configuring Display...");
    strip.begin();                              // Initialize the NeoPixel library
    strip.setBrightness(NEOPIXEL_BRIGHTNESS);   // Set brightness 
    strip.show();                               // Update strip to apply brightness and clear LEDs
    // Display the GIT SHA Pattern on the display to confirm correct SW version
    displayGitShaPattern();                     // Display GIT SHA to confirm correct SW version
    vTaskDelay(pdMS_TO_TICKS(3000));            // To view the GITSHA
    // When done, trandisition to DisplayState::DISPLAY_SENSE
    setDisplayState(DisplayState::DISPLAY_PRESSURE);
};

void DisplayTask::run_display_pressure(){
    // ✅ DEBUG: Print StateMachine State Change   
    // Serial.println("[DisplayTask] - Updating sensor display...");
    auto readings = SharedBuffer::getReadings();
    if (!readings.empty()) {
        const SensorData& latest = readings.back();
        displayPressure(latest.pressure);
    }
};

void DisplayTask::run_display_accel(){
    strip.setPixelColor(0, colors_lib[0]); 
    strip.setPixelColor(1, colors_lib[0]); 
    strip.setPixelColor(2, colors_lib[0]); 
    strip.show();
};

bool DisplayTask::debounceButton(bool rawState) {
    // Check if the rawstate reading of the button is different from the button's stable state
    if (rawState != stableButtonState) {
        // If enough time has elapsed to check for debounce
        if (millis() - lastButtonChange >= GPIO_DEBOUNCE_DELAY) {
            // Capture the button state
            stableButtonState = rawState;
            lastButtonChange = millis();
            return true;
        }
    } else {
        lastButtonChange = millis();
    }
    return false;
}

void DisplayTask::cycleDisplayState() {
    // Before switching, compute the aggregate sensor data for the last time
    SensorStats stats = SharedBuffer::getAggregatedStats();
    switch (current_state) {
        // If in PRESSURE DISPLAY -> DISPLAY ACCEL
        case DisplayState::DISPLAY_PRESSURE:
            setDisplayState(DisplayState::DISPLAY_ACCEL);
            break;
        // If in ACCEL DISPLAY -> PRESSURE ACCEL
        case DisplayState::DISPLAY_ACCEL:
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

// import_colorslib method deffinition
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

void DisplayTask::displayPressure(float pressure_float) {
    // Round a floating point value to the nearest integer
    int16_t pressure_rounded = static_cast<int16_t>(std::round(pressure_float * 10));
    // Apply bitmask and keep only bits 0-8
    uint16_t pressure_masked = static_cast<int16_t>(pressure_rounded) & 0x01FF;
    // COnvert to bitset
    std::bitset<16> pressure_bits(pressure_masked);
    // Compute and set color for each LED
    for (int LED = 0; LED < NEOPIXEL_COUNT; ++LED){
        // Starting bit and bimtask of LED
        int start_bit = LED * 3;
        uint16_t led_mask = 0b111 << start_bit;
        // Apply bitmask to extract LED components
        uint16_t led_bits = (pressure_masked & led_mask) >> start_bit;
        // Set the appropriate color to the LED
        // ======= RED ======== YELLOW ======== BLUE =======
        // ======= 2^2 ========  2^1   ========  2^0 =======
        // ======= 2^5 ========  2^4   ========  2^3 =======
        // ======= 2^8 ========  2^7   ========  2^6 =======
        // =======  4  ========    2   ========   1  =======
        // -------------------------------------------------
        // 1 =====  0  ========    0   ========   1  ======= => BLUE
        // 2 =====  0  ========    1   ========   0  ======= => YELLOW
        // 3 =====  0  ========    1   ========   1  ======= => GREEN
        // 4 =====  1  ========    0   ========   0  ======= => RED
        // 5 =====  1  ========    0   ========   1  ======= => PURPLE
        // 6 =====  1  ========    1   ========   0  ======= => ORANGE
        // 7 =====  1  ========    1   ========   1  ======= => WHITE
        // -------------------------------------------------
        // RED + BLUE         RED + BLUE + YELLOW         RED + BLUE
        // 2^9 + 2^8 + 2^6     + 2^5 + 2^4 + 2^3         + 2^2 + 2^0
        // Set pixel colors after reversing the display order for logical reading (Left to Righ)
        strip.setPixelColor(NEOPIXEL_COUNT - LED - 1, colors_lib[led_bits]); 
    }
    // Update the display
    strip.show();
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
    // Keep displaying SHA for 5 seconds
    vTaskDelay(pdMS_TO_TICKS(5000));      
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
