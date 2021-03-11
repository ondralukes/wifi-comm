#ifndef GLOBAL_H
#define GLOBAL_H

#ifndef REVISION
    #define REVISION "unknown"
#endif

#define PACKET_ANNOUNCEMENT 0
#define PACKET_MSG 1
#define PACKET_ACK 2
void throw_error(const char* msg);

#endif //GLOBAL_H
