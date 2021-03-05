#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "DeviceManager.h"
#include "global.h"

const char* ssid = "_node_comm";
const char* password = "supersecret123";

bool isAP = false;
DeviceManager deviceManager;
void setup() {
    WiFi.mode(WIFI_OFF);
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);
}

void connect(){
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("Scanning networks...");
    int8_t n = WiFi.scanNetworks(false, false, 0, (uint8 *) ssid);
    bool available = false;
    for(int i = 0;i<n;i++){
        auto scannedSSID = WiFi.SSID(i);
        Serial.println(scannedSSID);
        if(scannedSSID == ssid){
            available = true;
        }
    }
    if(available) {
        Serial.println("Connecting.");
        WiFi.begin(ssid, password);
        unsigned long start = millis();
        while (millis() - start < 30000) {
            if (WiFi.status() == WL_CONNECTED) {
                isAP = false;
                Serial.println("Connected.");
                return;
            }
            delay(100);
            Serial.print(".");
        }
    } else {
        Serial.println("Network not found.");
    }
    Serial.println("Could not connect to network. Creating Access Point.");
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
    Serial.println("Access Point created.");
}

void handle_ui(){
    if(Serial.available() == 0) return;
    while (Serial.available() > 0) {
        char c = Serial.read();
        if(c == 'r'){
            Serial.println("Reset");
            ESP.reset();
        }
    }
}

void status_led(){
    if(!isAP){
        digitalWrite(LED_BUILTIN, LOW);
        return;
    }
    static unsigned long prevBlink = 0;
    static bool state = false;
    if(millis() - prevBlink > 250){
        state = !state;
        prevBlink = millis();
        digitalWrite(LED_BUILTIN, state);
    }
}

void loop() {
    if(WiFi.status() != WL_CONNECTED && !isAP){
        Serial.println("No connection.");
        connect();
    }
    handle_ui();
    deviceManager.Update();
    status_led();
}