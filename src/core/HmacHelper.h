#pragma once

#include <ESP8266WebServer.h>

namespace HmacHelper {
    bool verifyRequest(ESP8266WebServer& server, const String& body = "");
    void sendUnauthorized(ESP8266WebServer& server);
    String generateSignature(const String& secret, const String& method,
                             const String& path, const String& timestamp,
                             const String& body);
}