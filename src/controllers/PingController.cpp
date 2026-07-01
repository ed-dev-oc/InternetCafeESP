#include "PingController.h"
#include "../config/DeviceConfig.h"
#include "../services/CoinService.h"
#include "../services/SessionContext.h"
#include "../services/RegistrationService.h"
#include "../services/HttpTaskQueue.h"
#include "../services/TimeService.h"
#include <ESP8266WiFi.h>

void PingController::handle(
    ESP8266WebServer& server
)
{
    String sessionUid =
        SessionContext::getSessionUid();

    bool coinEnabled =
        sessionUid.length() > 0;

    bool hasToken = RegistrationService::isRegistered();

    // Get network info
    String macAddress = WiFi.macAddress();
    String ipAddress = WiFi.localIP().toString();

    char json[512];
    snprintf(json, sizeof(json),
        "{\"status\":\"ok\",\"device\":\"%s\",\"has_token\":%s,\"session_uid\":\"%s\","
        "\"coin_enabled\":%s,\"mac_address\":\"%s\",\"ip_address\":\"%s\","
        "\"queue_size\":%d,\"uptime_ms\":%lu,\"time\":%ld,\"free_heap\":%u}",
        DEVICE_NAME,
        hasToken ? "true" : "false",
        sessionUid.c_str(),
        coinEnabled ? "true" : "false",
        macAddress.c_str(),
        ipAddress.c_str(),
        HttpTaskQueue::size(),
        millis(),
        TimeService::getServerTime(),
        ESP.getFreeHeap()
    );

    server.send(
        200,
        "application/json",
        json
    );
}