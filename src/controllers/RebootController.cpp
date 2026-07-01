#include "RebootController.h"
#include "../core/HmacHelper.h"

void RebootController::reboot(ESP8266WebServer& server)
{
    if (!HmacHelper::verifyRequest(server, "")) {
        return HmacHelper::sendUnauthorized(server);
    }

    server.send(200, "application/json", R"({"status":"ok","message":"Rebooting in 5 seconds"})");
    
    delay(5000);
    
    ESP.restart();
}