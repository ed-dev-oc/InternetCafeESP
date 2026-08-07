#pragma once

#include <ESP8266WebServer.h>
#include "../controllers/PingController.h"
#include "../controllers/CoinController.h"
#include "../controllers/DebugController.h"
#include "../controllers/RebootController.h"
#include "../controllers/ConfigController.h"

class ApiRoutes
{
public:
    static void registerRoutes(ESP8266WebServer& server)
    {
        server.on("/", HTTP_GET, [&server]()
        {
            ConfigController::handleIndex(server);
        });

        server.on("/config", HTTP_GET, [&server]()
        {
            ConfigController::handleIndex(server);
        });

        server.on("/config", HTTP_POST, [&server]()
        {
            ConfigController::handleLocalSave(server);
        });

        server.on("/api/device/config", HTTP_GET, [&server]()
        {
            ConfigController::handleRemoteGet(server);
        });

        server.on("/api/device/config", HTTP_POST, [&server]()
        {
            ConfigController::handleRemoteSave(server);
        });

        server.on("/ping", HTTP_GET, [&server]()
        {
            PingController::handle(server);
        });

        server.on("/coin/enable", HTTP_POST, [&server]()
        {
            CoinController::enable(server);
        });

        server.on("/coin/disable", HTTP_POST, [&server]()
        {
            CoinController::disable(server);
        });

        server.on("/debug/coin-insert", HTTP_POST, [&server]()
        {
            DebugController::coinDetectorSimulatePulse(server);
        });

        server.on("/debug/queue", HTTP_GET, [&server]()
        {
            DebugController::getQueue(server);
        });

        server.on("/reboot", HTTP_POST, [&server]()
        {
            RebootController::reboot(server);
        });
    }
};
