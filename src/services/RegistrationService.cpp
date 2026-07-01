#include "RegistrationService.h"
#include "HttpTaskQueue.h"
#include "../config/DeviceConfig.h"
#include <LittleFS.h>
#include <ESP8266WiFi.h>

String RegistrationService::secret = "";
bool RegistrationService::registered = false;
bool RegistrationService::locked = false;
int RegistrationService::heartbeatFailCount = 0;

void RegistrationService::begin() {
  load();
  if (registered) {
    Serial.printf("[REG] loaded secret=%.10s, locked=%d\n", secret.c_str(), locked);
  } else {
    Serial.println("[REG] no persisted secret, will register");
  }
}

void RegistrationService::loop() {
  if (WiFi.status() != WL_CONNECTED) return;

  // Only enqueue registration if not locked
  if (!registered && !locked) {
    // Enqueue a REGISTER task (dedup handled by HttpTaskQueue)
    HttpTaskQueue::enqueueRegistration();
  }
}

bool RegistrationService::isRegistered() {
  return registered;
}

bool RegistrationService::isLocked() {
  return locked;
}

String RegistrationService::getSecret() {
  return secret;
}

void RegistrationService::setSecret(const String& newSecret) {
  secret = newSecret;
  registered = true;
  locked = false;  // Clear locked state on successful registration
  heartbeatFailCount = 0;
  save();
  Serial.printf("[REG] secret set and saved: %s\n", secret.c_str());
}

void RegistrationService::setLocked(bool lockedState) {
  locked = lockedState;
  save();
  Serial.printf("[REG] locked state set to: %d\n", locked);
}

void RegistrationService::clearSecret() {
  secret = "";
  registered = false;
  locked = false;  // Also clear locked state for missing device case
  heartbeatFailCount = 0;
  LittleFS.remove("/secret.txt");
  Serial.println("[REG] secret cleared, will re-register");
}

void RegistrationService::save() {
  File file = LittleFS.open("/secret.txt", "w");
  if (!file) return;
  file.println(secret);
  file.println(locked ? "1" : "0");
  file.close();
}

void RegistrationService::load() {
  File file = LittleFS.open("/secret.txt", "r");
  if (!file) return;
  secret = file.readStringUntil('\n');
  secret.trim();
  
  // Load locked state if present
  if (file.available()) {
    String lockedStr = file.readStringUntil('\n');
    lockedStr.trim();
    locked = (lockedStr == "1");
  } else {
    locked = false;
  }
  
  file.close();
  registered = (secret.length() > 0);
}