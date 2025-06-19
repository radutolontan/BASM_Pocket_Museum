#include "DisplayTask.h"
#include <Adafruit_NeoPixel.h>
#include "globals.h"
#include <random>

// CLASS Constructor
DisplayTask::DisplayTask() 
: strip(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800) // initialize WS2812B strip
{

}

void DisplayTask::setupDisplayTask() {
    current_state = DisplayState::BOOT;
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
                setDisplayState(DisplayState::DISPLAY_SENSE);
                break;
            }
            case DisplayState::DISPLAY_SENSE:{
                run_display_sense();
                break;
            }
            case DisplayState::DISPLAY_SHOW:{
                run_display_show();
                setDisplayState(DisplayState::DISPLAY_SENSE);
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void DisplayTask::run_boot(){
    Serial.println("[DisplayTask] - Waiting for INIT command...");
    delay(500);
};

void DisplayTask::run_init(){
    Serial.println("[DisplayTask] - Configuring Display...");
    strip.begin();                              // Initialize the NeoPixel library
    strip.setBrightness(NEOPIXEL_BRIGHTNESS);   // Set brightness 
    strip.show();                               // Update strip to apply brightness and clear LEDs
    displayGitShaPattern();                     // Display GIT SHA to confirm correct SW version
};

void DisplayTask::run_display_sense(){
    Serial.println("[DisplayTask] - Updating sensor display...");
};

void DisplayTask::run_display_show(){

};

// import_colorslib method deffinition
void DisplayTask::import_colorlib() {
// Initialize Color library
    colors_lib.push_back(Adafruit_NeoPixel::Color(0, 0, 0));        // 0 OFF 
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 0, 0));      // 1 Red
    colors_lib.push_back(Adafruit_NeoPixel::Color(0, 255, 0));      // 2 Green
    colors_lib.push_back(Adafruit_NeoPixel::Color(0, 0, 255));      // 3 Blue
    colors_lib.push_back(Adafruit_NeoPixel::Color(255,  80, 0));    // 4 Orange
    colors_lib.push_back(Adafruit_NeoPixel::Color(250, 255, 0));    // 5 Yellow
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 0, 255));    // 6 Purple
    colors_lib.push_back(Adafruit_NeoPixel::Color(255, 255, 255));  // 7 White

    // Cycle all colors
    for (uint8_t j = 1; j < 8; j++){
        for (uint16_t i = 0; i < NEOPIXEL_COUNT; i++) {
            strip.setPixelColor(i, colors_lib[j]);
        }
        strip.show();
        vTaskDelay(pdMS_TO_TICKS(5000));     
    }
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
    // Keep displaying SHA for 10 seconds
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
