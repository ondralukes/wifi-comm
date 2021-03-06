#include <Arduino.h>
#include "DeviceManager.h"
#include "MessageBuilder.h"
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
    HandleIncoming();
    RemoveOld();
}

void DeviceManager::Announce() {
    if(millis() - lastAnnounce < 250) return;
    lastAnnounce = millis();
    const static IPAddress broadcast(192,168,1,255);
    if(!udp.beginPacket(broadcast, 9887))
        Serial.println("[Device Manager] Warning: Failed to begin announcement UDP packet.");
    udp.write('a');
    if(!udp.endPacket())
        Serial.println("[Device Manager] Warning: Failed to end announcement UDP packet.");
}

void DeviceManager::HandleIncoming() {
    static char buffer[4096];
    while (udp.parsePacket() != 0){
        auto ip = udp.remoteIP();
        int size = udp.read(buffer, 4096);
        Device* d;
        auto it = Iterator();
        while (!it.Ended()){
            auto& device = it.Current();
            if(device.address == ip){
                device.lastSeen = millis();
                d = &device;
                break;
            }
            it.Next();
        }
        if(it.Ended()) {
            Serial.print("[Device Manager] Found new device ");
            Serial.print(ip);
            Serial.println(" .");
            it.Insert(Device(ip));
            d = &it.Current();
        }
        if(size == 0) continue;
        if(buffer[0] == PACKET_MSG){
            Serial.print(ip);
            Serial.print(">");
            buffer[size] = '\0';
            Serial.println(&buffer[1]);
            udp.beginPacket(ip, 9887);
            udp.write(PACKET_ACK);
            udp.endPacket();
        } else if(buffer[0] == PACKET_ACK){
            if(d->messageToAck == nullptr){
                Serial.println("[Device Manager] Warning: Unexpected ACK.");
                continue;
            }
            d->messageToAck->remainingAcks--;
            if(d->messageToAck->remainingAcks == 0){
                Serial.println("[Device Manager] Message fully acknowledged.");
                free(d->messageToAck);
                d->messageToAck = nullptr;
            } else {
                Serial.print("[Device Manager] Message acknowledged. ");
                Serial.print(d->messageToAck->remainingAcks);
                Serial.println(" acknowledgments remaining.");
            }
        }
    }
}

void DeviceManager::RemoveOld() {
    auto it = Iterator();
    while (!it.Ended()){
        auto& device = it.Current();
        if(millis() - device.lastSeen > 1000){
            Serial.print("[Device Manager] Device ");
            Serial.print(device.address);
            Serial.println(" lost.");
            it.Remove();
            continue;
        }
        it.Next();
    }
}

void DeviceManager::SendToAll(Message *msg) {
    auto it = Iterator();
    while (!it.Ended()){
        auto& device = it.Current();
        if(!udp.beginPacket(device.address, 9887)){
            Serial.println("[Device Manager] Warning: Failed to begin message UDP packet.");
            it.Next();
            continue;
        }
        udp.write(PACKET_MSG);
        udp.write(msg->message, msg->len);
        if(!udp.endPacket()){
            Serial.println("[Device Manager] Warning: Failed to end message UDP packet.");
            it.Next();
            continue;
        }
        msg->remainingAcks++;
        if(device.messageToAck != nullptr){
            Serial.println("[Device Manager] Skipping acknowledgement check of previous message.");
            device.messageToAck->remainingAcks--;
            if(device.messageToAck->remainingAcks == 0){
                free(device.messageToAck);
            }
        }
        device.messageToAck = msg;
        it.Next();
    }
}

Device::Device(const IPAddress& ip): address(ip) {
    lastSeen = millis();
}
