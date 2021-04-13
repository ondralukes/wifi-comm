#include <Arduino.h>
#include "Display.h"
#include "WiFiManager.h"
#include "DeviceManager.h"
#include "Keyboard.h"

Display::Display() : lcd(0x27, 16, 2){
    lcd.init();
    lcd.backlight();
    memset(buffer, (int)' ', 16);
}

void Display::WriteLine(const char *msg, int line) {
    ClearLine(line);
    lcd.setCursor(0, line);
    lcd.print(msg);
}

void Display::Update() {
    if(millis() - lastUpdate < 250) return;
    lastUpdate = millis();
    if(start == (end-16)%2048) return;
    start++;
    if(start == 2048) start = 0;
    lcd.setCursor(0, 0);
    int i = start;
    int c = 0;
    while(i!=end && c++ < 16){
        lcd.write(buffer[i]);
        i++;
        if(i == 2048) i = 0;
    }
}

void Display::WriteRolling(const char* msg) {
    while(*msg != 0){
        buffer[end++] = *msg;
        if(end == 2048) end = 0;
        msg++;
    }
}

void Display::WriteRollingInt(int x) {
    char buf[64];
    itoa(x, buf, 10);
    WriteRolling(buf);
}

void Display::WriteBottom(char c) {
    lcd.setCursor(bottomPosition++, 1);
    lcd.write(c);
}

void Display::ReplaceBottom(char c) {
    lcd.setCursor(bottomPosition, 1);
    lcd.write(c);
}

void Display::ClearLine(int line) {
    lcd.setCursor(0, line);
    lcd.print("                ");
    bottomPosition = 0;
}

void Display::DeleteBottom() {
    lcd.setCursor(--bottomPosition, 1);
    lcd.write(' ');
}

void Display::Clear() {
    lcd.clear();
    memset(buffer, (int)' ', 16);
    start = 0;
    end = 16;
    ClearLine();
}

void Display::WriteRollingHex(int x) {
    char buf[64];
    itoa(x, buf, 16);
    WriteRolling(buf);
}

bool Display::NeedsUpdate(DeviceManager &deviceManager, unsigned long _shuttingDown, bool _inMessage,
                          unsigned long _forceShowAckStart) {
    return state.NeedsUpdate(deviceManager, _shuttingDown, _inMessage, _forceShowAckStart);
}

void Display::Prompt(Keyboard &keyboard, const char *msg, char *buf) {
    WriteLine(msg, 0);
    WriteLine(buf, 1);

    bool changed = false;
    char* insertAt = buf;
    unsigned long startTime = millis();
    while(changed || millis() - startTime < 2000){
        ESP.wdtFeed();
        int pk = keyboard.Peek();
        if(pk != -1){
            if(!changed){
                ClearLine();
                changed = true;
            }
            ReplaceBottom(pk);
        }
        int rd = keyboard.Read();
        if(rd != -1){
            if(rd == '#'){
                *insertAt = '\0';
                break;
            } else if (rd == '<'){
                if(changed && insertAt != buf){
                    DeleteBottom();
                    insertAt--;
                }
            } else {
                if(!changed){
                    ClearLine();
                    changed = true;
                }
                WriteBottom(rd);
                *(insertAt++) = rd;
            }
        }
    }
    Clear();
}

template<class T>
bool DisplayState::CheckAndAssign(T *dest, T value) {
    bool r = *dest != value;
    *dest = value;
    return r;
}

bool DisplayState::NeedsUpdate(DeviceManager &deviceManager, unsigned long _shuttingDown, bool _inMessage,
                               unsigned long _forceShowAckStart) {
    bool r = false;
    r |= CheckAndAssign(&status, WiFiManager::Status());
    r |= CheckAndAssign(&upstream, deviceManager.upstreamDevices);
    r |= CheckAndAssign(&downstream, deviceManager.downstreamDevices);
    r |= CheckAndAssign(&acksRemaining, deviceManager.acksRemaining);
    r |= CheckAndAssign(&attempts,deviceManager.attempts);
    r |= CheckAndAssign(&upstreamEnabled, WiFiManager::upstreamEnabled);
    r |= CheckAndAssign(&shuttingDown, _shuttingDown);
    if(inMessage) r = true;
    if(_inMessage && shuttingDown == -1){
        r = false;
    } else if((_forceShowAckStart != -1 &&(millis() - _forceShowAckStart) >= 1000)){
        r = true;
    }
    inMessage = _inMessage;
    return r;
}
