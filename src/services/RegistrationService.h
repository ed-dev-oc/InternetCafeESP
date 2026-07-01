#pragma once
#include <Arduino.h>

class RegistrationService {
public:
  static void begin();
  static void loop();
  static bool isRegistered();
  static bool isLocked();
  static String getSecret();
  static void setSecret(const String& secret);
  static void setLocked(bool locked);
  static void clearSecret();

private:
  static String secret;
  static bool registered;
  static bool locked;
  static int heartbeatFailCount;
  static void save();
  static void load();
};