#include <Arduino.h>
#include "DeviceManager.h"
#include "global.h"

void DeviceIterator::Next() {
    current = &(*current)->next;
}

Device & DeviceIterator::Current() {
    return (*current)->d;
}

DeviceIterator::DeviceIterator(DeviceListNode **_insert){
    current = _insert;
}

bool DeviceIterator::Ended() {
    return *current == nullptr;
}

void DeviceIterator::Insert(Device d) {
    auto afterNew = *current;
    *current = new DeviceListNode(d, afterNew);
}

void DeviceIterator::Remove() {
    if(*current == nullptr) return;
    auto next = (*current)->next;
    delete *current;
    *current = next;
}

DeviceIterator DeviceManager::Iterator() {
    return DeviceIterator(&list);
}

DeviceManager::DeviceManager() {
    udp.begin(9887);
}

void DeviceManager::Update() {
    Announce();
    Scan();
    RemoveOld();
}

void DeviceManager::Announce() {
    if(millis() - lastAnnounce < 250) return;
    lastAnnounce = millis();
    const static IPAddress broadcast(192,168,1,255);
    if(!udp.beginPacket(broadcast, 9887))
        Serial.println("Warning: Failed to begin UDP packet.");
    udp.write('a');
    if(!udp.endPacket())
        Serial.println("Warning: Failed to end UDP packet.");
}

void DeviceManager::Scan() {
    while (udp.parsePacket() != 0){
        auto ip = udp.remoteIP();
        auto it = Iterator();
        while (!it.Ended()){
            auto& device = it.Current();
            if(device.address == ip){
                device.lastSeen = millis();
                break;
            }
            it.Next();
        }
        if(it.Ended()) {
            Serial.print("Found new device ");
            Serial.print(ip);
            Serial.println(" .");
            it.Insert(Device(ip));
        }
        udp.read();
    }
}

void DeviceManager::RemoveOld() {
    auto it = Iterator();
    while (!it.Ended()){
        auto& device = it.Current();
        if(millis() - device.lastSeen > 1000){
            Serial.print("Device ");
            Serial.print(device.address);
            Serial.println(" lost.");
            it.Remove();
            continue;
        }
        it.Next();
    }
}

Device::Device(const IPAddress& ip): address(ip) {
    lastSeen = millis();
}
