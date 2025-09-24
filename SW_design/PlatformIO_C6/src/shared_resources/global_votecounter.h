#include <Arduino.h>

// -------------------------------------
// VOTE COUNTER NODE / HUB CONFIGURATION
// -------------------------------------
#define IS_HUB                  false         // set to false for nodes
constexpr uint8_t HUB_MAC[6] = {0xB4, 0x3A, 0x45, 0x9B, 0x86, 0x7C}; // for nodes
#define NODE_ID                 0             // only used if IS_HUB == false
#define NODE_SEND_INTERVAL      500           // ms, only for nodes