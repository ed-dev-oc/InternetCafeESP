#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "CoinService.h"
#include "../hardware/CoinGate.h"
#include "RailsClient.h"
#include "HttpTaskQueue.h"
#include "../models/CoinEvent.h"
#include "SessionContext.h"
#include "TimeService.h"

bool CoinService::accepting = false;
int CoinService::accumulatedPulses = 0;
unsigned long CoinService::lastPulseTime = 0;

void CoinService::enable()
{
    accepting = true;
    CoinGate::enable();
}

void CoinService::disable()
{
    accepting = false;
    CoinGate::disable();
    // Flush any remaining pulses when disabling
    if (accumulatedPulses > 0)
    {
        processAccumulatedPulses();
    }
}

bool CoinService::isAccepting()
{
    return accepting;
}

void CoinService::onCoinPulses(int pulses)
{
    if (pulses <= 0)
    {
        return;
    }

    // Accumulate pulses instead of sending immediately
    accumulatedPulses += pulses;
    lastPulseTime = millis();

    Serial.printf("[COIN] accumulated pulses: %d\n", accumulatedPulses);
}

void CoinService::loop()
{
    // Check if we have accumulated pulses and timeout has passed
    if (accumulatedPulses > 0 && (millis() - lastPulseTime) >= PULSE_TIMEOUT_MS)
    {
        processAccumulatedPulses();
    }

    // Check if session has ended (auto-disable)
    if (accepting && SessionContext::hasEnded())
    {
        Serial.println("[COIN] session ended, auto-disabling");
        disable();
        SessionContext::clear();
    }
}

void CoinService::processAccumulatedPulses()
{
    if (accumulatedPulses <= 0)
    {
        return;
    }

    String sessionUid = SessionContext::getSessionUid();
    if (sessionUid.isEmpty())
    {
        Serial.println("[COIN] no active session, discarding pulses");
        accumulatedPulses = 0;
        return;
    }

    CoinEvent event;
    char id[32];
    snprintf(id, sizeof(id), "%u-%lu", ESP.getChipId(), millis());
    event.id = id;
    event.macAddress = WiFi.macAddress();
    event.sessionUid = sessionUid;
    event.pulses = accumulatedPulses;
    event.createdAt = millis();

    accumulatedPulses = 0;

    HttpTaskQueue::enqueueCoinEvent(event);
    Serial.printf("[QUEUE] event=%s session=%s pulses=%d\n",
        event.id.c_str(), event.sessionUid.c_str(), event.pulses);
}