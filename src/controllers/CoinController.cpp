#include "CoinController.h"
#include "../services/CoinService.h"
#include "../services/SessionContext.h"
#include "../core/HmacHelper.h"

// Helper: build body string from args in the order received
static String buildBody(ESP8266WebServer& server)
{
    String body;
    body.reserve(256);
    for (int i = 0; i < server.args(); i++) {
        if (i > 0) body += "&";
        body += server.argName(i);
        body += "=";
        body += server.arg(i);
    }
    return body;
}

void CoinController::enable(ESP8266WebServer& server)
{
    String body = buildBody(server);

    Serial.printf("[HMAC] body='%s'\n", body.c_str());
    if (!HmacHelper::verifyRequest(server, body)) {
        return HmacHelper::sendUnauthorized(server);
    }

    String sessionUid = server.arg("session_uid");
    String pcName = server.arg("pc_name");
    String endedAtStr = server.arg("ended_at");

    SessionContext::setSessionUid(sessionUid);
    SessionContext::setPcName(pcName);
    
    if (!endedAtStr.isEmpty()) {
        unsigned long endedAt = endedAtStr.toInt();
        SessionContext::setEndedAt(endedAt);
    }

    CoinService::enable();

    server.send(200, "application/json", R"({"status":"enabled"})");
}

void CoinController::disable(ESP8266WebServer& server)
{
    String body = buildBody(server);

    if (!HmacHelper::verifyRequest(server, body)) {
        return HmacHelper::sendUnauthorized(server);
    }

    String sessionUid = server.arg("session_uid");
    String currentSessionUid = SessionContext::getSessionUid();

    // Only disable if session_uid matches or current session is empty
    if (currentSessionUid.isEmpty()) {
        Serial.println("[COIN] no active session, ignoring disable");
    } else if (!sessionUid.equals(currentSessionUid)) {
        Serial.printf("[COIN] session mismatch (got=%s, current=%s), ignoring disable\n",
            sessionUid.c_str(), currentSessionUid.c_str());
    } else {
        CoinService::disable();
        SessionContext::clear();
    }

    server.send(200, "application/json", R"({"status":"disabled"})");
}
