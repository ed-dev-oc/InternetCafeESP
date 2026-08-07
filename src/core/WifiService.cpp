#include "WifiService.h"

#include "../config/DeviceSettings.h"
#include "../config/WifiConfig.h"

bool WifiService::setupMode = false;
unsigned long WifiService::lastReconnectAttempt = 0;

bool WifiService::connect(unsigned long timeoutMs)
{
    setupMode = false;
    lastReconnectAttempt = 0;

    if (!DeviceSettings::isConfigured()) {
        Serial.println("[WIFI] configuration missing, starting setup AP");
        startSetupAccessPoint();
        return false;
    }

    if (connectStation(timeoutMs)) {
        return true;
    }

    Serial.println("[WIFI] station connect failed, starting setup AP");
    startSetupAccessPoint();
    return false;
}

void WifiService::loop()
{
    if (setupMode) {
        return;
    }

    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    if (!DeviceSettings::isConfigured()) {
        return;
    }

    if (millis() - lastReconnectAttempt < 30000) {
        return;
    }

    lastReconnectAttempt = millis();
    Serial.println("[WIFI] attempting background reconnect");
    connectStation(10000);
}

bool WifiService::isSetupMode()
{
    return setupMode;
}

bool WifiService::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

bool WifiService::connectStation(unsigned long timeoutMs)
{
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.disconnect();
    delay(100);

    setHostname();

    if (DeviceSettings::useStaticIp()) {
        applyStaticIp();
    }

    String ssid = DeviceSettings::wifiSsid();
    String password = DeviceSettings::wifiPassword();

    Serial.printf("[WIFI] connecting to %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startedAt = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startedAt) < timeoutMs) {
        delay(250);
        yield();
        Serial.print('.');
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println();
        Serial.println("[WIFI] connect timeout");
        return false;
    }

    Serial.println();
    Serial.println("[WIFI] connected");
    Serial.print("[WIFI] IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("[WIFI] MAC: ");
    Serial.println(WiFi.macAddress());
    return true;
}

void WifiService::startSetupAccessPoint()
{
    WiFi.mode(WIFI_AP_STA);
    WiFi.persistent(false);
    WiFi.disconnect();
    WiFi.softAPdisconnect(true);
    delay(100);

    WiFi.softAPConfig(SETUP_AP_IP, SETUP_AP_GATEWAY, SETUP_AP_SUBNET);

    String ssid = DeviceSettings::setupApSsid();
    bool ok = WiFi.softAP(ssid.c_str());
    setupMode = true;

    Serial.printf("[WIFI] setup AP %s (%s)\n", ok ? "started" : "failed", ssid.c_str());
    Serial.print("[WIFI] AP IP: ");
    Serial.println(WiFi.softAPIP());
}

void WifiService::applyStaticIp()
{
    bool ok = WiFi.config(
        DeviceSettings::localIp(),
        DeviceSettings::gateway(),
        DeviceSettings::subnet(),
        DeviceSettings::primaryDns(),
        DeviceSettings::secondaryDns()
    );

    Serial.printf("[WIFI] static IP %s\n", ok ? "applied" : "failed");
}

void WifiService::setHostname()
{
    String hostname = DeviceSettings::deviceName();
    if (hostname.length() == 0) {
        hostname = String("esp-") + String(ESP.getChipId());
    }
    WiFi.hostname(hostname.c_str());
}
