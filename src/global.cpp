#include <HardwareSerial.h>
#include "global.h"
void throw_error(const char* msg){
    Serial.println("\nFatal error: ");
    Serial.println(msg);
    Serial.println("Please restart the device.");
    while(1);
}