#include "ConfigController.h"

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>

#include "../core/HmacHelper.h"
#include "../config/DeviceSettings.h"
#include "../core/WifiService.h"
#include "../services/HttpTaskQueue.h"
#include "../services/RegistrationService.h"
#include "../services/SessionContext.h"

namespace {
String htmlEscape(const String& value)
{
    String out;
    out.reserve(value.length() + 16);
    for (size_t i = 0; i < value.length(); ++i) {
        char c = value[i];
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += c; break;
        }
    }
    return out;
}

String requestPlainBody(ESP8266WebServer& server)
{
    if (!server.hasArg("plain")) {
        return "";
    }
    String body = server.arg("plain");
    body.trim();
    return body;
}

String fieldFromRequest(ESP8266WebServer& server, JsonDocument* doc, const char* key)
{
    if (doc != nullptr) {
        JsonVariant value = (*doc)[key];
        if (!value.isNull()) {
            if (value.is<const char*>()) {
                const char* text = value.as<const char*>();
                return text ? String(text) : "";
            }
            if (value.is<bool>()) {
                return value.as<bool>() ? "true" : "false";
            }
            if (value.is<long>()) {
                return String(value.as<long>());
            }
        }
        return "";
    }

    if (server.hasArg(key)) {
        return server.arg(key);
    }

    return "";
}

bool boolFromRequest(ESP8266WebServer& server, JsonDocument* doc, const char* key)
{
    if (doc != nullptr) {
        JsonVariant value = (*doc)[key];
        if (!value.isNull()) {
            if (value.is<bool>()) {
                return value.as<bool>();
            }
            if (value.is<const char*>()) {
                String text = String(value.as<const char*>());
                text.toLowerCase();
                return text == "on" || text == "1" || text == "true";
            }
            if (value.is<long>()) {
                return value.as<long>() != 0;
            }
        }
        return false;
    }

    if (server.hasArg(key)) {
        String value = server.arg(key);
        value.toLowerCase();
        return value == "on" || value == "1" || value == "true";
    }

    return false;
}

void applyConfigValues(ESP8266WebServer& server, JsonDocument* doc)
{
    String value;

    value = fieldFromRequest(server, doc, "wifi_ssid");
    if (value.length() > 0) {
        DeviceSettings::setWifiSsid(value);
    }

    value = fieldFromRequest(server, doc, "wifi_password");
    if (value.length() > 0) {
        DeviceSettings::setWifiPassword(value);
    }

    value = fieldFromRequest(server, doc, "device_name");
    if (value.length() > 0) {
        DeviceSettings::setDeviceName(value);
    }

    value = fieldFromRequest(server, doc, "server_url");
    if (value.length() > 0) {
        DeviceSettings::setServerUrl(value);
    }

    value = fieldFromRequest(server, doc, "admin_password");
    if (value.length() > 0) {
        DeviceSettings::setAdminPassword(value);
    }

    if (doc != nullptr) {
        if (!(*doc)["use_static_ip"].isNull()) {
            DeviceSettings::setUseStaticIp(boolFromRequest(server, doc, "use_static_ip"));
        }
    } else {
        DeviceSettings::setUseStaticIp(server.hasArg("use_static_ip"));
    }

    value = fieldFromRequest(server, doc, "local_ip");
    if (value.length() > 0) {
        IPAddress address;
        if (address.fromString(value)) {
            DeviceSettings::setLocalIp(address);
        }
    }

    value = fieldFromRequest(server, doc, "gateway");
    if (value.length() > 0) {
        IPAddress address;
        if (address.fromString(value)) {
            DeviceSettings::setGateway(address);
        }
    }

    value = fieldFromRequest(server, doc, "subnet");
    if (value.length() > 0) {
        IPAddress address;
        if (address.fromString(value)) {
            DeviceSettings::setSubnet(address);
        }
    }

    value = fieldFromRequest(server, doc, "primary_dns");
    if (value.length() > 0) {
        IPAddress address;
        if (address.fromString(value)) {
            DeviceSettings::setPrimaryDns(address);
        }
    }

    value = fieldFromRequest(server, doc, "secondary_dns");
    if (value.length() > 0) {
        IPAddress address;
        if (address.fromString(value)) {
            DeviceSettings::setSecondaryDns(address);
        }
    }

    if (DeviceSettings::deviceName().length() == 0) {
        DeviceSettings::setDeviceName(String("COIN-SLOT-") + String(ESP.getChipId(), HEX));
    }
}

