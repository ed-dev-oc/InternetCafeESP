#include <Arduino.h>
#include "RailsClient.h"
#include "RegistrationService.h"
#include "TimeService.h"
#include "../core/HmacHelper.h"
#include "../config/DeviceSettings.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

static String buildServerUrl(const String& path)
{
    String serverUrl = DeviceSettings::serverUrl();
    if (serverUrl.length() == 0) {
        return "";
    }
    return serverUrl + path;
}

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
    String path = String("/api/coin_slot_sessions/") + event.sessionUid + "/coin_transactions";
    String url = buildServerUrl(path);
    if (url.length() == 0) {
        Serial.println("[RAILS] server URL missing, skipping coin event");
        return false;
    }

    WiFiClient client;
    HTTPClient http;
    
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
        
        if (detail.indexOf("locked") != -1 || detail.indexOf("Invalid") != -1 || detail.indexOf("Locked") != -1) {
            Serial.println("[RAILS] Device is locked, waiting for admin unlock");
            RegistrationService::setLocked(true);
        } else {
            Serial.println("[RAILS] Device not registered, will re-register");
            RegistrationService::clearSecret();
        }
    }

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
    String serverUrl = DeviceSettings::serverUrl();
    if (serverUrl.length() == 0) {
        Serial.println("[REG] server URL missing, cannot register");
        return false;
    }

    WiFiClient client;
    client.setTimeout(15000);
    HTTPClient http;
    http.setTimeout(15000);
    
    String url = serverUrl + "/api/coin_slots/register";
    
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    http.addHeader("Connection", "close");

    String macAddress = WiFi.macAddress();
    String ipAddress = WiFi.localIP().toString();
    String deviceName = DeviceSettings::deviceName();
    char deviceId[32];
    snprintf(deviceId, sizeof(deviceId), "esp-%u", ESP.getChipId());

    char body[256];
    snprintf(body, sizeof(body),
        "{\"coin_slot\":{\"name\":\"%s\",\"device_id\":\"%s\",\"mac_address\":\"%s\",\"ip_address\":\"%s\"}}",
        deviceName.c_str(), deviceId, macAddress.c_str(), ipAddress.c_str());
    
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
    String serverUrl = DeviceSettings::serverUrl();
    if (serverUrl.length() == 0) {
        Serial.println("[HEARTBEAT] server URL missing, skipping");
        return false;
    }

    WiFiClient client;
    HTTPClient http;
    
    String path = "/api/coin_slots/" + deviceId + "/heartbeat";
    String url = buildServerUrl(path);
    
    String macAddress = WiFi.macAddress();
    String ipAddress = WiFi.localIP().toString();
    String deviceName = DeviceSettings::deviceName();
    
    char body[256];
    snprintf(body, sizeof(body),
        "{\"coin_slot\":{\"name\":\"%s\",\"ip_address\":\"%s\",\"mac_address\":\"%s\"}}",
        deviceName.c_str(), ipAddress.c_str(), macAddress.c_str());
    
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    http.addHeader("Connection", "close");
    addHmacHeaders(http, secret, "POST", path, body);

    int code = http.POST(body);
    bool success = (code == 200);

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
        
        if (detail.indexOf("locked") != -1 || detail.indexOf("Invalid") != -1 || detail.indexOf("Locked") != -1) {
            Serial.println("[RAILS] Device is locked, waiting for admin unlock");
            RegistrationService::setLocked(true);
        } else {
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
