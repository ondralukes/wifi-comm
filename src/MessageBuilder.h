#ifndef WIFI_COMM_MESSAGEBUILDER_H
#define WIFI_COMM_MESSAGEBUILDER_H

#include "DeviceManager.h"

struct Message {
    uint8_t remainingAcks;
    int len;
    char message[0];
};

class MessageBuilder {
public:
    static void BeginMessage();

    static void Write(char c);

    static bool Delete();

    static void Clear();

    static void SendMessage(DeviceManager &deviceManager);

private:
    static char buffer[4096];
    static int len;
};


#endif //WIFI_COMM_MESSAGEBUILDER_H
