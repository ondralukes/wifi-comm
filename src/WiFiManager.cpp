#include <ESP8266WiFi.h>
#include "WiFiManager.h"
#include "global.h"
#include "Display.h"

const char* password = "12345678";

WiFiManager::State WiFiManager::state = Idle;
bool WiFiManager::upstreamEnabled = true;
uint32_t WiFiManager::host;
void WiFiManager::CheckConnection() {
    int status = WiFi.status();
    if(status == WL_CONNECTED || (status == WL_IDLE_STATUS && state == Connecting)){
        if(!upstreamEnabled){
            WiFi.disconnect();
            state = Idle;
        }
        return;
    }
    if(!upstreamEnabled) return;
    if(status == WL_CONNECT_FAILED){
        state = Idle;
    }
    if(state == Connecting) return;
    if(state == Idle){
        WiFi.scanNetworks(true);
        state = Scanning;
        return;
    }
    int8_t n = WiFi.scanComplete();
    // Still running
    if(n == -1) return;

    String targetSSID;
    int32_t bestRSSI = -1000;
    for(int i = 0;i<n;i++){
        auto scannedSSID = WiFi.SSID(i);
        auto rssi = WiFi.RSSI(i);
        if(scannedSSID.startsWith("_wifi_comm_") && rssi > bestRSSI){
            bestRSSI = rssi;
            targetSSID = scannedSSID;
        }
    }
    if(bestRSSI != -1000) {
        WiFi.begin(targetSSID, password);
        host = strtol(targetSSID.c_str()+11, nullptr, 16);
        state = Connecting;
    }
}

void WiFiManager::Init() {
    WiFi.mode(WIFI_AP_STA);
    uint32_t chipId = ESP.getChipId();
    uint8_t ip1 = chipId&0xffu;
    if(ip1 == 0) ip1 = 1;
    if(ip1 == 25) ip1 = 254;

    uint8_t ip2 = (chipId>>8u)&0xffu;
    if(ip2 == 0) ip2 = 1;
    if(ip2 == 25) ip2 = 254;

    uint8_t ip3 = (chipId>>16u)&0xffu;
    if(ip3 == 0) ip3 = 1;
    if(ip3 == 25) ip3 = 254;

    if(!WiFi.softAPConfig(
            IPAddress(ip3,ip2,ip1,1),
            IPAddress(ip3,ip2,ip1,1),
            IPAddress(255,255,255,0)
    )){
        throw_error("Failed to configure AP.");
    }
    char ssid[64];
    snprintf(ssid, 64, "_wifi_comm_%x", chipId);
    if(!WiFi.softAP(ssid, password)){
        throw_error("Failed to start AP.");
    }
}

bool WiFiManager::Connected() {
    return WiFi.status() == WL_CONNECTED;
}

uint32_t WiFiManager::HostId() {
    return host;
}
