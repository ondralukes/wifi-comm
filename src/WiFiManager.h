#ifndef WIFI_COMM_WIFIMANAGER_H
#define WIFI_COMM_WIFIMANAGER_H

#include "Display.h"

class WiFiManager {
public:
    enum Status{
        Scanning,
        Connecting,
        Disabled,
        Connected
    };
    static void Init();
    static void CheckConnection();
    static enum Status Status();
    static uint32_t HostId();
    static bool upstreamEnabled;
private:
    static enum Status status;
    static uint32_t host;
};


#endif //WIFI_COMM_WIFIMANAGER_H
