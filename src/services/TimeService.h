#pragma once

#include <Arduino.h>

class TimeService {
public:
    static void begin();
    static void retryIfNeeded();
    static long getServerTime();  // Unix timestamp
    static bool isReady();

private:
    static long baseServerTime;      // Unix timestamp from server
    static unsigned long baseMillis; // ESP millis() at sync
    static bool synced;
    static unsigned long lastSyncAttempt;
};