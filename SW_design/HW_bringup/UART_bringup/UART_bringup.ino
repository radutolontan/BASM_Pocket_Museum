void setup() {
  // Start USB CDC serial communication
  Serial.begin(115200);

  // Wait for the USB CDC connection to be ready (optional)
  while (!Serial) {
    delay(10);  // wait for serial monitor to open
  }

  Serial.println("Starting...");
}

void loop() {
  Serial.println("Hello");
  delay(1000);  // wait 1 second
}
