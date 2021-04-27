#include <ESP8266WiFi.h>
#include <Esp.h>
#include "Packet.h"
#include "MessageBuilder.h"

void WriteMsgPacket(WiFiUDP &udp, uint32_t targetId, uint32_t packetId, const Message *msg) {
    udp.write(reinterpret_cast<const char *>(&targetId), sizeof(uint32_t));
    udp.write(packetId);
    udp.write(Msg);
    udp.write(msg->message, msg->len);
}

void WriteAnnPacket(WiFiUDP &udp, uint32_t packetId, const char *name) {
    uint32_t chipId = ESP.getChipId();
    udp.write(reinterpret_cast<const char *>(&chipId), sizeof(uint32_t));
    udp.write(packetId);
    udp.write(Ann);
    udp.write(name);
}

void WriteAckPacket(WiFiUDP &udp, uint32_t packetId, uint32_t targetId) {
    uint32_t chipId = ESP.getChipId();
    udp.write(reinterpret_cast<const char *>(&chipId), sizeof(uint32_t));
    udp.write(packetId);
    udp.write(Ack);
    udp.write(reinterpret_cast<const char *>(&targetId), sizeof(uint32_t));
}
