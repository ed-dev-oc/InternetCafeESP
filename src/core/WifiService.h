#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>

class WifiService
{
public:
    static bool connect(unsigned long timeoutMs = 15000);
    static void loop();
    static bool isSetupMode();
    static bool isConnected();

private:
    static bool setupMode;
    static unsigned long lastReconnectAttempt;

    static bool connectStation(unsigned long timeoutMs);
    static void startSetupAccessPoint();
    static void applyStaticIp();
    static void setHostname();
};
