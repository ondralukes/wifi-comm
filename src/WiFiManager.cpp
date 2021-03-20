#include <ESP8266WiFi.h>
#include "WiFiManager.h"
#include "global.h"

const char* password = "12345678";

enum WiFiManager::Status WiFiManager::status = Disabled;
bool WiFiManager::upstreamEnabled = true;
uint32_t WiFiManager::host;
void WiFiManager::CheckConnection() {
    int wifiStatus = WiFi.status();
    if(wifiStatus == WL_CONNECTED || (wifiStatus == WL_IDLE_STATUS && status == Connecting)){
        if(!upstreamEnabled){
            WiFi.disconnect();
            status = Disabled;
        }
        status = Connected;
        return;
    }
    if(!upstreamEnabled){
        status = Disabled;
        return;
    }
    if(wifiStatus == WL_CONNECT_FAILED || status == Disabled || status == Connected){
        WiFi.scanNetworks(true);
        status = Scanning;
        return;
    }
    if(status == Connecting) return;
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
        status = Connecting;
    } else {
        WiFi.scanNetworks(true);
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

enum WiFiManager::Status WiFiManager::Status() {
    return status;
}

uint32_t WiFiManager::HostId() {
    return host;
}