String renderInput(const char* label, const char* name, const String& value, const char* type = "text", const char* placeholder = "")
{
    String html;
    html.reserve(320);
    html += "<label>";
    html += label;
    html += "<input name='";
    html += name;
    html += "' type='";
    html += type;
    html += "' value='";
    html += htmlEscape(value);
    html += "' placeholder='";
    html += placeholder;
    html += "'></label>";
    return html;
}

String renderIpInput(const char* label, const char* name, const IPAddress& value)
{
    return renderInput(label, name, value.toString(), "text", "0.0.0.0");
}

String renderPasswordRow(const char* label, const char* name, const char* placeholder, const char* checkboxLabel)
{
    String html;
    html.reserve(640);
    html += "<div class='field-row password-row'>";
    html += "<div class='field-input'>";
    html += "<label class='field-label' for='";
    html += name;
    html += "'>";
    html += label;
    html += "</label>";
    html += "<input id='";
    html += name;
    html += "' class='password-field' name='";
    html += name;
    html += "' type='password' value='' placeholder='";
    html += htmlEscape(placeholder);
    html += "'></div>";
    html += "<label class='show-toggle'><input id='";
    html += name;
    html += "_show' type='checkbox' onchange='togglePasswordVisibility(";
    html += "\"";
    html += name;
    html += "\"";
    html += ", this)'>";
    html += checkboxLabel;
    html += "</label>";
    html += "</div>";
    return html;
}

String renderStatusPage(const char* title, const char* message, const char* linkLabel, const char* linkHref, bool isError)
{
    String html;
    html.reserve(4096);
    html += "<!doctype html><html><head><meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>ESP Configuration</title>";
    html += "<style>body{font-family:Arial,sans-serif;max-width:760px;margin:0 auto;padding:24px;background:#f4f7fb;color:#132238}.card{background:#fff;padding:24px;border-radius:16px;box-shadow:0 8px 24px rgba(0,0,0,.08)}h1{margin:0 0 12px}.error{color:#b42318}.ok{color:#027a48}a.btn{display:inline-block;margin-top:18px;padding:12px 18px;border-radius:10px;background:#175cd3;color:#fff;text-decoration:none;font-weight:700}</style></head><body><div class='card'>";
    html += "<h1>";
    html += htmlEscape(title);
    html += "</h1><p class='";
    html += isError ? "error" : "ok";
    html += "'>";
    html += htmlEscape(message);
    html += "</p>";
    html += "<a class='btn' href='";
    html += htmlEscape(linkHref);
    html += "'>";
    html += htmlEscape(linkLabel);
    html += "</a></div></body></html>";
    return html;
}

String renderResetSection()
{
    String html;
    html.reserve(4096);
    html += "<section class='danger-card'>";
    html += "<h2>Factory reset</h2>";
    html += "<p>This will erase the saved Wi-Fi, device, registration, session, and queue data, then reboot into setup mode.</p>";
    html += "<p><strong>Type RESET to confirm.</strong> This action cannot be undone.</p>";
    html += "<form method='post' action='/factory-reset'>";
    html += renderPasswordRow("Admin password", "reset_access_password", "required to factory reset", "Show");
    html += renderInput("Confirmation", "confirm_reset", "", "text", "RESET");
    html += "<button class='danger-button' type='submit'>Erase and reboot</button>";
    html += "</form></section>";
    return html;
}

