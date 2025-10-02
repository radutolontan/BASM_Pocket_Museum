#include "VoteCounter.h"
#include "shared_resources/globals.h"
#include "shared_resources/global_votecounter.h"
#include "DFRobot_LedDisplayModule.h"

// ================== Node ==================
Node::Node(const uint8_t hub_mac[6], uint8_t id, unsigned long sendIntervalMs)
    : nodeID(id), lastSend(0), sendInterval(sendIntervalMs), nextValue(0), hasNewValue(false) {
    memcpy(hubMac, hub_mac, 6);
}

void Node::begin() {
    pinMode(DEBUG_LED_PIN, OUTPUT);
    digitalWrite(DEBUG_LED_PIN, LOW);
    Serial.printf("Node %d initialized\n", nodeID);
}

void Node::loop() {
    unsigned long now = millis();
    if (now - lastSend >= sendInterval) {
        lastSend = now;
        if (hasNewValue) {
            WiFi.mode(WIFI_STA);
            if (esp_now_init() != ESP_OK) {
                Serial.println("ESP-NOW init failed");
                return;
            }

            esp_now_register_send_cb(onDataSentStatic);

            esp_now_peer_info_t peerInfo = {};
            memcpy(peerInfo.peer_addr, hubMac, 6);
            peerInfo.channel = 0;
            peerInfo.encrypt = false;
            esp_now_add_peer(&peerInfo);

            NodePayload payload{nodeID, nextValue};
            esp_err_t result = esp_now_send(hubMac, (uint8_t*)&payload, sizeof(payload));
            if (result == ESP_OK) flashLED();
            else Serial.println("Send failed");

            hasNewValue = false;
            esp_now_deinit();
            WiFi.mode(WIFI_OFF);
        }
    }
}

void Node::setNextValue(int value) {
    nextValue = value;
    hasNewValue = true;
}

void Node::flashLED() {
    digitalWrite(DEBUG_LED_PIN, HIGH);
    delay(50);
    digitalWrite(DEBUG_LED_PIN, LOW);
}

void Node::onDataSentStatic(
#if ESP_IDF_VERSION_MAJOR >= 5
    const wifi_tx_info_t *info,
#else
    const uint8_t *mac_addr,
#endif
    esp_now_send_status_t status)
{
#if ESP_IDF_VERSION_MAJOR >= 5
    if (info) {
        Serial.printf("Send status to %02X:%02X:%02X:%02X:%02X:%02X: %s\n",
                      info->des_addr[0], info->des_addr[1], info->des_addr[2],
                      info->des_addr[3], info->des_addr[4], info->des_addr[5],
                      status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
    } else {
        Serial.printf("Send status (no peer info): %s\n",
                      status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
    }
#else
    Serial.printf("Send status to %02X:%02X:%02X:%02X:%02X:%02X: %s\n",
                  mac_addr[0], mac_addr[1], mac_addr[2],
                  mac_addr[3], mac_addr[4], mac_addr[5],
                  status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
#endif
}

// ================== Hub ==================
Hub* Hub::activeHub = nullptr;

// Define static channel arrays
uint8_t Hub::CHANNELS_MUX[] = {MUX_IO_NODE_0, MUX_IO_NODE_1, MUX_IO_NODE_2, MUX_IO_NODE_3};
const uint8_t Hub::NUM_CHANNELS = sizeof(Hub::CHANNELS_MUX) / sizeof(Hub::CHANNELS_MUX[0]);

// LED objects
DFRobot_LedDisplayModule* ledDisplays[Hub::NUM_CHANNELS] = {nullptr};

Hub::Hub(uint8_t maxNodes_) : maxNodes(maxNodes_) {
    pinMode(DEBUG_LED_PIN, OUTPUT);
    digitalWrite(DEBUG_LED_PIN, LOW);
    memset(nodeCounters, 0, sizeof(nodeCounters));
    memset(baseOffsets, 0, sizeof(baseOffsets));
    memset(lastSeenValues, 0, sizeof(lastSeenValues));

    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        ledDisplays[i] = nullptr;
    }
}

void Hub::begin() {
    activeHub = this;
    prefs.begin("hubstore", false);
    // Load offsets from NVM
    loadFromNVM();
    // Initialize I2C Bus
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 100000);
    // Initialize WiFi on HUB
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("[VoteCounter] - ESP-NOW init failed");
        return;
    }
    esp_now_register_recv_cb(onDataRecvStatic);
    // Initialize DF0645 displays
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        tcaSelect(CHANNELS_MUX[i]);
        ledDisplays[i] = new DFRobot_LedDisplayModule(&Wire, 0x48);
        while (ledDisplays[i]->begin(ledDisplays[i]->e4Bit) != 0) {
            Serial.println("[VoteCounter] - LED init failed, retrying...");
            delay(100);
        }
        ledDisplays[i]->setDisplayArea(1, 2, 3, 4);
        ledDisplays[i]->displayOn();
        ledDisplays[i]->setBrightness(4);

        Serial.printf("[VoteCounter] - LED display on mux channel %d initialized with value %d.\n",
                      CHANNELS_MUX[i], nodeCounters[i]);
    }
    Serial.println("[VoteCounter] - Hub ready, listening for nodes...");
}

