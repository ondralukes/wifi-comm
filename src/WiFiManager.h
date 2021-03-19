#ifndef WIFI_COMM_WIFIMANAGER_H
#define WIFI_COMM_WIFIMANAGER_H

#include "Display.h"

class WiFiManager {
public:
    static void Init();
    static void CheckConnection();
    static bool Connected();
    static uint32_t HostId();
    static bool upstreamEnabled;
private:
    enum State{
        Scanning,
        Connecting,
        Idle
    };
    static State state;
    static uint32_t host;
};


#endif //WIFI_COMM_WIFIMANAGER_H
