#pragma once
#include "../models/CoinEvent.h"

class RailsClient
{
public:
    static bool sendCoinEvent(const CoinEvent& event);
    static bool registerCoinSlot(String& outSecret);
    static bool heartbeatCoinSlot(const String& secret, const String& deviceId);
};