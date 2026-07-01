#include <Arduino.h>
#include "CoinDetector.h"
#include "../config/Pins.h"
#include "../config/AppConfig.h"

volatile uint32_t pulseCount = 0;
volatile uint32_t lastPulseTime = 0;
bool CoinDetector::dryRun = DRY_RUN;

// Minimum time between valid pulses (debounce) in microseconds
const uint32_t DEBOUNCE_US = 1000;  // 1ms debounce

void IRAM_ATTR onCoinPulse()
{
    uint32_t now = micros();
    // Only count if enough time has passed (debounce)
    if (now - lastPulseTime >= DEBOUNCE_US)
    {
        pulseCount++;
        lastPulseTime = now;
    }
}

void CoinDetector::begin()
{
    if (dryRun)
    {
        Serial.println("[DRY RUN] CoinDetector initialized (no interrupt)");
        return;
    }

    pinMode(COIN_PULSE_PIN, INPUT_PULLUP);

    attachInterrupt(
        digitalPinToInterrupt(COIN_PULSE_PIN),
        onCoinPulse,
        FALLING
    );
}

int CoinDetector::consumePulses()
{
    noInterrupts();

    int count = pulseCount;
    pulseCount = 0;

    interrupts();

    return count;
}

void CoinDetector::simulatePulse()
{
    pulseCount++;

    Serial.println("[DRY RUN] Coin pulse simulated");
}