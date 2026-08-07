#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

class DeviceSettings
{
public:
    static void begin();
    static bool load();
    static bool save();
    static void resetToDefaults();

    static bool isConfigured();
    static bool hasAdminPassword();
    static bool adminPasswordMatches(const String& password);

    static String wifiSsid();
    static String wifiPassword();
    static String deviceName();
    static String serverUrl();
    static String adminPassword();

    static bool useStaticIp();
    static IPAddress localIp();
    static IPAddress gateway();
    static IPAddress subnet();
    static IPAddress primaryDns();
    static IPAddress secondaryDns();

    static void setWifiSsid(const String& value);
    static void setWifiPassword(const String& value);
    static void setDeviceName(const String& value);
    static void setServerUrl(const String& value);
    static void setAdminPassword(const String& value);
    static void setUseStaticIp(bool value);
    static void setLocalIp(const IPAddress& value);
    static void setGateway(const IPAddress& value);
    static void setSubnet(const IPAddress& value);
    static void setPrimaryDns(const IPAddress& value);
    static void setSecondaryDns(const IPAddress& value);

    static String setupApSsid();

    static void toJson(JsonObject object, bool includeSecrets = false);
    static bool applyJson(JsonVariantConst payload, bool allowPartial = true);

private:
    static void applyDefaults();
    static String defaultDeviceName();
    static bool parseIp(const String& value, IPAddress& output);

    static String wifiSsidValue;
    static String wifiPasswordValue;
    static String deviceNameValue;
    static String serverUrlValue;
    static String adminPasswordValue;
    static bool useStaticIpValue;
    static IPAddress localIpValue;
    static IPAddress gatewayValue;
    static IPAddress subnetValue;
    static IPAddress primaryDnsValue;
    static IPAddress secondaryDnsValue;
};
