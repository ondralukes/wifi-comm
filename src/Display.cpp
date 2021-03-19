#include <Arduino.h>
#include "Display.h"
#include "global.h"

Display::Display() : lcd(0x27, 16, 2){
    lcd.init();
    lcd.backlight();
    memset(buffer, (int)' ', 16);
}

void Display::WriteStatus(const char* msg) {
    ClearBottom();
    lcd.setCursor(0, 1);
    lcd.print(msg);
}

void Display::Update() {
    if(millis() - lastUpdate < 500) return;
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

void Display::ClearBottom() {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    bottomPosition = 0;
}

void Display::Clear() {
    lcd.clear();
    memset(buffer, (int)' ', 16);
    start = 0;
    end = 16;
    ClearBottom();
}

void Display::WriteRollingHex(int x) {
    char buf[64];
    itoa(x, buf, 16);
    WriteRolling(buf);
}
