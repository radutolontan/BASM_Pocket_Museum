#include <Arduino.h>

// -------------------------------------
// VOTE COUNTER NODE / HUB CONFIGURATION
// -------------------------------------
#define IS_HUB                  false          // set to false for nodes
constexpr uint8_t HUB_MAC[6] = {0xB4, 0x3A, 0x45, 0x9B, 0x86, 0x7C}; // for nodes
#define NODE_ID                 3             // only used if IS_HUB == false
#define NODE_SEND_INTERVAL      500           // ms, only for nodes

// I2C MUX CONFIG
#define MUX_ADDR 0x70
#define MUX_IO_NODE_0 6
#define MUX_IO_NODE_1 4
#define MUX_IO_NODE_2 2
#define MUX_IO_NODE_3 1