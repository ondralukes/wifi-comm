#include "MessageBuilder.h"

char MessageBuilder::buffer[4096];
int MessageBuilder::len;

void MessageBuilder::BeginMessage() {
    len = 0;
}

void MessageBuilder::Write(char c) {
    buffer[len++] = c;
}

void MessageBuilder::SendMessage(DeviceManager &deviceManager) {
    auto *msg = static_cast<Message *>(malloc(sizeof(Message) + len));
    msg->len = len;
    msg->remainingAcks = 0;
    memcpy(msg->message, buffer, len);
    deviceManager.SendToAll(msg);
}

bool MessageBuilder::Delete() {
    len--;
    if (len <= 0) {
        len = 0;
    }
    return len != 0;
}

void MessageBuilder::Clear() {
    len = 0;
}
