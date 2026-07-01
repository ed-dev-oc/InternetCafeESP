#include "HeartbeatService.h"
#include "HttpTaskQueue.h"
#include "RegistrationService.h"
#include "../config/DeviceConfig.h"
#include <ESP8266WiFi.h>

unsigned long HeartbeatService::lastHeartbeat = 0;

void HeartbeatService::begin()
{
    // Nothing to initialize
}

void HeartbeatService::loop()
{
    if (!RegistrationService::isRegistered()) return;
    if (WiFi.status() != WL_CONNECTED) return;

    if (millis() - lastHeartbeat >= HEARTBEAT_INTERVAL_MS || lastHeartbeat == 0)
    {
        lastHeartbeat = millis();
        HttpTaskQueue::enqueueHeartbeat();
    }
}