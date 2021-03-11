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

DeviceManager::DeviceManager(Display &display) : display(display){
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
        display.WriteRolling("!UDP_ANN_BEGIN_ERR");
    udp.write('a');
    if(!udp.endPacket())
        display.WriteRolling("!UDP_ANN_END_ERR");
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
            display.WriteRollingInt(ip[3]);
            display.WriteRolling("[+]");
            it.Insert(Device(ip));
            d = &it.Current();
        }
        if(size == 0) continue;
        if(buffer[0] == PACKET_MSG){
            display.WriteRollingInt(ip[3]);
            display.WriteRolling(">");
            buffer[size] = '\0';
            display.WriteRolling(&buffer[1]);
            udp.beginPacket(ip, 9887);
            udp.write(PACKET_ACK);
            udp.endPacket();
        } else if(buffer[0] == PACKET_ACK){
            if(d->messageToAck == nullptr){
                display.WriteRolling("!ACK_UNEXP");
                continue;
            }
            d->messageToAck->remainingAcks--;
            if(d->messageToAck->remainingAcks == 0){
                display.WriteRolling("ACK OK.");
                free(d->messageToAck);
                d->messageToAck = nullptr;
            } else {
                display.WriteRolling("ACK ");
                display.WriteRollingInt(d->messageToAck->remainingAcks);
                display.WriteRolling(" REMAIN.");
            }
        }
    }
}

void DeviceManager::RemoveOld() {
    auto it = Iterator();
    while (!it.Ended()){
        auto& device = it.Current();
        if(millis() - device.lastSeen > 1000){
            display.WriteRollingInt(device.address[3]);
            display.WriteRolling("[-]");
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
            display.WriteRolling("!UDP_MSG_BEGIN_ERR");
            it.Next();
            continue;
        }
        udp.write(PACKET_MSG);
        udp.write(msg->message, msg->len);
        if(!udp.endPacket()){
            display.WriteRolling("!UDP_MSG_END_ERR");
            it.Next();
            continue;
        }
        msg->remainingAcks++;
        if(device.messageToAck != nullptr){
            display.WriteRolling("!ACK_SKIP");
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
