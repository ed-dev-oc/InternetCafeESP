#include <Arduino.h>
#include "RailsClient.h"
#include "RegistrationService.h"
#include "TimeService.h"
#include "../core/HmacHelper.h"
#include "../config/DeviceConfig.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

static void addHmacHeaders(HTTPClient& http, const String& secret,
                           const String& method, const String& path,
                           const String& body = "") {
    String timestamp = String(TimeService::getServerTime());
    String signature = HmacHelper::generateSignature(secret, method, path, timestamp, body);
    String deviceId = "esp-" + String(ESP.getChipId());
    http.addHeader("X-DEVICE-ID", deviceId);
    http.addHeader("X-TIMESTAMP", timestamp);
    http.addHeader("X-SIGNATURE", signature);
}


bool RailsClient::sendCoinEvent(const CoinEvent& event)
{
    WiFiClient client;
    HTTPClient http;
    
    char url[256];
    snprintf(url, sizeof(url), "%s/api/coin_slot_sessions/%s/coin_transactions",
             SERVER_URL, event.sessionUid.c_str());
    
    char path[128];
    snprintf(path, sizeof(path), "/api/coin_slot_sessions/%s/coin_transactions",
             event.sessionUid.c_str());
    
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    
    char body[128];
    snprintf(body, sizeof(body),
        "{\"transaction_uid\":\"%s\",\"peso_amount\":%d}",
        event.id.c_str(), event.pulses);
    
    int code;
    if (RegistrationService::isRegistered()) {
        String secret = RegistrationService::getSecret();
        addHmacHeaders(http, secret, "POST", path, body);
        code = http.POST(body);
    } else {
        code = http.POST(body);
    }

    int ok_code = 200;
    int create_code = 201;
    bool success = (code == ok_code || code == create_code);

    // 401 Unauthorized - differentiate between missing device and locked device
    if (code == 401) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload);
        
        String detail = "";
        if (!err && doc["detail"].is<const char*>()) {
            detail = String(doc["detail"].as<const char*>());
            Serial.printf("[RAILS] 401 Unauthorized: %s\n", detail.c_str());
        } else {
            Serial.printf("[RAILS] 401 Unauthorized: %s\n", payload.c_str());
        }
        
        // Check if device is locked (invalid device) or missing
        // Missing device: allow re-registration
        // Locked device: wait for admin unlock
        if (detail.indexOf("locked") != -1 || detail.indexOf("Invalid") != -1 || detail.indexOf("Locked") != -1) {
            // Invalid device - locked by admin, wait for unlock
            Serial.println("[RAILS] Device is locked, waiting for admin unlock");
            RegistrationService::setLocked(true);
        } else {
            // Missing device - allow re-registration
            Serial.println("[RAILS] Device not registered, will re-register");
            RegistrationService::clearSecret();
        }
    }

    // 422 with "transaction_uid has already been taken" means transaction was already recorded - treat as success
    if (code == 422) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload);
        if (!err && doc["errors"]["transaction_uid"].is<JsonArray>()) {
            for (JsonVariant v : doc["errors"]["transaction_uid"].as<JsonArray>()) {
                if (v.as<String>().indexOf("taken") != -1) {
                    success = true;
                    break;
                }
            }
        }
    }

    http.end();
    return success;
}

