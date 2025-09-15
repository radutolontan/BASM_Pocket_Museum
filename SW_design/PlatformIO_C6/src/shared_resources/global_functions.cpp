#include "global_functions.h"

// Generic debounce helper
bool helperdebounceButton(bool rawState, bool &stableState, unsigned long &lastChange, unsigned long debounceDelay) {
    unsigned long now = millis();
    // Check if the raw state differs from the one which was just read
    if (rawState != stableState) {
        if (now - lastChange >= debounceDelay) {
            stableState = rawState;
            lastChange = now;
        }
    } else {
        lastChange = now;
    }
    // Return stableState
    return stableState; 
}
