#include "TimeService.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "../config/DeviceSettings.h"
#include <ESP8266WiFi.h>

long TimeService::baseServerTime = 0;
unsigned long TimeService::baseMillis = 0;
bool TimeService::synced = false;
unsigned long TimeService::lastSyncAttempt = 0;

void TimeService::begin() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[TIME] WiFi not connected, skipping");
        return;
    }

    String serverUrl = DeviceSettings::serverUrl();
    if (serverUrl.length() == 0) {
        Serial.println("[TIME] server URL missing, skipping");
        return;
    }

    WiFiClient client;
    HTTPClient http;
    String url = serverUrl + "/api/server_time";

    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    http.addHeader("Connection", "close");

    http.setTimeout(5000);
    
    int code = http.GET();
    Serial.printf("[TIME] Response code: %d\n", code);
    
    if (code == 200) {
        String payload = http.getString();
        int idx = payload.indexOf("\"server_time\":");
        if (idx != -1) {
            baseServerTime = payload.substring(idx + 14).toInt();
            baseMillis = millis();
            synced = true;
            Serial.printf("[TIME] synced: server=%lu, millis=%lu\n",
                          baseServerTime, baseMillis);
        }
    } else {
       Serial.printf("[TIME] failed code=%d\n", code);
    }
    http.end();
}

void TimeService::retryIfNeeded() {
    if (synced) return;
    if (WiFi.status() != WL_CONNECTED) return;
    if (millis() - lastSyncAttempt < 10000) return;
    lastSyncAttempt = millis();
    Serial.println("[TIME] retrying sync...");
    begin();
}

long TimeService::getServerTime() {
    if (!synced) return 0;
    return baseServerTime + ((millis() - baseMillis) / 1000);
}

bool TimeService::isReady() {
    return synced;
}
