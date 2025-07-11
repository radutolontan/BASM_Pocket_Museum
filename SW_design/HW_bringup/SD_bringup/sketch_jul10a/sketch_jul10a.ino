#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

// ==== CONFIGURABLE SPI PINS (ESP32 or other boards) ====
#define PIN_MOSI 18
#define PIN_MISO 20
#define PIN_SCK  19
#define PIN_CS   23    // You can change this to any available pin

// Use VSPI bus (common for SPI peripherals on ESP32)
SPIClass spiBus(1);  // Use 1 for VSPI (instead of VSPI macro)

// Function prototype
void listDir(const char *dirname, uint8_t levels);

void setup() {
  Serial.begin(115200);
  Serial.println("Waiting 10 sec...");
  delay(2000);
  Serial.println("Waiting 8 sec...");
  delay(2000);
  Serial.println("Waiting 6 sec...");
  delay(2000);
  Serial.println("Waiting 4 sec...");
  delay(2000);
  Serial.println("Waiting 2 sec...");
  delay(2000);
  Serial.println("Initializing SD card...");

  // Begin SPI with custom pins
  spiBus.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);

  if (!SD.begin(PIN_CS, spiBus)) {
    Serial.println("Card Mount Failed!");
    return;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached.");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) Serial.println("MMC");
  else if (cardType == CARD_SD) Serial.println("SDSC");
  else if (cardType == CARD_SDHC) Serial.println("SDHC");
  else Serial.println("UNKNOWN");

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %llu MB\n", cardSize);

  Serial.println("Listing root directory:");
  listDir("/", 0);

  File testFile = SD.open("/test.txt", FILE_WRITE);
  if (testFile) {
    testFile.println("Hello from ESP32!");
    testFile.close();
    Serial.println("File written: /test.txt");
  } else {
    Serial.println("Failed to write file");
  }
}

void loop() {
  // Nothing
}

void listDir(const char *dirname, uint8_t levels) {
  File root = SD.open(dirname);
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("DIR : ");
      Serial.println(file.name());
      if (levels) listDir(file.name(), levels - 1);
    } else {
      Serial.print("FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}