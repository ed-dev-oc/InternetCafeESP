#include "DeviceSettings.h"

#include <LittleFS.h>

String DeviceSettings::wifiSsidValue = "";
String DeviceSettings::wifiPasswordValue = "";
String DeviceSettings::deviceNameValue = "";
String DeviceSettings::serverUrlValue = "";
String DeviceSettings::adminPasswordValue = "";
bool DeviceSettings::useStaticIpValue = false;
IPAddress DeviceSettings::localIpValue = IPAddress(192, 168, 1, 254);
IPAddress DeviceSettings::gatewayValue = IPAddress(192, 168, 1, 1);
IPAddress DeviceSettings::subnetValue = IPAddress(255, 255, 255, 0);
IPAddress DeviceSettings::primaryDnsValue = IPAddress(192, 168, 1, 1);
IPAddress DeviceSettings::secondaryDnsValue = IPAddress(1, 1, 1, 1);

void DeviceSettings::begin()
{
    applyDefaults();
    load();

    if (deviceNameValue.isEmpty()) {
        deviceNameValue = defaultDeviceName();
    }
}

bool DeviceSettings::load()
{
    File file = LittleFS.open("/device_settings.json", "r");
    if (!file) {
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();

    if (err) {
        Serial.printf("[CFG] failed to load settings: %s\n", err.c_str());
        return false;
    }

    wifiSsidValue = String((const char*)(doc["wifi_ssid"] | ""));
    wifiPasswordValue = String((const char*)(doc["wifi_password"] | ""));
    deviceNameValue = String((const char*)(doc["device_name"] | ""));
    serverUrlValue = String((const char*)(doc["server_url"] | ""));
    adminPasswordValue = String((const char*)(doc["admin_password"] | ""));
    useStaticIpValue = doc["use_static_ip"] | false;

    String localIp = String((const char*)(doc["local_ip"] | ""));
    String gateway = String((const char*)(doc["gateway"] | ""));
    String subnet = String((const char*)(doc["subnet"] | ""));
    String primaryDns = String((const char*)(doc["primary_dns"] | ""));
    String secondaryDns = String((const char*)(doc["secondary_dns"] | ""));

    if (localIp.length() > 0) {
        parseIp(localIp, localIpValue);
    }
    if (gateway.length() > 0) {
        parseIp(gateway, gatewayValue);
    }
    if (subnet.length() > 0) {
        parseIp(subnet, subnetValue);
    }
    if (primaryDns.length() > 0) {
        parseIp(primaryDns, primaryDnsValue);
    }
    if (secondaryDns.length() > 0) {
        parseIp(secondaryDns, secondaryDnsValue);
    }

    Serial.printf("[CFG] loaded device=%s ssid=%s server=%s static_ip=%d\n",
        deviceNameValue.c_str(),
        wifiSsidValue.c_str(),
        serverUrlValue.c_str(),
        useStaticIpValue);

    return true;
}

bool DeviceSettings::save()
{
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    toJson(root, true);

    File file = LittleFS.open("/device_settings.json", "w");
    if (!file) {
        Serial.println("[CFG] failed to open settings file for write");
        return false;
    }

    if (serializeJsonPretty(doc, file) == 0) {
        file.close();
        Serial.println("[CFG] failed to write settings file");
        return false;
    }

    file.close();
    Serial.printf("[CFG] saved device=%s ssid=%s server=%s static_ip=%d\n",
        deviceNameValue.c_str(),
        wifiSsidValue.c_str(),
        serverUrlValue.c_str(),
        useStaticIpValue);
    return true;
}

void DeviceSettings::resetToDefaults()
{
    applyDefaults();
}

bool DeviceSettings::isConfigured()
{
    return wifiSsidValue.length() > 0
        && wifiPasswordValue.length() > 0
        && serverUrlValue.length() > 0;
}

bool DeviceSettings::hasAdminPassword()
{
    return adminPasswordValue.length() > 0;
}

bool DeviceSettings::adminPasswordMatches(const String& password)
{
    if (!hasAdminPassword()) {
        return true;
    }
    return password == adminPasswordValue;
}

String DeviceSettings::wifiSsid()
{
    return wifiSsidValue;
}

String DeviceSettings::wifiPassword()
{
    return wifiPasswordValue;
}

String DeviceSettings::deviceName()
{
    if (deviceNameValue.length() > 0) {
        return deviceNameValue;
    }
    return defaultDeviceName();
}

String DeviceSettings::serverUrl()
{
    return serverUrlValue;
}

String DeviceSettings::adminPassword()
{
    return adminPasswordValue;
}

bool DeviceSettings::useStaticIp()
{
    return useStaticIpValue;
}

IPAddress DeviceSettings::localIp()
{
    return localIpValue;
}

IPAddress DeviceSettings::gateway()
{
    return gatewayValue;
}

IPAddress DeviceSettings::subnet()
{
    return subnetValue;
}

IPAddress DeviceSettings::primaryDns()
{
    return primaryDnsValue;
}

IPAddress DeviceSettings::secondaryDns()
{
    return secondaryDnsValue;
}

void DeviceSettings::setWifiSsid(const String& value)
{
    wifiSsidValue = value;
}

void DeviceSettings::setWifiPassword(const String& value)
{
    wifiPasswordValue = value;
}

void DeviceSettings::setDeviceName(const String& value)
{
    deviceNameValue = value;
}

void DeviceSettings::setServerUrl(const String& value)
{
    serverUrlValue = value;
}

void DeviceSettings::setAdminPassword(const String& value)
{
    adminPasswordValue = value;
}

void DeviceSettings::setUseStaticIp(bool value)
{
    useStaticIpValue = value;
}

void DeviceSettings::setLocalIp(const IPAddress& value)
{
    localIpValue = value;
}

void DeviceSettings::setGateway(const IPAddress& value)
{
    gatewayValue = value;
}

void DeviceSettings::setSubnet(const IPAddress& value)
{
    subnetValue = value;
}

void DeviceSettings::setPrimaryDns(const IPAddress& value)
{
    primaryDnsValue = value;
}

void DeviceSettings::setSecondaryDns(const IPAddress& value)
{
    secondaryDnsValue = value;
}

String DeviceSettings::setupApSsid()
{
    return String("InternetCafeESP-Setup-") + String(ESP.getChipId(), HEX);
}

void DeviceSettings::toJson(JsonObject object, bool includeSecrets)
{
    object["wifi_ssid"] = wifiSsidValue;
    object["device_name"] = deviceName();
    object["server_url"] = serverUrlValue;
    object["use_static_ip"] = useStaticIpValue;
    object["local_ip"] = localIpValue.toString();
    object["gateway"] = gatewayValue.toString();
    object["subnet"] = subnetValue.toString();
    object["primary_dns"] = primaryDnsValue.toString();
    object["secondary_dns"] = secondaryDnsValue.toString();
    object["admin_password_set"] = hasAdminPassword();
    object["wifi_password_set"] = wifiPasswordValue.length() > 0;
    object["configured"] = isConfigured();
    object["setup_ap_ssid"] = setupApSsid();

    if (includeSecrets) {
        object["wifi_password"] = wifiPasswordValue;
        object["admin_password"] = adminPasswordValue;
    } else {
        object["wifi_password"] = "";
        object["admin_password"] = "";
    }
}

bool DeviceSettings::applyJson(JsonVariantConst payload, bool allowPartial)
{
    if (!payload.is<JsonObjectConst>()) {
        return false;
    }

    JsonObjectConst root = payload.as<JsonObjectConst>();

    if (root.containsKey("wifi_ssid") || !allowPartial) {
        String value = String((const char*)(root["wifi_ssid"] | ""));
        if (value.length() > 0 || !allowPartial) {
            setWifiSsid(value);
        }
    }
    if (root.containsKey("wifi_password") || !allowPartial) {
        String value = String((const char*)(root["wifi_password"] | ""));
        if (value.length() > 0 || !allowPartial) {
            setWifiPassword(value);
        }
    }
    if (root.containsKey("device_name") || !allowPartial) {
        String value = String((const char*)(root["device_name"] | ""));
        if (value.length() > 0 || !allowPartial) {
            setDeviceName(value);
        }
    }
    if (root.containsKey("server_url") || !allowPartial) {
        String value = String((const char*)(root["server_url"] | ""));
        if (value.length() > 0 || !allowPartial) {
            setServerUrl(value);
        }
    }
    if (root.containsKey("admin_password") || !allowPartial) {
        String value = String((const char*)(root["admin_password"] | ""));
        if (value.length() > 0 || !allowPartial) {
            setAdminPassword(value);
        }
    }
    if (root.containsKey("use_static_ip") || !allowPartial) {
        setUseStaticIp(root["use_static_ip"] | false);
    }

    if (root.containsKey("local_ip")) {
        String value = String((const char*)(root["local_ip"] | ""));
        if (value.length() > 0) {
            parseIp(value, localIpValue);
        }
    }
    if (root.containsKey("gateway")) {
        String value = String((const char*)(root["gateway"] | ""));
        if (value.length() > 0) {
            parseIp(value, gatewayValue);
        }
    }
    if (root.containsKey("subnet")) {
        String value = String((const char*)(root["subnet"] | ""));
        if (value.length() > 0) {
            parseIp(value, subnetValue);
        }
    }
    if (root.containsKey("primary_dns")) {
        String value = String((const char*)(root["primary_dns"] | ""));
        if (value.length() > 0) {
            parseIp(value, primaryDnsValue);
        }
    }
    if (root.containsKey("secondary_dns")) {
        String value = String((const char*)(root["secondary_dns"] | ""));
        if (value.length() > 0) {
            parseIp(value, secondaryDnsValue);
        }
    }

    if (deviceNameValue.length() == 0) {
        deviceNameValue = defaultDeviceName();
    }

    return true;
}

void DeviceSettings::applyDefaults()
{
    wifiSsidValue = "";
    wifiPasswordValue = "";
    deviceNameValue = defaultDeviceName();
    serverUrlValue = "";
    adminPasswordValue = "";
    useStaticIpValue = false;
    localIpValue = IPAddress(192, 168, 1, 254);
    gatewayValue = IPAddress(192, 168, 1, 1);
    subnetValue = IPAddress(255, 255, 255, 0);
    primaryDnsValue = IPAddress(192, 168, 1, 1);
    secondaryDnsValue = IPAddress(1, 1, 1, 1);
}

String DeviceSettings::defaultDeviceName()
{
    return String("COIN-SLOT-") + String(ESP.getChipId(), HEX);
}

bool DeviceSettings::parseIp(const String& value, IPAddress& output)
{
    return output.fromString(value);
}
