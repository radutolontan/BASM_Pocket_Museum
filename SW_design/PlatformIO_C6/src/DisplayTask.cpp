#include "DisplayTask.h"
#include <Adafruit_NeoPixel.h>
#include <SharedDataBuffer.h>
#include "globals.h"
#include <random>
#include <bitset>
#include <cmath>
#include <cstdint>

// CLASS Constructor
DisplayTask::DisplayTask() 
: strip(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800) // initialize SK6805-EC20 strip
{

}

void DisplayTask::setupDisplayTask() {
    setDisplayState(DisplayState::BOOT);
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
        switch (current_state) {
            case DisplayState::BOOT:{
                run_boot();
                break;
            }
            case DisplayState::INIT:{
                run_init();
                vTaskDelay(pdMS_TO_TICKS(3000)); // To view the GITSHA
                setDisplayState(DisplayState::DISPLAY_SENSE);
                break;
            }
            case DisplayState::DISPLAY_SENSE:{
                run_display_sense();
                setDisplayState(DisplayState::SLEEP);
                break;
            }
            case DisplayState::DISPLAY_SHOW:{
                run_display_show();
                setDisplayState(DisplayState::DISPLAY_SENSE);
                break;
            }
            case DisplayState::SLEEP:{
                run_display_sleep();
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void DisplayTask::run_boot(){
    // ✅ DEBUG: Print StateMachine State Change
    Serial.println("[DisplayTask] - Waiting for INIT command...");
    delay(500);
};

void DisplayTask::run_init(){
    // ✅ DEBUG: Print StateMachine State Change    
    Serial.println("[DisplayTask] - Configuring Display...");
    strip.begin();                              // Initialize the NeoPixel library
    strip.setBrightness(NEOPIXEL_BRIGHTNESS);   // Set brightness 
    strip.show();                               // Update strip to apply brightness and clear LEDs
    displayGitShaPattern();                     // Display GIT SHA to confirm correct SW version
    import_colorlib();                          // Import color library
};

void DisplayTask::run_display_sense(){
    // ✅ DEBUG: Print StateMachine State Change   
    // Serial.println("[DisplayTask] - Updating sensor display...");
    auto readings = SharedBuffer::getReadings();
    if (!readings.empty()) {
        const SensorData& latest = readings.back();
        displayPressure(latest.pressure);
    }
};

void DisplayTask::run_display_show(){

};

void DisplayTask::run_display_sleep(){
    if (millis() - lastUpdateTime >= DISPLAY_UPDATE_INTERVAL) {
        setDisplayState(DisplayState::DISPLAY_SENSE);
    } else {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
};

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
