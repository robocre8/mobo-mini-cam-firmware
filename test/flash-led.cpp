#include <Arduino.h>

#define FLASH_LED_PIN 4

void setup()
{
    pinMode(FLASH_LED_PIN, OUTPUT);
}

void loop()
{
    digitalWrite(FLASH_LED_PIN, HIGH);  // LED ON
    delay(500);

    digitalWrite(FLASH_LED_PIN, LOW);   // LED OFF
    delay(500);
}