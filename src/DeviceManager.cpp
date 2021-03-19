#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "DeviceManager.h"
#include "MessageBuilder.h"
#include "global.h"
#include "WiFiManager.h"

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
    uint32_t chipId = ESP.getChipId();
    if(WiFi.status() == WL_CONNECTED) {
        IPAddress broadcast = WiFi.gatewayIP();
        broadcast[3] = 255;
        if (!udp.beginPacket(broadcast, 9887))
            display.WriteRolling("!UDP_ANN_BEGIN_ERR");
        udp.write(reinterpret_cast<const char *>(&chipId), sizeof(uint32_t));
        udp.write(lastSentId);
        if (!udp.endPacket())
            display.WriteRolling("!UDP_ANN_END_ERR");
    }

    IPAddress broadcast = WiFi.softAPIP();
    broadcast[3] = 255;
    if (!udp.beginPacket(broadcast, 9887))
        display.WriteRolling("!UDP_ANN_BEGIN_ERR");
    udp.write(reinterpret_cast<const char *>(&chipId), sizeof(uint32_t));
    udp.write(lastSentId++);
    if (!udp.endPacket())
        display.WriteRolling("!UDP_ANN_END_ERR");
}

void DeviceManager::HandleIncoming() {
    static char buffer[4096];
    while (udp.parsePacket() != 0){
        auto ip = udp.remoteIP();
        int size = udp.read(buffer, 4096);
        if(size < sizeof(uint32_t)+1) continue;
        uint32_t id = *reinterpret_cast<uint32_t *>(&buffer);
        uint32_t myId = ESP.getChipId();
        if(id == myId) continue;
        // Drop all non-ACK packets from host's station interface
        if(WiFiManager::Connected() && id == WiFiManager::HostId() && ip != WiFi.gatewayIP()){
            if(size < sizeof(uint32_t)*2+2 || buffer[sizeof(uint32_t)+1] != PACKET_ACK) {
                continue;
            }
        }
        Device* d;
        auto it = Iterator();
        while (!it.Ended()){
            auto& device = it.Current();
            if(device.id == id){
                device.lastSeen = millis();
                // Keep host as upstream device
                if(device.address != ip && ( ip==WiFi.gatewayIP() || id!=WiFiManager::HostId() || !WiFiManager::Connected())){
                    if(device.upstream){
                        upstreamDevices--;
                    } else {
                        downstreamDevices--;
                    }
                    if(ip == WiFi.gatewayIP() && WiFiManager::Connected()){
                        upstreamDevices++;
                    } else {
                        downstreamDevices++;
                    }
                    device.address = ip;
                }
                d = &device;
                break;
            }
            it.Next();
        }
        if(it.Ended()) {
            display.WriteRolling(" ");
            display.WriteRollingHex(id);
            display.WriteRolling("[+]");
            Device newDevice(id, ip);
            it.Insert(newDevice);
            if(newDevice.upstream){
                upstreamDevices++;
            } else {
                downstreamDevices++;
            }
            d = &it.Current();
        }

        uint8_t packetId = buffer[sizeof(uint32_t)];
        if(d->lastPacketId >= packetId && d->lastPacketId < 250){
            continue;
        }
        d->lastPacketId = packetId;

        bool forward = true;

        if(size >= sizeof(uint32_t)+2 && buffer[sizeof(uint32_t)+1] == PACKET_MSG){
            display.WriteRolling(" ");
            display.WriteRollingHex(id);
            display.WriteRolling(">");
            buffer[size] = '\0';
            display.WriteRolling(&buffer[sizeof(uint32_t)+2]);

            // Send ACK
            udp.beginPacket(d->address, 9887);
            udp.write(reinterpret_cast<const char *>(&myId), sizeof(uint32_t));
            udp.write(lastSentId++);
            udp.write(PACKET_ACK);
            udp.write(reinterpret_cast<const char *>(&d->id), sizeof(uint32_t));
            udp.endPacket();
        } else if(size >= sizeof(uint32_t)*2+2 && buffer[sizeof(uint32_t)+1] == PACKET_ACK){
            uint32_t targetId = *reinterpret_cast<uint32_t *>(&buffer[sizeof(uint32_t) + 2]);
            if(targetId == myId) {
                if (d->messageToAck == nullptr) {
                    display.WriteRolling("!ACK_UNEXP");
                    continue;
                }
                d->messageToAck->remainingAcks--;
                if (d->messageToAck->remainingAcks == 0) {
                    display.WriteRolling("ACK OK.");
                    free(d->messageToAck);
                    d->messageToAck = nullptr;
                } else {
                    display.WriteRolling("ACK ");
                    display.WriteRollingInt(d->messageToAck->remainingAcks);
                    display.WriteRolling(" REMAIN.");
                }
                forward = false;
            }
        }
        if(!forward || WiFi.status() != WL_CONNECTED) continue;
        if(ip != WiFi.gatewayIP()) {
            udp.beginPacket(WiFi.gatewayIP(), 9887);
            udp.write(buffer, size);
            udp.endPacket();
        } else {
            IPAddress broadcast = WiFi.softAPIP();
            broadcast[3] = 255;
            udp.beginPacket(broadcast, 9887);
            udp.write(buffer, size);
            udp.endPacket();
        }
    }
}

void DeviceManager::RemoveOld() {
    auto it = Iterator();
    while (!it.Ended()){
        auto& device = it.Current();
        if(millis() - device.lastSeen > 1000){
            display.WriteRolling(" ");
            display.WriteRollingHex(device.id);
            display.WriteRolling("[-]");
            if(device.upstream){
                upstreamDevices--;
            } else {
                downstreamDevices--;
            }
            it.Remove();
            continue;
        }
        it.Next();
    }
}

void DeviceManager::SendToAll(Message *msg) {
    uint32_t id = ESP.getChipId();
    if(WiFi.status() == WL_CONNECTED) {
        IPAddress broadcast = WiFi.gatewayIP();
        broadcast[3] = 255;
        if (!udp.beginPacket(broadcast, 9887)) {
            display.WriteRolling("!UP_MSG_BEGIN_ERR");
            return;
        }
        udp.write(reinterpret_cast<const char *>(&id), sizeof(uint32_t));
        udp.write(lastSentId);
        udp.write(PACKET_MSG);
        udp.write(msg->message, msg->len);
        if (!udp.endPacket()) {
            display.WriteRolling("!UP_MSG_END_ERR");
        }
    }

    IPAddress broadcast = WiFi.softAPIP();
    broadcast[3] = 255;
    if (!udp.beginPacket(broadcast, 9887)) {
        display.WriteRolling("!DOWN_MSG_BEGIN_ERR");
        return;
    }
    udp.write(reinterpret_cast<const char *>(&id), sizeof(uint32_t));
    udp.write(lastSentId++);
    udp.write(PACKET_MSG);
    udp.write(msg->message, msg->len);
    if (!udp.endPacket()) {
        display.WriteRolling("!DOWN_MSG_END_ERR");
    }
    auto it = Iterator();
    while (!it.Ended()){
        auto& device = it.Current();
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

Device::Device(uint32_t id, const IPAddress &ip) :  id(id), address(ip) {
    lastSeen = millis();
    upstream = address == WiFi.gatewayIP();
}
