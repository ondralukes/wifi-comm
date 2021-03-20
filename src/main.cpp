#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "DeviceManager.h"
#include "WiFiManager.h"
#include "MessageBuilder.h"
#include "Keyboard.h"

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
        static enum WiFiManager::Status prevStatus;
        static int prevUpstream = 0;
        static int prevDownstream = 0;
        static int prevAcksRemaining = 0;
        static bool prevUpstreamEnabled = true;
        enum WiFiManager::Status status = WiFiManager::Status();
        if(
                prevInMessage ||
                status != prevStatus ||
                prevUpstream != deviceManager.upstreamDevices ||
                prevDownstream != deviceManager.downstreamDevices ||
                prevUpstreamEnabled != WiFiManager::upstreamEnabled ||
                prevAcksRemaining != deviceManager.acksRemaining){
            char text[17];
            if(deviceManager.acksRemaining != 0){
                snprintf(text, 17, "%d ACKs", deviceManager.acksRemaining);
            } else if(status == WiFiManager::Connected){
                snprintf(text, 17, "UP%02d@%06x DN%02d", deviceManager.upstreamDevices, WiFiManager::HostId(), deviceManager.downstreamDevices);
            } else {
                const char* statusText;
                switch (status) {
                    case WiFiManager::Disabled:
                        statusText = "OFF ";
                        break;
                    case WiFiManager::Scanning:
                        statusText = "SCAN";
                        break;
                    case WiFiManager::Connecting:
                        statusText = "CONN";
                        break;
                    default:
                        break;
                }
                snprintf(text, 17, "UP   %s   DN%02d", statusText, deviceManager.downstreamDevices);
            }

            lcd.WriteStatus(text);
            prevStatus = status;
            prevUpstream = deviceManager.upstreamDevices;
            prevDownstream = deviceManager.downstreamDevices;
            prevUpstreamEnabled = WiFiManager::upstreamEnabled;
            prevAcksRemaining = deviceManager.acksRemaining;
        }
    }
    prevInMessage = inMessage;
    lcd.Update();
    deviceManager.Update();
    WiFiManager::CheckConnection();
}