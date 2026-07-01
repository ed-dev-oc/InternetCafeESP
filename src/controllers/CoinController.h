#pragma once

#include <ESP8266WebServer.h>

class CoinController
{
public:
    static void enable(ESP8266WebServer& server);
    static void disable(ESP8266WebServer& server);
};
