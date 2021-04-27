#ifndef WIFI_COMM_WIFIMANAGER_H
#define WIFI_COMM_WIFIMANAGER_H

class Display;

class WiFiManager {
public:
    enum Status {
        Scanning,
        Connecting,
        Disabled,
        Connected
    };

    static void Init(const char *networkCode);

    static void CheckConnection();

    static enum Status Status();

    static uint32_t HostId();

    static bool upstreamEnabled;
private:
    static enum Status status;
    static uint32_t host;
    static char prefix[128];
    static char password[128];
};


#endif //WIFI_COMM_WIFIMANAGER_H
