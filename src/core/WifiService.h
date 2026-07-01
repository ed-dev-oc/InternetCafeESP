#pragma once

#include <ESP8266WiFi.h>
#include "../config/WifiConfig.h"

class WifiService
{
public:
    static void connect()
    {
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        WiFi.persistent(true);
        // WiFi.config(LOCAL_IP, GATEWAY, SUBNET, PRIMARY_DNS, SECONDARY_DNS);
        
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        Serial.print("Connecting WiFi");

        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
            
            yield();
        }

        Serial.println("\nWiFi Connected");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("Mac Address: ");
        Serial.println(WiFi.macAddress());
    }
};