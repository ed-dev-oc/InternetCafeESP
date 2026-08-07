#include "HmacHelper.h"
#include <bearssl/bearssl_hmac.h>
#include "../services/RegistrationService.h"
#include "../services/TimeService.h"

static const unsigned long MAX_TIMESTAMP_AGE_MS = 5 * 60 * 1000; // 5 minutes

static String computeHmac(const String& secret, const String& data) {
    br_hmac_key_context kc;
    br_hmac_key_init(&kc, &br_sha256_vtable, secret.c_str(), secret.length());
    
    br_hmac_context ctx;
    br_hmac_init(&ctx, &kc, 0);
    br_hmac_update(&ctx, data.c_str(), data.length());
    
    uint8_t out[32];
    br_hmac_out(&ctx, out);
    
    char sig[65];
    for (int i = 0; i < 32; i++) {
        snprintf(sig + i * 2, 3, "%02x", out[i]);
    }
    sig[64] = '\0';
    return String(sig);
}

namespace HmacHelper {
    bool verifyRequest(ESP8266WebServer& server, const String& body)
    {
        if (!RegistrationService::isRegistered()) {
            Serial.println("[HMAC] refusing request because device is not registered");
            return false;
        }

        String signature = server.header("X-SIGNATURE");
        String timestamp = server.header("X-TIMESTAMP");
        
        if (signature.length() == 0 || timestamp.length() == 0) {
            Serial.println("[HMAC] Missing signature or timestamp header");
            return false;
        }

        unsigned long ts = timestamp.toInt();
        unsigned long now = TimeService::getServerTime();
        if (abs((long)(now - ts)) > MAX_TIMESTAMP_AGE_MS / 1000) {
            Serial.printf("[HMAC] Timestamp too old: now=%lu, ts=%lu\n", now, ts);
            return false;
        }

        String path = server.uri();
        String data = String("SERVER|") + path + "|" + timestamp + "|" + body;
        
        String secret = RegistrationService::getSecret();
        String expected = computeHmac(secret, data);
        
        if (signature != expected) {
            Serial.printf("[HMAC] Signature mismatch\nexpected: %s\ngot:      %s\n", expected.c_str(), signature.c_str());
            return false;
        }

        return true;
    }

    void sendUnauthorized(ESP8266WebServer& server) {
        server.send(401, "application/json", R"({"error":"unauthorized"})");
    }

    String generateSignature(const String& secret, const String& method,
                             const String& path, const String& timestamp,
                             const String& body) {
        String data = method + "|" + path + "|" + timestamp + "|" + body;
        return computeHmac(secret, data);
    }
}