void Hub::loop() {
    // --- Handle g_bmsLatched transitions ---
    if (lastBmsLatched && !g_bmsLatched) {
        // Transition from true → false, turn off all displays
        for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
            if (ledDisplays[i]) {
                tcaSelect(CHANNELS_MUX[i]);
                ledDisplays[i]->displayOff();
            }
        }
        Serial.println("[VoteCounter] - g_bmsLatched is false, displays OFF");
    }
    lastBmsLatched = g_bmsLatched;

    // Handle Command Line Instructions
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd.equalsIgnoreCase("reset")) {
            resetBaseOffsets();
            Serial.println("Base offsets reset!");
        } else if (cmd.equalsIgnoreCase("test")) {
            testMode = !testMode;
            Serial.printf("Test mode %s\n", testMode ? "ENABLED" : "DISABLED");
        } else if (cmd.startsWith("setOffset")) {
            int firstSpace = cmd.indexOf(' ');
            int secondSpace = cmd.indexOf(' ', firstSpace + 1);
            if (firstSpace > 0 && secondSpace > firstSpace) {
                int nodeID = cmd.substring(firstSpace + 1, secondSpace).toInt();
                int newOffset = cmd.substring(secondSpace + 1).toInt();
                if (nodeID >= 0 && nodeID < MAX_NODES) {
                    baseOffsets[nodeID] = newOffset;
                    lastSeenValues[nodeID] = 0;
                    nodeCounters[nodeID] = baseOffsets[nodeID];
                    saveToNVM();
                    Serial.printf("Node %d offset reset to %d, total=%d\n",
                                  nodeID, baseOffsets[nodeID], nodeCounters[nodeID]);
                    if (nodeID < NUM_CHANNELS && ledDisplays[nodeID]) {
                        tcaSelect(CHANNELS_MUX[nodeID]);
                        ledDisplays[nodeID]->print(nodeCounters[nodeID]);
                    }
                } else {
                    Serial.println("Invalid nodeID");
                }
            } else {
                Serial.println("Invalid command format. Usage: setOffset <nodeID> <value>");
            }
        }
    }
    // Update all displays every loop
    updateDisplays();
}

void Hub::flashLED() {
    digitalWrite(DEBUG_LED_PIN, HIGH);
    delay(50);
    digitalWrite(DEBUG_LED_PIN, LOW);
}

void Hub::handleRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
    if (len != sizeof(NodePayload)) return;
    NodePayload payload;
    memcpy(&payload, incomingData, sizeof(payload));
    if (payload.nodeID < maxNodes) {
        int raw = payload.value;
        int lastSeen = lastSeenValues[payload.nodeID];
        if (!testMode) {
            if (raw < lastSeen) baseOffsets[payload.nodeID] += lastSeen;
            nodeCounters[payload.nodeID] = baseOffsets[payload.nodeID] + raw;
        } else {
            nodeCounters[payload.nodeID] = raw;
        }
        lastSeenValues[payload.nodeID] = raw;
        saveToNVM();
        Serial.printf("Node %d total=%d (raw=%d, base=%d)\n",
                      payload.nodeID,
                      nodeCounters[payload.nodeID],
                      raw,
                      baseOffsets[payload.nodeID]);
        flashLED();
    } else {
        Serial.println("Invalid nodeID received");
    }
}

void Hub::onDataRecvStatic(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
    if (activeHub) activeHub->handleRecv(info, incomingData, len);
}

// ============ Persistence ============
void Hub::loadFromNVM() {
    for (int i = 0; i < MAX_NODES; i++) {
        baseOffsets[i] = prefs.getInt((String("base") + i).c_str(), 0);
        lastSeenValues[i] = prefs.getInt((String("last") + i).c_str(), 0);
        nodeCounters[i] = baseOffsets[i] + lastSeenValues[i]; // Ensure startup value
    }
}

void Hub::saveToNVM() {
    for (int i = 0; i < MAX_NODES; i++) {
        prefs.putInt((String("base") + i).c_str(), baseOffsets[i]);
        prefs.putInt((String("last") + i).c_str(), lastSeenValues[i]);
    }
}

void Hub::resetBaseOffsets() {
    for (int i = 0; i < MAX_NODES; i++) {
        baseOffsets[i] = 0;
        nodeCounters[i] = lastSeenValues[i]; // update display immediately
    }
    saveToNVM();
}

void Hub::tcaSelect(uint8_t channel) {
    if (channel > 7) return;
    Wire.beginTransmission(MUX_ADDR);
    Wire.write(1 << channel);
    Wire.endTransmission();
}

void Hub::updateDisplays() {
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        if (ledDisplays[i]) {
            tcaSelect(CHANNELS_MUX[i]);
            int value = nodeCounters[i];

            // Build a 4-char buffer: leading '.' + digits
            char buf0[2] = ".";  // leftmost digit
            char buf1[2] = ".";
            char buf2[2] = ".";
            char buf3[2] = ".";  // rightmost digit

            // Convert integer to string
            char strVal[12];
            snprintf(strVal, sizeof(strVal), "%d", value);

            int len = strlen(strVal);
            // Place digits in the rightmost positions
            // Example: "42" -> "..42"
            switch (len) {
                case 4:
                    buf0[0] = strVal[0];
                    buf1[0] = strVal[1];
                    buf2[0] = strVal[2];
                    buf3[0] = strVal[3];
                    break;
                case 3:
                    buf1[0] = strVal[0];
                    buf2[0] = strVal[1];
                    buf3[0] = strVal[2];
                    break;
                case 2:
                    buf2[0] = strVal[0];
                    buf3[0] = strVal[1];
                    break;
                case 1:
                default:
                    buf3[0] = strVal[0];
                    break;
            }

            // Display all 4 chars
            ledDisplays[i]->print(buf0, buf1, buf2, buf3);
        }
    }
}

