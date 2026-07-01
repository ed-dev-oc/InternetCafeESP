#include <Arduino.h>
#include "CoinGate.h"
#include "../config/Pins.h"
#include "../config/AppConfig.h"

bool CoinGate::enabled = false;
bool CoinGate::dryRun = DRY_RUN; // <-- no hardware yet

void CoinGate::begin()
{
    if (dryRun)
    {
        Serial.println("[DRY RUN] CoinGate initialized");
        return;
    }

    pinMode(COIN_RELAY_PIN, OUTPUT);

    disable();
}

void CoinGate::enable()
{
    enabled = true;

    if (dryRun)
    {
        Serial.println("[DRY RUN] Coin acceptor ENABLED");
        return;
    }

    digitalWrite(COIN_RELAY_PIN, LOW);
}

void CoinGate::disable()
{
    enabled = false;

    if (dryRun)
    {
        Serial.println("[DRY RUN] Coin acceptor DISABLED");
        return;
    }

    digitalWrite(COIN_RELAY_PIN, HIGH);
}

bool CoinGate::isEnabled()
{
    return enabled;
}
