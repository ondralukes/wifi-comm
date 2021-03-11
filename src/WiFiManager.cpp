#include <ESP8266WiFi.h>
#include "WiFiManager.h"
#include "global.h"
#include "Display.h"

const char* ssid = "_node_comm";
const char* password = "supersecret123";

bool WiFiManager::isAP = false;

void WiFiManager::CheckConnection(Display &lcd) {
    if(WiFi.status() == WL_CONNECTED || isAP){
        return;
    }
    digitalWrite(LED_BUILTIN, HIGH);
    lcd.ShowLoading("Scanning...");
    int8_t n = WiFi.scanNetworks(false, false, 0, (uint8 *) ssid);
    bool available = false;
    for(int i = 0;i<n;i++){
        auto scannedSSID = WiFi.SSID(i);
        if(scannedSSID == ssid){
            available = true;
        }
    }
    if(available) {
        lcd.ShowLoading("Connecting...");
        WiFi.begin(ssid, password);
        unsigned long start = millis();
        while (millis() - start < 30000) {
            if (WiFi.status() == WL_CONNECTED) {
                isAP = false;
                lcd.ShowLoading("Connected.");
                delay(1000);
                lcd.Clear();
                return;
            }
            delay(100);
        }
        lcd.ShowLoading("Failed.");
    } else {
    }
    lcd.ShowLoading("Creating AP...");
    if(!WiFi.softAPConfig(
            IPAddress(192,168,1,1),
            IPAddress(192,168,1,1),
            IPAddress(255,255,255,0)
    )){
        throw_error("Failed to configure AP.");
    }
    if(!WiFi.softAP(ssid, password)){
        throw_error("Failed to start AP.");
    }
    isAP = true;
    lcd.ShowLoading("AP created.");
    delay(1000);
    lcd.Clear();
}

WiFiStatus WiFiManager::Status() {
    return isAP? AP : Station;
}
