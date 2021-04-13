#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include "DeviceManager.h"
#include "WiFiManager.h"
#include "MessageBuilder.h"
#include "Keyboard.h"

Display lcd;
DeviceManager deviceManager(lcd);

const uint8_t rowPins [4] = {D7, D8, D6, D5};
// Column with pin 255 is used as fallback
const uint8_t colsPins [4] = {D0, D4, D3, 255};
Keyboard keyboard(rowPins, 4, colsPins, 4);
void setup() {
    // Convert RX to GPIO
    pinMode(D9, FUNCTION_3);
    pinMode(D9, OUTPUT);
    pinMode(D7, INPUT);
    digitalWrite(D9, LOW);
    WiFi.mode(WIFI_OFF);
    lcd.WriteLine("wifi-comm", 0);
    lcd.WriteLine(REVISION);
    delay(1000);
    EEPROM.begin(512);
    char magicByte = EEPROM.read(0);
    char name[128];
    lcd.Clear();
    if(magicByte != 0x69){
        snprintf(name, 128, "u%06x", ESP.getChipId());
    } else {
        int index = 1;
        char* c = name;
        while((*(c++) = EEPROM[index]) != '\0')
            index++;
    }

    lcd.WriteLine("Name", 0);
    lcd.WriteLine(name, 1);

    unsigned long start = millis();
    bool changed = false;
    char* insertAt = name;
    while(changed || millis() - start < 2000){
        ESP.wdtFeed();
        int pk = keyboard.Peek();
        if(pk != -1){
            if(!changed){
                lcd.ClearLine();
                changed = true;
            }
            lcd.ReplaceBottom(pk);
        }
        int rd = keyboard.Read();
        if(rd != -1){
            if(rd == '#'){
                *insertAt = '\0';
                break;
            } else if (rd == '<'){
                if(changed && insertAt != name){
                    lcd.DeleteBottom();
                    insertAt--;
                }
            } else {
                if(!changed){
                    lcd.ClearLine();
                    changed = true;
                }
                lcd.WriteBottom(rd);
                *(insertAt++) = rd;
            }
        }
    }

    if(magicByte != 0x69)
        EEPROM.write(0, 0x69);
    const char* c = name;
    int addr = 1;
    do {
        EEPROM.write(addr++, *c);
    } while (*(c++) != '\0');
    EEPROM.commit();

    lcd.Clear();
    WiFiManager::Init();
    deviceManager.SetName(name);
    lcd.WriteRolling(name);
}

void loop() {
    static bool inMessage = false;
    static unsigned long shuttingDownStart = -1;
    static unsigned long forceShowAckStart = -1;
    int pk = keyboard.Peek();
    if(pk != -1){
        if(!inMessage){
            MessageBuilder::BeginMessage();
            lcd.ClearLine();
            inMessage = true;
        }
        lcd.ReplaceBottom(pk);
    }
    int rd = keyboard.Read();
    if(rd != -1){
        if(rd == '#'){
            if(inMessage) {
                MessageBuilder::SendMessage(deviceManager);
                lcd.ClearLine();
                forceShowAckStart = millis();
                inMessage = false;
            }
        } else if(rd == ';'){
            WiFiManager::upstreamEnabled = !WiFiManager::upstreamEnabled;
        } else if(rd == '^'){
            shuttingDownStart = shuttingDownStart==-1?millis():-1;
        } else if (rd == '<'){
            if(inMessage) lcd.DeleteBottom();
            inMessage = MessageBuilder::Delete();
        } else {
            if(!inMessage){
                MessageBuilder::BeginMessage();
                lcd.ClearLine();
                inMessage = true;
            }
            lcd.WriteBottom(rd);
            MessageBuilder::Write(rd);
        }
    }

    if(lcd.NeedsUpdate(deviceManager, shuttingDownStart, inMessage, forceShowAckStart)){
        enum WiFiManager::Status status = WiFiManager::Status();
        char text[17];
        if(shuttingDownStart != -1){
            snprintf(text, 17, "Shutting down...");
        } else if(deviceManager.acksRemaining != 0 ||  (forceShowAckStart != -1 && millis() - forceShowAckStart < 1000)){
            snprintf(text, 17, "Sent to %d/%d A%d", (deviceManager.acks - deviceManager.acksRemaining), deviceManager.acks, deviceManager.attempts);
            if(deviceManager.acksRemaining != 0) forceShowAckStart = millis();
        } else if(status == WiFiManager::Connected){
            snprintf(text, 17, "UP%02d@%06x DN%02d", deviceManager.upstreamDevices, WiFiManager::HostId(), deviceManager.downstreamDevices);
            forceShowAckStart = -1;
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
            forceShowAckStart = -1;
        }

        lcd.WriteLine(text);
    }
    lcd.Update();
    deviceManager.Update();
    WiFiManager::CheckConnection();
    if(shuttingDownStart != -1 && millis() - shuttingDownStart > 3000){
        digitalWrite(D9, HIGH);
    }
}