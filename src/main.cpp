#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "DeviceManager.h"
#include "WiFiManager.h"
#include "MessageBuilder.h"

DeviceManager deviceManager;
void setup() {
    WiFi.mode(WIFI_OFF);
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.println();
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
    WiFiManager::CheckConnection();
    handle_ui();
    deviceManager.Update();
    status_led();
}