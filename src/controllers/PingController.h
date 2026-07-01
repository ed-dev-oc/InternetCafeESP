#pragma once

#include <ESP8266WebServer.h>

class PingController
{
public:
    static void handle(ESP8266WebServer& server);
};