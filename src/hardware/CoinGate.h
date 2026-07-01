#pragma once

class CoinGate
{
public:
    static void begin();
    static void disable();
    static void enable();
    static bool isEnabled();

private:
    static bool enabled;
    static bool dryRun;
};
