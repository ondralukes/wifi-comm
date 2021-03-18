#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "DeviceManager.h"
#include "WiFiManager.h"
#include "MessageBuilder.h"
#include "Keyboard.h"
#include "Display.h"

Display lcd;
DeviceManager deviceManager(lcd);

const uint8_t rowPins [4] = {D4, D9, D3, D5};
const uint8_t colsPins [4] = {D0, D6, D7, D8};
Keyboard keyboard(rowPins, 4, colsPins, 4);
void setup() {
    // Convert RX to GPIO
    pinMode(D9, FUNCTION_3);
    WiFi.mode(WIFI_OFF);
}

void loop() {
    static bool inMessage = false;
    int pk = keyboard.Peek();
    if(pk != -1){
        lcd.ReplaceBottom(pk);
    }
    int rd = keyboard.Read();
    if(rd != -1){
        if(rd == '#'){
            if(inMessage) {
                MessageBuilder::SendMessage(deviceManager);
                lcd.ClearBottom();
                inMessage = false;
            }
        } else {
            lcd.WriteBottom(rd);
            if(!inMessage) {
                MessageBuilder::BeginMessage();
                inMessage = true;
            }
            MessageBuilder::Write(rd);
        }
    }
    WiFiManager::CheckConnection(lcd);
    lcd.Update();
    deviceManager.Update();
}