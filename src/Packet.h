#ifndef WIFI_COMM_PACKET_H
#define WIFI_COMM_PACKET_H

#include <cstdint>
#include "MessageBuilder.h"

enum PacketType : char {
    Ack = 2,
    Msg = 1,
    Ann = 0
};
struct Packet {
    uint32_t srcId;
    uint8_t packetId;
    PacketType type;
    union {
        char data[4];
        uint32_t targetId;
    }__attribute__((packed));
} __attribute__((packed));

void WriteMsgPacket(WiFiUDP &udp, uint32_t targetId, uint32_t packetId, const Message *msg);

void WriteAnnPacket(WiFiUDP &udp, uint32_t packetId, const char *name);

void WriteAckPacket(WiFiUDP &udp, uint32_t packetId, uint32_t targetId);

#endif //WIFI_COMM_PACKET_H