String renderPage()
{
    String html;
    html.reserve(14000);
    html += "<!doctype html><html><head><meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>ESP Configuration</title>";
    html += R"HTML(
<style>
body{font-family:Arial,sans-serif;max-width:900px;margin:0 auto;padding:24px;background:#f4f7fb;color:#132238}
h1{margin:0 0 12px}
p,small{line-height:1.5}
form,.danger-card{background:#fff;padding:20px;border-radius:16px;box-shadow:0 8px 24px rgba(0,0,0,.08)}
.danger-card{margin-top:20px;border:1px solid #ffd5d2;background:linear-gradient(180deg,#fff7f6 0%,#fff 100%)}
h2{margin:0 0 10px;font-size:1.15rem}
label{display:block;margin:12px 0;font-weight:600}
input{width:100%;box-sizing:border-box;padding:10px 12px;margin-top:6px;border:1px solid #cfd7e3;border-radius:10px;font-size:16px}
input[type=checkbox]{width:auto;margin:0}
.field-row{display:flex;align-items:flex-end;gap:16px;margin:12px 0}
.field-row .field-input{flex:1;margin:0}
.field-row .field-input input{margin-top:6px}
.show-toggle{display:flex;align-items:center;gap:8px;margin:0px;white-space:nowrap;padding:10px 12px;border:1px solid #d7e3ff;border-radius:10px;background:#eef4ff;font-size:14px;font-weight:700;justify-content:flex-start}
.field-row.password-row .show-toggle{align-self:flex-end}
button{margin-top:18px;padding:12px 18px;border:0;border-radius:10px;background:#175cd3;color:#fff;font-size:16px;font-weight:700;cursor:pointer}
button.danger-button{background:#b42318}
button.danger-button:hover{background:#962016}
code{background:#e8eef7;padding:2px 6px;border-radius:6px}
</style>
<script>
function togglePasswordVisibility(fieldId, checkbox) {
  const field = document.getElementById(fieldId);
  if (field) {
    field.type = checkbox.checked ? 'text' : 'password';
  }
}
</script>
)HTML";
    html += "</head><body>";
    html += "<h1>ESP Configuration</h1>";
    html += "<p>Setup mode: <strong>";
    html += (WifiService::isSetupMode() ? "yes" : "no");
    html += "</strong></p>";
    html += "<p>Device: <strong>" + htmlEscape(DeviceSettings::deviceName()) + "</strong></p>";
    html += "<p>Server: <code>" + htmlEscape(DeviceSettings::serverUrl()) + "</code></p>";
    html += "<p>Wi-Fi status: <strong>";
    html += (WiFi.status() == WL_CONNECTED ? "connected" : "not connected");
    html += "</strong></p>";
    html += "<p>Save changes below. Leave password fields blank to keep their current value.</p>";
    html += "<form method='post' action='/config'>";
    html += renderInput("Device name", "device_name", DeviceSettings::deviceName());
    html += renderInput("Server URL", "server_url", DeviceSettings::serverUrl(), "url", "http://192.168.1.10:3000");
    html += renderInput("Wi-Fi SSID", "wifi_ssid", DeviceSettings::wifiSsid());
    html += renderPasswordRow("Wi-Fi password", "wifi_password", "leave blank to keep current", "Show");
    html += "<label><input name='use_static_ip' type='checkbox' value='1'";
    if (DeviceSettings::useStaticIp()) {
        html += " checked";
    }
    html += ">Use static IP</label>";
    html += renderIpInput("Static IP", "local_ip", DeviceSettings::localIp());
    html += renderIpInput("Gateway", "gateway", DeviceSettings::gateway());
    html += renderIpInput("Subnet", "subnet", DeviceSettings::subnet());
    html += renderIpInput("Primary DNS", "primary_dns", DeviceSettings::primaryDns());
    html += renderIpInput("Secondary DNS", "secondary_dns", DeviceSettings::secondaryDns());
    html += renderPasswordRow("Current admin password", "access_password", "required if one is already set", "Show");
    html += renderPasswordRow("New admin password", "admin_password", "optional", "Show");
    html += "<button type='submit'>Save and reboot</button>";
    html += "</form>";
    html += renderResetSection();
    html += "<p><small>Setup AP SSID: <code>" + htmlEscape(DeviceSettings::setupApSsid()) + "</code></small></p>";
    html += "</body></html>";
    return html;
}

String jsonStatus(bool includeSecrets)
{
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    DeviceSettings::toJson(root, includeSecrets);
    root["wifi_connected"] = WiFi.status() == WL_CONNECTED;
    root["setup_mode"] = WifiService::isSetupMode();
    root["ip_address"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    root["mac_address"] = WiFi.macAddress();

    String out;
    serializeJsonPretty(doc, out);
    return out;
}

bool saveDeviceSettings()
{
    return DeviceSettings::save();
}

bool removeIfPresent(const char* path)
{
    if (!LittleFS.exists(path)) {
        return true;
    }

    if (!LittleFS.remove(path)) {
        Serial.printf("[RESET] failed to remove %s\n", path);
        return false;
    }

    Serial.printf("[RESET] removed %s\n", path);
    return true;
}

void saveAndReplyHtml(ESP8266WebServer& server, const char* message)
{
    if (!saveDeviceSettings()) {
        server.send(500, "text/html", renderStatusPage(
            "Save failed",
            "The ESP could not save the new configuration. Please go back and try again.",
            "Back to config",
            "/config",
            true
        ));
        return;
    }

    server.send(200, "text/html", renderStatusPage(
        "Settings saved",
        message,
        "Return to config",
        "/config",
        false
    ));
    delay(1000);
    ESP.restart();
}

void saveAndReplyJson(ESP8266WebServer& server, const char* message)
{
    if (!saveDeviceSettings()) {
        server.send(500, "application/json", R"({"status":"error","message":"failed to save settings"})");
        return;
    }

    String json = String("{\"status\":\"ok\",\"message\":\"") + message + "\",\"rebooting\":true}";
    server.send(200, "application/json", json);
    delay(1000);
    ESP.restart();
}

bool wipeFactoryResetState()
{
    bool ok = true;

    DeviceSettings::resetToDefaults();
    SessionContext::clear();
    HttpTaskQueue::clear();
    RegistrationService::clearSecret();

    WiFi.disconnect(true);
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);

    ok &= removeIfPresent("/device_settings.json");
    ok &= removeIfPresent("/secret.txt");
    ok &= removeIfPresent("/http_queue.jsonl");
    ok &= removeIfPresent("/http_queue.jsonl.tmp");
    ok &= removeIfPresent("/http_queue.csv");
    ok &= removeIfPresent("/session.txt");

    return ok;
}

void sendResetReply(ESP8266WebServer& server)
{
    server.send(200, "text/html", renderStatusPage(
        "Factory reset complete",
        "All stored device data was erased. The ESP is rebooting into setup mode now.",
        "Return to config",
        "/config",
        false
    ));
    delay(1000);
    ESP.restart();
}
}

void ConfigController::handleIndex(ESP8266WebServer& server)
{
    server.send(200, "text/html", renderPage());
}

void ConfigController::handleLocalSave(ESP8266WebServer& server)
{
    String accessPassword = server.arg("reset_access_password");
    if (!DeviceSettings::adminPasswordMatches(accessPassword)) {
        server.send(401, "text/html", renderStatusPage(
            "Unauthorized",
            "The current admin password is incorrect.",
            "Back to config",
            "/config",
            true
        ));
        return;
    }

    applyConfigValues(server, nullptr);
    saveAndReplyHtml(server, "Your settings were saved successfully. The ESP is rebooting now.");
}

void ConfigController::handleLocalReset(ESP8266WebServer& server)
{
    String accessPassword = server.arg("reset_access_password");
    if (!DeviceSettings::adminPasswordMatches(accessPassword)) {
        server.send(401, "text/html", renderStatusPage(
            "Unauthorized",
            "The current admin password is incorrect.",
            "Back to config",
            "/config",
            true
        ));
        return;
    }

    String confirmation = server.arg("confirm_reset");
    confirmation.trim();
    confirmation.toUpperCase();
    if (confirmation != "RESET") {
        server.send(400, "text/html", renderStatusPage(
            "Confirmation required",
            "Type RESET in the confirmation field before factory resetting the device.",
            "Back to config",
            "/config",
            true
        ));
        return;
    }

    if (!wipeFactoryResetState()) {
        server.send(500, "text/html", renderStatusPage(
            "Reset failed",
            "The ESP could not remove every stored file. Please try again.",
            "Back to config",
            "/config",
            true
        ));
        return;
    }

    sendResetReply(server);
}

void ConfigController::handleRemoteGet(ESP8266WebServer& server)
{
    if (!HmacHelper::verifyRequest(server, "")) {
        return HmacHelper::sendUnauthorized(server);
    }

    server.send(200, "application/json", jsonStatus(false));
}

void ConfigController::handleRemoteSave(ESP8266WebServer& server)
{
    String body = requestPlainBody(server);
    if (!HmacHelper::verifyRequest(server, body)) {
        return HmacHelper::sendUnauthorized(server);
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        server.send(400, "application/json", R"({"status":"error","message":"invalid json"})");
        return;
    }

    applyConfigValues(server, &doc);
    saveAndReplyJson(server, "remote settings saved");
}
