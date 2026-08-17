#pragma once

#include <ESP8266WebServer.h>

class ConfigController
{
public:
    static void handleIndex(ESP8266WebServer& server);
    static void handleLocalSave(ESP8266WebServer& server);
    static void handleLocalReset(ESP8266WebServer& server);
    static void handleRemoteGet(ESP8266WebServer& server);
    static void handleRemoteSave(ESP8266WebServer& server);
};