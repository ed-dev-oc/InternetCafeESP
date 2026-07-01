#pragma once

class CoinDetector
{
public:
    static void begin();
    static int consumePulses();

    static void simulatePulse();

private:
    static bool dryRun;
};
