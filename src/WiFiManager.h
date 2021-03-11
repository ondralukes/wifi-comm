#ifndef WIFI_COMM_WIFIMANAGER_H
#define WIFI_COMM_WIFIMANAGER_H

#include "Display.h"

enum WiFiStatus{
    Station,
    AP
};

class WiFiManager {
public:
    static void CheckConnection(Display &lcd);
    static WiFiStatus Status();
private:
    static bool isAP;
};


#endif //WIFI_COMM_WIFIMANAGER_H
