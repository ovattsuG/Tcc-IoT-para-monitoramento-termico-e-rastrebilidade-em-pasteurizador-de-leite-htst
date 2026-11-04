#include <Arduino.h>

void setup() {
    Serial.begin(115200);
}

void loop() {
    Serial.println("ola, edge-htst");
    delay(1000);
}