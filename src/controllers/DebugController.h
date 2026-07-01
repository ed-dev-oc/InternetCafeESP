#pragma once

#include <ESP8266WebServer.h>

class DebugController
{
public:
    static void coinDetectorSimulatePulse(ESP8266WebServer& server);
    static void getQueue(ESP8266WebServer& server);
};
