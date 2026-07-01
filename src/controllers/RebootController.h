#pragma once

#include <ESP8266WebServer.h>

class RebootController
{
public:
    static void reboot(ESP8266WebServer& server);
};