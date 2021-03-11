#ifndef WIFI_COMM_DISPLAY_H
#define WIFI_COMM_DISPLAY_H

#include <LiquidCrystal_I2C.h>

class Display {
public:
    Display();
    void Clear();
    void ShowLoading(const char* msg);
    void Update();
    void WriteRolling(const char* msg);
    void WriteRollingInt(int x);
    void WriteBottom(char c);
    void ReplaceBottom(char c);
    void ClearBottom();
private:
    LiquidCrystal_I2C lcd;
    char buffer[2048];
    int start = 0;
    int end = 16;
    unsigned long lastUpdate = 0;

    int bottomPosition = 0;
};


#endif //WIFI_COMM_DISPLAY_H
