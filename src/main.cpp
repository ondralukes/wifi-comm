#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "DeviceManager.h"
#include "WiFiManager.h"
#include "MessageBuilder.h"
#include "Keyboard.h"
#include "Display.h"

Display lcd;
DeviceManager deviceManager(lcd);

const uint8_t rowPins [4] = {10, D9, D3, D5};
const uint8_t colsPins [4] = {D4, D6, D7, D8};
Keyboard keyboard(rowPins, 4, colsPins, 4);
void setup() {
    // Convert RX to GPIO
    pinMode(D9, FUNCTION_3);
    WiFi.mode(WIFI_OFF);
    pinMode(LED_BUILTIN, OUTPUT);

    lcd.WriteRolling("test");
}


void status_led(){
    if(WiFiManager::Status() == Station){
        digitalWrite(LED_BUILTIN, LOW);
        return;
    }
    static unsigned long prevBlink = 0;
    static bool state = false;
    if(millis() - prevBlink > 250){
        state = !state;
        prevBlink = millis();
        digitalWrite(LED_BUILTIN, state);
    }
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
    status_led();
}