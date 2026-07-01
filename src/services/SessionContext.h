#pragma once

#include <Arduino.h>

class SessionContext
{
public:
    static void begin();

    static void setSessionUid(const String& sessionUid);
    static void setPcName(const String& pcName);
    static void setEndedAt(unsigned long endedAt);

    static String getSessionUid();
    static String getPcName();
    static unsigned long getEndedAt();
    static bool hasEnded();

    static void clear();

private:
    static String currentSessionUid;
    static String currentPcName;
    static unsigned long currentEndedAt;

    static void save();
    static void load();
};