#pragma once

class CoinService
{
private:
    static bool accepting;
    static int accumulatedPulses;
    static unsigned long lastPulseTime;
    static const unsigned long PULSE_TIMEOUT_MS = 500;  // Wait 500ms after last pulse before sending
    static void processAccumulatedPulses();  // Send accumulated pulses as one event

public:
    static void enable();
    static void disable();
    static bool isAccepting();
    static void onCoinPulses(int pulses);
    static void loop();  // Call in main loop to process accumulated pulses
};