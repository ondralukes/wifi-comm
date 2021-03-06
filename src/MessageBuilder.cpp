#include <HardwareSerial.h>
#include "MessageBuilder.h"

char MessageBuilder::buffer[4096];
int MessageBuilder::len;
void MessageBuilder::BeginMessage() {
    Serial.println("[Message Builder] Begin message.");
    len = 0;
}

void MessageBuilder::Write(char c) {
    buffer[len++] = c;
}

void MessageBuilder::SendMessage(DeviceManager &deviceManager) {
    auto* msg = static_cast<Message *>(malloc(sizeof(Message) + len));
    msg->len = len;
    msg->remainingAcks = 0;
    memcpy(msg->message, buffer, len);
    Serial.print("[Message Builder] Sending ");
    Serial.print(len);
    Serial.println(" bytes.");
    deviceManager.SendToAll(msg);
}
