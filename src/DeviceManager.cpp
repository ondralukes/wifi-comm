#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <cstring>
#include "DeviceManager.h"
#include "MessageBuilder.h"
#include "WiFiManager.h"
#include "Packet.h"

void DeviceIterator::Next() {
    current = &(*current)->next;
}

Device &DeviceIterator::Current() {
    return (*current)->d;
}

DeviceIterator::DeviceIterator(DeviceListNode **_insert) {
    current = _insert;
}

bool DeviceIterator::Ended() {
    return *current == nullptr;
}

void DeviceIterator::Insert(Device &d) {
    auto afterNew = *current;
    *current = new DeviceListNode(d, afterNew);
}

void DeviceIterator::Remove() {
    if (*current == nullptr) return;
    auto next = (*current)->next;
    delete *current;
    *current = next;
}

DeviceIterator DeviceManager::Iterator() {
    return DeviceIterator(&list);
}

DeviceManager::DeviceManager(Display &display) : display(display) {
    udp.begin(9887);
}

void DeviceManager::Update() {
    Announce();
    HandleIncoming();
    RetryFailed();
    RemoveOld();
}

void DeviceManager::Announce() {
    if (millis() - lastAnnounce < 250) return;
    lastAnnounce = millis();
    if (WiFi.status() == WL_CONNECTED) {
        IPAddress broadcast = WiFi.gatewayIP();
        broadcast[3] = 255;
        if (!udp.beginPacket(broadcast, 9887))
            display.WriteRolling("!0");
        WriteAnnPacket(udp, lastSentId, name);
        if (!udp.endPacket())
            display.WriteRolling("!1");
    }

    IPAddress broadcast = WiFi.softAPIP();
    broadcast[3] = 255;
    if (!udp.beginPacket(broadcast, 9887))
        display.WriteRolling("!2");
    WriteAnnPacket(udp, lastSentId++, name);
    if (!udp.endPacket())
        display.WriteRolling("!3");
}

void DeviceManager::HandleIncoming() {
    static char buffer[4096];
    while (udp.parsePacket() != 0) {
        auto ip = udp.remoteIP();
        int size = udp.read(buffer, 4096);
        if (size < (int) sizeof(uint32_t) + 1) continue;
        auto *p = reinterpret_cast<Packet *>(buffer);
        uint32_t id = p->srcId;
        uint32_t myId = ESP.getChipId();
        if (p->srcId == myId) continue;
        // Drop all non-ACK packets from host's station interface
        if (WiFiManager::Status() == WiFiManager::Connected && id == WiFiManager::HostId() && ip != WiFi.gatewayIP()) {
            if (size < (int) sizeof(uint32_t) * 2 + 2 || p->type == Ack) {
                continue;
            }
        }
        Device *d = nullptr;
        auto it = Iterator();
        while (!it.Ended()) {
            auto &device = it.Current();
            if (device.id == id) {
                // Switch IP addresses only after 300 ms to avoid rapid IP switching
                if (device.address != ip && (millis() - device.lastSeen) > 625) {
                    if (device.upstream) {
                        upstreamDevices--;
                    } else {
                        downstreamDevices--;
                    }
                    if (ip == WiFi.gatewayIP() && WiFiManager::Status() == WiFiManager::Connected) {
                        upstreamDevices++;
                        device.upstream = true;
                    } else {
                        downstreamDevices++;
                        device.upstream = false;
                    }
                    device.address = ip;
                }
                device.lastSeen = millis();
                d = &device;
                break;
            }
            it.Next();
        }
        if (it.Ended()) {
            Device newDevice(id, ip);
            it.Insert(newDevice);
            if (newDevice.upstream) {
                upstreamDevices++;
            } else {
                downstreamDevices++;
            }
            d = &it.Current();
        }

        if (d->lastPacketId >= p->packetId && d->lastPacketId < 250) {
            continue;
        }
        d->lastPacketId = p->packetId;

        bool forward = true;

        if (size >= (int) sizeof(uint32_t) + 2 && p->type == Msg) {
            display.WriteRolling(" ");
            display.WriteRolling(d->name);
            display.WriteRolling(">");
            buffer[size] = '\0';
            display.WriteRolling(p->data);

            // Send ACK
            udp.beginPacket(d->address, 9887);
            WriteAckPacket(udp, lastSentId++, d->id);
            udp.endPacket();
        } else if (size >= (int) sizeof(uint32_t) * 2 + 2 && p->type == Ack) {
            if (p->targetId == myId) {
                if (d->messageToAck == nullptr) {
                    display.WriteRolling("!4");
                    continue;
                }
                d->messageToAck->remainingAcks--;
                acksRemaining--;
                if (d->messageToAck->remainingAcks == 0) {
                    free(d->messageToAck);
                }
                d->messageToAck = nullptr;
                forward = false;
            }
        } else if (size >= (int) sizeof(uint32_t) + 3 && p->type == Ann) {
            buffer[size] = '\0';
            strcpy(d->name, p->data);
        }
        if (!forward || WiFi.status() != WL_CONNECTED) continue;
        if (ip != WiFi.gatewayIP()) {
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
    while (!it.Ended()) {
        auto &device = it.Current();
        if (millis() - device.lastSeen > 1000) {
            if (device.upstream) {
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
    if (WiFi.status() == WL_CONNECTED) {
        IPAddress broadcast = WiFi.gatewayIP();
        broadcast[3] = 255;
        if (!udp.beginPacket(broadcast, 9887)) {
            display.WriteRolling("!5");
            return;
        }
        WriteMsgPacket(udp, id, lastSentId, msg);
        if (!udp.endPacket()) {
            display.WriteRolling("!6");
        }
    }

    IPAddress broadcast = WiFi.softAPIP();
    broadcast[3] = 255;
    if (!udp.beginPacket(broadcast, 9887)) {
        display.WriteRolling("!7");
        return;
    }
    WriteMsgPacket(udp, id, lastSentId++, msg);
    if (!udp.endPacket()) {
        display.WriteRolling("!8");
    }
    auto it = Iterator();
    while (!it.Ended()) {
        auto &device = it.Current();
        msg->remainingAcks++;
        acksRemaining++;
        if (device.messageToAck != nullptr) {
            display.WriteRolling("!9");
            device.messageToAck->remainingAcks--;
            acksRemaining--;
            if (device.messageToAck->remainingAcks == 0) {
                free(device.messageToAck);
            }
        }
        device.messageToAck = msg;
        device.lastAttemptTime = millis();
        it.Next();
    }
    attempts = 1;
    acks = acksRemaining;
}

void DeviceManager::RetryFailed() {
    unsigned long time = millis();
    uint32_t id = ESP.getChipId();
    auto it = Iterator();
    while (!it.Ended()) {
        auto &device = it.Current();
        if (device.messageToAck != nullptr && time - device.lastAttemptTime > 250) {
            if (!udp.beginPacket(device.address, 9887)) {
                display.WriteRolling("!a");
                return;
            }
            WriteMsgPacket(udp, id, lastSentId++, device.messageToAck);
            if (!udp.endPacket()) {
                display.WriteRolling("!b");
            }
            device.lastAttemptTime = time;
            attempts++;
        }
        it.Next();
    }
}

void DeviceManager::SetName(const char *n) {
    strcpy(name, n);
}

Device::Device(uint32_t id, const IPAddress &ip) : id(id), address(ip) {
    lastSeen = millis();
    upstream = address == WiFi.gatewayIP() && WiFiManager::Status() == WiFiManager::Connected;
    strcpy(name, "???");
}
