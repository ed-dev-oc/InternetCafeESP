#pragma once
#include <ESP8266WiFi.h>
#include "local_credentials.h"

// STATIC IP
IPAddress LOCAL_IP(192, 168, 1, 254);
IPAddress GATEWAY(192, 168, 1, 1);
IPAddress SUBNET(255, 255, 255, 0);
IPAddress PRIMARY_DNS(192, 168, 1, 1);
IPAddress SECONDARY_DNS(1, 1, 1, 1);
