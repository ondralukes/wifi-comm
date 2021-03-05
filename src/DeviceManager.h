#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H
#include <IPAddress.h>
#include <WiFiUdp.h>

struct Device{
    explicit Device(const IPAddress& ip);
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
    DeviceManager();
    void Update();
    DeviceIterator Iterator();

private:
    WiFiUDP udp;
    unsigned long lastAnnounce = 0;
    DeviceListNode* list = nullptr;

    void Announce();
    void Scan();
    void RemoveOld();
};


#endif //UNTITLED1_DEVICEMANAGER_H
