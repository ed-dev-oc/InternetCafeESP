#pragma once

#include <stdint.h>

class HeartbeatService
{
public:
    static void begin();
    static void loop();

private:
    static unsigned long lastHeartbeat;
};