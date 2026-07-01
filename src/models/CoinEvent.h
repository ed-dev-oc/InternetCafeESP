#pragma once

#include <Arduino.h>

struct CoinEvent
{
    String id;
    String macAddress;
    String sessionUid;
    int pulses;
    unsigned long createdAt;
};