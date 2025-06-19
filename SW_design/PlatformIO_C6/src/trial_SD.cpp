// #include <SPI.h>
// #include <SD.h>

// // Define your custom SPI pins
// #define SD_MISO  19
// #define SD_MOSI  23
// #define SD_SCK   18
// #define SD_CS     5  // Chip Select pin (choose an unused GPIO)

// SPIClass spi = SPIClass(VSPI); // or HSPI on some boards

// void setup() {
//   Serial.begin(115200);

//   // Initialize custom SPI bus
//   spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

//   // Initialize SD card with custom SPI and CS pin
//   if (!SD.begin(SD_CS, spi)) {
//     Serial.println("SD Card initialization failed!");
//     return;
//   }

//   Serial.println("SD Card initialized.");
  
//   // List files
//   File root = SD.open("/");
//   while (true) {
//     File entry = root.openNextFile();
//     if (!entry) break;
//     Serial.print("FILE: ");
//     Serial.println(entry.name());
//     entry.close();
//   }
// }

// void loop() {}