bool RailsClient::registerCoinSlot(String& outSecret)
{
    WiFiClient client;
    client.setTimeout(15000);
    HTTPClient http;
    http.setTimeout(15000);
    
    char url[256];
    snprintf(url, sizeof(url), "%s/api/coin_slots/register", SERVER_URL);
    
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    http.addHeader("Connection", "close");

    String macAddress = WiFi.macAddress();
    String ipAddress = WiFi.localIP().toString();
    char deviceId[32];
    snprintf(deviceId, sizeof(deviceId), "esp-%u", ESP.getChipId());

    char body[256];
    snprintf(body, sizeof(body),
        "{\"coin_slot\":{\"name\":\"%s\",\"device_id\":\"%s\",\"mac_address\":\"%s\",\"ip_address\":\"%s\"}}",
        DEVICE_NAME, deviceId, macAddress.c_str(), ipAddress.c_str());
    
    yield();

    int code = http.POST(body);

    if (code <= 0) {
        delay(300);
        Serial.printf("[REG] NETWORK ERROR: %s (%d)\n",
                    http.errorToString(code).c_str(),
                    code);

        http.end();
        return false;
    }

    bool success = (code == 200 || code == 201);

    if (success) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload);
        if (!err) {
            // Force deep copy so String survives after doc is destroyed
            outSecret = String(doc["secret"].as<const char*>());
            Serial.printf("[REG] response code=%d secret=%s\n",
                        code, outSecret.c_str());
        } else {
            Serial.printf("[REG] JSON parse error: %s\n", err.c_str());
        }
    } else if (code == 422) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload);
        Serial.printf("[REG] 422 Validation Failed\n");
        if (!err && doc["errors"].is<JsonArray>()) {
            bool allTaken = true;
            for (JsonVariant v : doc["errors"].as<JsonArray>()) {
                const char* field = v["field"] | "unknown";
                const char* errCode = v["code"] | "unknown";
                const char* message = v["message"] | "no message";
                Serial.printf("[REG]   field=\"%s\" code=\"%s\" message=\"%s\"\n",
                            field, errCode, message);
                if (String(errCode) != "TAKEN") {
                    allTaken = false;
                }
            }
            if (allTaken) {
                Serial.println("[REG] device already registered on server");
            }
        } else {
            Serial.printf("[REG]   could not parse errors: %s\n", err.c_str());
            Serial.printf("[REG]   raw payload: %s\n", payload.c_str());
        }
    } else {
        Serial.printf("[REG] failed, code=%d\n", code);
        String payload = http.getString();
        Serial.printf("[REG] response body: %s\n", payload.c_str());
    }

    http.end();
    return success;
}

bool RailsClient::heartbeatCoinSlot(const String& secret, const String& deviceId)
{
    WiFiClient client;
    HTTPClient http;
    
    char url[256];
    snprintf(url, sizeof(url), "%s/api/coin_slots/%s/heartbeat",
             SERVER_URL, deviceId.c_str());
    
    char path[128];
    snprintf(path, sizeof(path), "/api/coin_slots/%s/heartbeat",
             deviceId.c_str());
    
    String macAddress = WiFi.macAddress();
    String ipAddress = WiFi.localIP().toString();
    
    char body[256];
    snprintf(body, sizeof(body),
        "{\"coin_slot\":{\"ip_address\":\"%s\",\"mac_address\":\"%s\"}}",
        ipAddress.c_str(), macAddress.c_str());
    
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    http.addHeader("Connection", "close");
    addHmacHeaders(http, secret, "POST", path, body);

    int code = http.POST(body);
    bool success = (code == 200);

    // 401 Unauthorized - differentiate between missing device and locked device
    if (code == 401) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload);
        
        String detail = "";
        if (!err && doc["detail"].is<const char*>()) {
            detail = String(doc["detail"].as<const char*>());
            Serial.printf("[RAILS] heartbeat 401 Unauthorized: %s\n", detail.c_str());
        } else {
            Serial.printf("[RAILS] heartbeat 401 Unauthorized: %s\n", payload.c_str());
        }
        
        // Check if device is locked (invalid device) or missing
        if (detail.indexOf("locked") != -1 || detail.indexOf("Invalid") != -1 || detail.indexOf("Locked") != -1) {
            // Invalid device - locked by admin, wait for unlock
            Serial.println("[RAILS] Device is locked, waiting for admin unlock");
            RegistrationService::setLocked(true);
        } else {
            // Missing device - allow re-registration
            Serial.println("[RAILS] Device not registered, will re-register");
            RegistrationService::clearSecret();
        }
    }

    if (success) {
        Serial.printf("[HEARTBEAT] ok, code=%d\n", code);
    } else {
        Serial.printf("[HEARTBEAT] failed, code=%d\n", code);
    }

    http.end();
    return success;
}