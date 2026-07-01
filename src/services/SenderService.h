#pragma once

class SenderService
{
public:
    static void loop();
    static unsigned long calcBackoff(int retryCount);
};