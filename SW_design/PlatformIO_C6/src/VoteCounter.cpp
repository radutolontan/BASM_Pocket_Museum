#include "VoteCounter.h"
#include "shared_resources/globals.h"

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

Hub::Hub(uint8_t maxNodes_) : maxNodes(maxNodes_) {
    pinMode(DEBUG_LED_PIN, OUTPUT);
    digitalWrite(DEBUG_LED_PIN, LOW);
    memset(nodeCounters, 0, sizeof(nodeCounters));
    memset(baseOffsets, 0, sizeof(baseOffsets));
    memset(lastSeenValues, 0, sizeof(lastSeenValues));
}

void Hub::begin() {
    activeHub = this;
    prefs.begin("hubstore", false);
    loadFromNVM();

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        return;
    }
    esp_now_register_recv_cb(onDataRecvStatic);

    Serial.println("Hub ready, listening for nodes...");
}

void Hub::loop() {
    // Handle Command Line Instructions
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        // reset -> resets ALL nodes to zero
        if (cmd.equalsIgnoreCase("reset")) {
            resetBaseOffsets();
            Serial.println("Base offsets reset!");
        // test -> enables test mode
        } else if (cmd.equalsIgnoreCase("test")) {
            testMode = !testMode;
            Serial.printf("Test mode %s\n", testMode ? "ENABLED" : "DISABLED");
        // setOffset -> manually forces an offset for one of the nodes
        } else if (cmd.startsWith("setOffset")) {
        // Parse the command: setOffset <nodeID> <value>
        int firstSpace = cmd.indexOf(' ');
        int secondSpace = cmd.indexOf(' ', firstSpace + 1);
        if (firstSpace > 0 && secondSpace > firstSpace) {
            // Extract requested node_ID and new_Offset
            int nodeID = cmd.substring(firstSpace + 1, secondSpace).toInt();
            int newOffset = cmd.substring(secondSpace + 1).toInt();
            // Check if nodeID is valid
            if (nodeID >= 0 && nodeID < MAX_NODES) {
                // Reset everything for this node
                baseOffsets[nodeID] = newOffset;
                lastSeenValues[nodeID] = 0;
                nodeCounters[nodeID] = baseOffsets[nodeID]; // start fresh
                // Save new baseOffset to NVM
                saveToNVM();
                Serial.printf("Node %d offset reset to %d, total=%d\n",
                            nodeID,
                            baseOffsets[nodeID],
                            nodeCounters[nodeID]);
            } else {
                Serial.println("Invalid nodeID");
            }
        } else {
            Serial.println("Invalid command format. Usage: setOffset <nodeID> <value>");
        }
    }
    }
}

void Hub::flashLED() {
    digitalWrite(DEBUG_LED_PIN, HIGH);
    delay(50);
    digitalWrite(DEBUG_LED_PIN, LOW);
}

void Hub::handleRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
    // Check if recieved message has the correct size; if not => trash
    if (len != sizeof(NodePayload)) return;
    // Copy incoming message
    NodePayload payload;
    memcpy(&payload, incomingData, sizeof(payload));
    // If the NODE_ID exists:
    if (payload.nodeID < maxNodes) {
        int raw = payload.value;
        int lastSeen = lastSeenValues[payload.nodeID];
        // If NOT in test_mode
        if (!testMode) {
            // If a NODE reset has happened
            if (raw < lastSeen) {
                // Increment the baseOffset by the last recieved value
                baseOffsets[payload.nodeID] += lastSeen;
            }
            // Compute the total (for display) as the baseOffset + the raw value just recieved
            nodeCounters[payload.nodeID] = baseOffsets[payload.nodeID] + raw;
        } else {
            // If in test mode; don't worry about chainging the offsets. just display the raw data from the node
            nodeCounters[payload.nodeID] = raw;
        }
        // Update the lastSeenValue with the last reading
        lastSeenValues[payload.nodeID] = raw;
        // Save lastSeenValue and baseOffset to NVM
        saveToNVM();
        // Debug Statements
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
    }
    saveToNVM();
}