#ifndef WIFI_COMM_DEVICEMANAGER_H
#define WIFI_COMM_DEVICEMANAGER_H
#include <IPAddress.h>
#include <WiFiUdp.h>
#include "Display.h"

struct Message;
struct Device{
    explicit Device(const IPAddress& ip);
    Message* messageToAck = nullptr;
    IPAddress address;
    unsigned long lastSeen;
};

struct DeviceListNode{
    DeviceListNode(Device d, DeviceListNode* next)
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
    void Insert(Device d);
    void Remove();
    DeviceListNode** current;
    friend class DeviceManager;
};
class DeviceManager {
public:
    DeviceManager(Display &display);
    void Update();
    DeviceIterator Iterator();
    void SendToAll(Message* msg);
private:
    WiFiUDP udp;
    Display&display;
    unsigned long lastAnnounce = 0;
    DeviceListNode* list = nullptr;

    void Announce();
    void HandleIncoming();
    void RemoveOld();
};


#endif //WIFI_COMM_DEVICEMANAGER_H
