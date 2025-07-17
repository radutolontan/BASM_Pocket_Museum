#include "SDManager.h"
#include <SD.h>
#include <SPI.h>
#include "globals.h" 

#define DEBOUNCE_DELAY_MS 300

void SDManager::setupSDTask() {
    // Go straight to boot mode
    setSDState(SDState::BOOT);
}

void SDManager::setSDState(SDState new_state){
    // Set the current_state to the (selected) new_state 
    current_state = new_state;
    // TO DO: add checks for correct state transitions
}

bool SDManager::enqueueRequest(const SDRequest& req) {
    if (sdQueue == nullptr) return false;
    return xQueueSend(sdQueue, &req, portMAX_DELAY) == pdTRUE;
}

void SDManager::handleRequest(const SDRequest& req) {
    Serial.printf("SDManager: Handling %s request on %s\n",
                  req.type == SDRequest::Type::READ ? "READ" : "WRITE",
                  req.filename);

    // Placeholder: Call callback
    if (req.callback) {
        req.callback(true, req.data, req.length);
    }
}

void SDManager::runSDTaskWrapper(void* param) {
    auto* instance = static_cast<SDManager*>(param);
    instance->runSDTask();
}

void SDManager::runSDTask() {
    while (true) {
        // Raw reading of SD_CARD_DETECT_PIN
        bool rawState = digitalRead(SD_CARD_DETECT_PIN) == HIGH;
        // Check for debounce to confirm valid transition
        if (debounceCardDetect(rawState)) {
            Serial.printf("[SDManager] - Debounced card state change: %s\n", stableCardInserted ? "Inserted" : "Removed");
        }
        // State Machine
        switch (current_state) {
            case SDState::BOOT:
                run_boot();
                // State transitions out of BOOT are handled internally
                break;

            case SDState::WAIT_FOR_INSERT:
                run_wait_for_insert();
                // State transitions out of WAIT_FOR_INSERT are handled internally
                break;

            case SDState::MOUNTING:
                run_mounting();
                // State transitions out of MOUNTING are handled internally
                break;

            case SDState::READY:
                run_ready();
                // State transitions out of READY are handled internally
                break;

            case SDState::UNMOUNTING:
                run_unmounting();
                // State transitions out of UNMOUNTING are handled internally
                break;

            case SDState::ERROR:
                Serial.println("SDManager: ERROR state. Waiting for card removal.");
                if (!stableCardInserted) {
                    current_state = SDState::WAIT_FOR_INSERT;
                }
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void SDManager::run_boot(){
    // Initialize Card_Detect pin (HIGH = card inserted)
    pinMode(SD_CARD_DETECT_PIN, INPUT_PULLDOWN);  

    // Initialize debounce tracking to actual Card_Detect state
    bool initialState = digitalRead(SD_CARD_DETECT_PIN) == HIGH;
    stableCardInserted = initialState;

    // Create SD Queue ; if successful, set WAIT_FOR_INSERT mode
    sdQueue = xQueueCreate(10, sizeof(SDRequest));
    if (sdQueue == nullptr) {
        Serial.println("[SDManager] - Failed to create queue!");
        setSDState(SDState::ERROR);
    } else {
        Serial.println("[SDManager] - BOOT Successful!");
        setSDState(SDState::WAIT_FOR_INSERT);
    }
}

void SDManager::run_wait_for_insert(){
    if (stableCardInserted) {
        Serial.println("[SDManager] - Card Inserted! Proceeding with Mounting...");
        // Sleep 500 ms before releasing (allow SD card to fully seat)
        vTaskDelay(pdMS_TO_TICKS(500)); 
        // If a card has been inserted, we can go ahead and mount it
        setSDState(SDState::MOUNTING);
    }
    else{
        // Sleep 100 ms before releasing (don't check too often)
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void SDManager::run_mounting(){
    // Fire up SPI bus with custom pins
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_CS_PIN);
    // Check if SD begin was successful
    if (!SD.begin(SPI_CS_PIN, SPI)) {
        Serial.println("[SDManager] - Card Mount Failed!");
        setSDState(SDState::ERROR);
        return;
    }
    // Check if SD card type is present
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("[SDManager] - No SD Card Attached!");
        setSDState(SDState::ERROR);
        return;
    }
    // Print SD Card Metadata
    Serial.print("[SDManager] - SD Card Type: ");
    if (cardType == CARD_MMC) Serial.println("MMC");
    else if (cardType == CARD_SD) Serial.println("SDSC");
    else if (cardType == CARD_SDHC) Serial.println("SDHC");
    else Serial.println("UNKNOWN");
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("[SDManager] - SD Card Size: %llu MB\n", cardSize);

    //If we got to this point, everything worked and we can set READY::MODE
    setSDState(SDState::READY);
}

void SDManager::run_ready(){
    if (!stableCardInserted) {
        Serial.println("[SDManager] - Card removed → UNMOUNTING");
        // If card was removed, transition state to UNMOUNTING
        setSDState(SDState::UNMOUNTING);
    } else {
        SDRequest req;
        if (xQueueReceive(sdQueue, &req, 10 / portTICK_PERIOD_MS)) {
            handleRequest(req);
        }
    }
}

void SDManager::run_unmounting(){
    Serial.println("[SDManager] - Unmounting SD card...");
    SD.end();
    // Transition state to Ready for Insert
    setSDState(SDState::WAIT_FOR_INSERT);
}

bool SDManager::debounceCardDetect(bool rawState) {
    // Implementation for non-blocking debounce for confirming SD Card insertion/desertion is correct
    if (rawState != stableCardInserted) {
        // Detected a possible change — check if it's stable
        if (millis() - lastDetectChange >= DEBOUNCE_DELAY_MS) {
            // Reconfirm the change
            bool confirmedState = digitalRead(SD_CARD_DETECT_PIN) == HIGH;
            if (confirmedState == rawState) {
                stableCardInserted = rawState;
                lastDetectChange = millis();
                return true; // Valid debounced change
            }
        }
    } else {
        // Input is stable, reset debounce timer
        lastDetectChange = millis();
    }
    return false;
}