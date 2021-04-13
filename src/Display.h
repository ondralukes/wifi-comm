#ifndef WIFI_COMM_DISPLAY_H
#define WIFI_COMM_DISPLAY_H

#include <LiquidCrystal_I2C.h>
#include "WiFiManager.h"

class DeviceManager;
class DisplayState{
public:
    bool NeedsUpdate(DeviceManager &deviceManager, unsigned long _shuttingDown, bool _inMessage,
                     unsigned long _forceShowAckStart);

private:
    enum WiFiManager::Status status;
    int upstream = 0;
    int downstream = 0;
    int acksRemaining = 0;
    int attempts = 0;
    bool upstreamEnabled = true;
    unsigned long shuttingDown = -1;
    bool inMessage;

    template<class T>
    bool CheckAndAssign(T* dest, T value);
};

class Display {
public:
    Display();
    void Clear();
    void WriteLine(const char *msg, int line = 1);
    void Update();
    void WriteRolling(const char* msg);
    void WriteRollingInt(int x);
    void WriteRollingHex(int x);
    void WriteBottom(char c);
    void ReplaceBottom(char c);
    void DeleteBottom();
    void ClearLine(int line = 1);
    bool NeedsUpdate(DeviceManager &deviceManager, unsigned long _shuttingDown, bool _inMessage,
    unsigned long _forceShowAckStart);
private:
    LiquidCrystal_I2C lcd;
    DisplayState state;
    char buffer[2048];
    int start = 0;
    int end = 16;
    unsigned long lastUpdate = 0;

    int bottomPosition = 0;
};


#endif //WIFI_COMM_DISPLAY_H
