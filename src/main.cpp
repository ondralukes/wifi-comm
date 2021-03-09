#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "DeviceManager.h"
#include "WiFiManager.h"
#include "MessageBuilder.h"
#include "Keyboard.h"

#ifndef REVISION
    #define REVISION "unknown"
#endif
DeviceManager deviceManager;
const uint8_t rowPins [4] = {D1, D2, D3, D5};
const uint8_t colsPins [4] = {D4, D6, D7, D8};
Keyboard keyboard(rowPins, 4, colsPins, 4);
void setup() {
    WiFi.mode(WIFI_OFF);
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.println();
    Serial.println(ESP.getResetReason());
    Serial.print("[System] wifi-comm@");
    Serial.println(REVISION);
}


void handle_ui(){
    static bool message = false;
    if(Serial.available() == 0) return;
    while (Serial.available() > 0) {
        char c = Serial.read();
        if(c == '`'){
            if(message){
                MessageBuilder::SendMessage(deviceManager);
                message = false;
            } else {
                MessageBuilder::BeginMessage();
                message = true;
            }
            continue;
        }
        if(message){
            MessageBuilder::Write(c);
            continue;
        }
        if(c == 'r'){
            Serial.println("[System] Reset.");
            ESP.reset();
        }
    }
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
        Serial.printf("%c\x1b[D", pk);
    }
    int rd = keyboard.Read();
    if(rd != -1){
        if(rd == '#'){
            Serial.println();
            if(inMessage) {
                MessageBuilder::SendMessage(deviceManager);
                inMessage = false;
            }
        } else {
            Serial.print((char)rd);
            if(!inMessage) {
                MessageBuilder::BeginMessage();
                inMessage = true;
            }
            MessageBuilder::Write(rd);
        }
    }
    WiFiManager::CheckConnection();
    handle_ui();
    deviceManager.Update();
    status_led();
}