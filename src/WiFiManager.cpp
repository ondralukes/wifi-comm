#include <HardwareSerial.h>
#include <ESP8266WiFi.h>
#include "WiFiManager.h"
#include "global.h"

const char* ssid = "_node_comm";
const char* password = "supersecret123";

bool WiFiManager::isAP = false;

void WiFiManager::CheckConnection() {
    if(WiFi.status() == WL_CONNECTED || isAP){
        return;
    }
    Serial.println("[WiFi] Lost connection.");
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("[WiFi] Scanning networks...");
    int8_t n = WiFi.scanNetworks(false, false, 0, (uint8 *) ssid);
    bool available = false;
    for(int i = 0;i<n;i++){
        auto scannedSSID = WiFi.SSID(i);
        if(scannedSSID == ssid){
            available = true;
        }
    }
    if(available) {
        Serial.println("[WiFi] Network found. Connecting.");
        WiFi.begin(ssid, password);
        unsigned long start = millis();
        while (millis() - start < 30000) {
            if (WiFi.status() == WL_CONNECTED) {
                isAP = false;
                Serial.println("[WiFi] Connected.");
                return;
            }
            delay(100);
        }
        Serial.println("[WiFi] Could not connect.");
    } else {
        Serial.println("[WiFi] Network not found.");
    }
    Serial.println("[WiFi] Creating Access Point.");
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
    Serial.println("[WiFi] Access Point created.");
}

WiFiStatus WiFiManager::Status() {
    return isAP? AP : Station;
}
