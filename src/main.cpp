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
    lcd.WriteRolling(REVISION);
    WiFiManager::Init();
}

void loop() {
    static bool inMessage = false;
    int pk = keyboard.Peek();
    if(pk != -1){
        if(!inMessage){
            MessageBuilder::BeginMessage();
            lcd.ClearBottom();
            inMessage = true;
        }
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
        } else if(rd == ';'){
            WiFiManager::upstreamEnabled = !WiFiManager::upstreamEnabled;
        } else {
            lcd.WriteBottom(rd);
            MessageBuilder::Write(rd);
        }
    }

    static bool prevInMessage = true;
    if(!inMessage){
        static bool prevConnected = false;
        static int prevUpstream = 0;
        static int prevDownstream = 0;
        static bool prevUpstreamEnabled = true;
        bool connected = WiFiManager::Connected();
        if(
                prevInMessage ||
                connected != prevConnected ||
                prevUpstream != deviceManager.upstreamDevices ||
                prevDownstream != deviceManager.downstreamDevices ||
                prevUpstreamEnabled != WiFiManager::upstreamEnabled){
            char status[17];
            if(connected){
                snprintf(status, 17, "UP%02d@%06x DN%02d", deviceManager.upstreamDevices, WiFiManager::HostId(), deviceManager.downstreamDevices);
            } else {
                if(WiFiManager::upstreamEnabled){
                    snprintf(status, 17, "UP   N/C    DN%02d", deviceManager.downstreamDevices);
                } else {
                    snprintf(status, 17, "UP   OFF    DN%02d", deviceManager.downstreamDevices);
                }
            }

            lcd.WriteStatus(status);
            prevConnected = connected;
            prevUpstream = deviceManager.upstreamDevices;
            prevDownstream = deviceManager.downstreamDevices;
            prevUpstreamEnabled = WiFiManager::upstreamEnabled;
        }
    }
    prevInMessage = inMessage;
    lcd.Update();
    deviceManager.Update();
    WiFiManager::CheckConnection();
}