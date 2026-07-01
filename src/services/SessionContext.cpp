#include "SessionContext.h"

#include <LittleFS.h>
#include "TimeService.h"

String SessionContext::currentSessionUid = "";
String SessionContext::currentPcName = "";
unsigned long SessionContext::currentEndedAt = 0;

void SessionContext::begin()
{
    load();
}

void SessionContext::setSessionUid(const String& sessionUid)
{
    currentSessionUid = sessionUid;
    save();
    Serial.printf("[SESSION] uid=%s\n", currentSessionUid.c_str());
}

void SessionContext::setPcName(const String& pcName)
{
    currentPcName = pcName;
    save();
    Serial.printf("[SESSION] pc_name=%s\n", currentPcName.c_str());
}

void SessionContext::setEndedAt(unsigned long endedAt)
{
    currentEndedAt = endedAt;
    save();
    Serial.printf("[SESSION] ended_at=%lu\n", currentEndedAt);
}

String SessionContext::getSessionUid()
{
    return currentSessionUid;
}

String SessionContext::getPcName()
{
    return currentPcName;
}

unsigned long SessionContext::getEndedAt()
{
    return currentEndedAt;
}

bool SessionContext::hasEnded()
{
    if (currentEndedAt == 0) {
        return false;
    }
    long currentTime = TimeService::getServerTime();
    if (currentTime == 0) {
        // Time not synced yet, cannot determine
        return false;
    }
    return (unsigned long)currentTime >= currentEndedAt;
}

void SessionContext::clear()
{
    currentSessionUid = "";
    currentPcName = "";
    currentEndedAt = 0;
    save();
    Serial.println("[SESSION] cleared");
}

void SessionContext::save()
{
    File file = LittleFS.open("/session.txt", "w");
    if (!file) return;
    file.println(currentSessionUid);
    file.println(currentPcName);
    file.println(currentEndedAt);
    file.close();
}

void SessionContext::load()
{
    File file = LittleFS.open("/session.txt", "r");
    if (!file) return;
    currentSessionUid = file.readStringUntil('\n');
    currentSessionUid.trim();
    currentPcName = file.readStringUntil('\n');
    currentPcName.trim();
    String endedAtStr = file.readStringUntil('\n');
    endedAtStr.trim();
    currentEndedAt = endedAtStr.toInt();
    file.close();
    Serial.printf("[SESSION] loaded uid=%s pc=%s ended=%lu\n",
        currentSessionUid.c_str(), currentPcName.c_str(), currentEndedAt);
}