#include "global_functions.h"
#include <vector>
#include <cmath>

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

double vector_norm(float x, float y, float z){
    // Compute the norm of a 3D vector
    double norm3D = std::sqrt(x*x + y*y + z*z);
    return norm3D;
}
