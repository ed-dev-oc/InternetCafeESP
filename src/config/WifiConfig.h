#pragma once

#include <ESP8266WiFi.h>

// Setup access point network defaults.
// Runtime Wi-Fi credentials live in DeviceSettings.
static const IPAddress SETUP_AP_IP(192, 168, 4, 1);
static const IPAddress SETUP_AP_GATEWAY(192, 168, 4, 1);
static const IPAddress SETUP_AP_SUBNET(255, 255, 255, 0);
