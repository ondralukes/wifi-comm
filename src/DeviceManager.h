#ifndef WIFI_COMM_DEVICEMANAGER_H
#define WIFI_COMM_DEVICEMANAGER_H
#include <IPAddress.h>
#include <WiFiUdp.h>
#include "Display.h"

struct Message;
struct Device{
    Device(uint32_t id, const IPAddress &ip);
    Message* messageToAck = nullptr;
    unsigned long lastAttemptTime;
    uint32_t id;
    uint8_t lastPacketId = 255;
    bool upstream;
    IPAddress address;
    unsigned long lastSeen;
    char name[64];
};

struct DeviceListNode{
    DeviceListNode(Device &d, DeviceListNode* next)
        :d(d), next(next){}
    Device d;
    DeviceListNode* next;
};

struct DeviceIterator{
public:
    void Next();
    Device & Current();
    bool Ended();
private:
    explicit DeviceIterator(DeviceListNode **_insert);
    void Insert(Device &d);
    void Remove();
    DeviceListNode** current;
    friend class DeviceManager;
};
class DeviceManager {
public:
    explicit DeviceManager(Display &display);
    void Update();
    DeviceIterator Iterator();
    void SendToAll(Message* msg);
    void SetName(const char* n);

    int upstreamDevices = 0;
    int downstreamDevices = 0;
    int acksRemaining = 0;
    int acks = 0;
    int attempts = 0;
private:
    WiFiUDP udp;
    Display&display;
    char name[128];
    unsigned long lastAnnounce = 0;
    DeviceListNode* list = nullptr;
    uint8_t lastSentId = 0;

    void Announce();
    void HandleIncoming();
    void RemoveOld();
    void RetryFailed();
};


#endif //WIFI_COMM_DEVICEMANAGER_H
