#include "SenderService.h"
#include "HttpTaskQueue.h"
#include "RailsClient.h"
#include "RegistrationService.h"
#include "TimeService.h"
#include "../models/HttpTask.h"
#include "../models/CoinEvent.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

unsigned long lastSend = 0;

unsigned long SenderService::calcBackoff(int retryCount) {
  unsigned long backoff = 5000UL * (1UL << min(retryCount, 6));
  if (backoff > 60000UL) backoff = 60000UL;
  return backoff;
}

void SenderService::loop() {
  if (millis() - lastSend < 1000) return;
  lastSend = millis();

  if (!HttpTaskQueue::hasReadyTask()) return;

  HttpTask task = HttpTaskQueue::peek();
  bool ok = false;

  // Guard: skip HMAC-requiring tasks if time not synced
  bool needsHmac = (task.type == HttpTask::COIN_EVENT && RegistrationService::isRegistered());
  if (needsHmac && !TimeService::isReady()) {
    Serial.println("[SYNC] time not synced, skipping HMAC task");
    HttpTaskQueue::requeueFront(5000);
    return;
  }

  switch (task.type) {
    case HttpTask::REGISTER: {
      String outSecret;
      ok = RailsClient::registerCoinSlot(outSecret);
      if (ok && outSecret.length() > 0) {
        RegistrationService::setSecret(outSecret);
        Serial.printf("[SYNC] REGISTER success, secret saved\n");
      } else {
        ok = false;
      }
      break;
    }
    case HttpTask::COIN_EVENT: {
      CoinEvent event;
      event.id = task.id;
      StaticJsonDocument<256> doc;
      deserializeJson(doc, task.payload);
      event.macAddress = doc["mac"].as<String>();
      event.sessionUid = doc["session_uid"].as<String>();
      event.pulses = doc["pulses"].as<int>();
      event.createdAt = task.createdAt;
      ok = RailsClient::sendCoinEvent(event);
      break;
    }
    case HttpTask::HEARTBEAT: {
      String secret = RegistrationService::getSecret();
      ok = RailsClient::heartbeatCoinSlot(secret, "esp-" + String(ESP.getChipId()));
      break;
    }
  }

  if (ok) {
    Serial.printf("[SYNC] %s success\n", HttpTask::typeStr(task.type));
    HttpTaskQueue::removeFirst();
  } else {
    unsigned long backoff = calcBackoff(task.retryCount);
    Serial.printf("[SYNC] %s failed, requeue with %lums backoff\n",
      HttpTask::typeStr(task.type), backoff);
    HttpTaskQueue::requeueFront(backoff);
  }
}