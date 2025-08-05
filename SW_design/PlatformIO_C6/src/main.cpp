#include <Arduino.h>

void setup() {
  Serial0.begin(115200);
  Serial0.println("UART0 test: Hello World");
}

void loop() {
  static unsigned long last = 0;
  if (millis() - last > 2000) {
    Serial0.println("UART0: Still running...");
    last = millis();
  }
}
