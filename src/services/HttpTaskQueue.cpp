#include "HttpTaskQueue.h"
#include "RailsClient.h"
#include "../config/DeviceConfig.h"
#include "../config/AppConfig.h"
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <algorithm>

namespace {
const char* QUEUE_PATH = "/http_queue.jsonl";
const char* QUEUE_TMP_PATH = "/http_queue.jsonl.tmp";

bool isJsonLine(const String& line)
{
  return line.length() > 0 && line[0] == '{';
}

bool parseTaskFromJsonLine(const String& line, HttpTask& t)
{
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, line);
  if (err) {
    Serial.printf("[HTQ] skipping invalid json line: %s\n", err.c_str());
    return false;
  }

  String typeStr = String((const char*)(doc["type"] | ""));
  if (typeStr == "REGISTER") {
    t.type = HttpTask::REGISTER;
  } else if (typeStr == "COIN_EVENT") {
    t.type = HttpTask::COIN_EVENT;
  } else if (typeStr == "HEARTBEAT") {
    t.type = HttpTask::HEARTBEAT;
  } else {
    Serial.printf("[HTQ] skipping task with unknown type: %s\n", typeStr.c_str());
    return false;
  }

  t.id = String((const char*)(doc["id"] | ""));
  t.priority = (HttpTask::Priority)((int)(doc["priority"] | (int)HttpTask::priorityFor(t.type)));
  t.payload = String((const char*)(doc["payload"] | ""));
  t.retryCount = (int)(doc["retryCount"] | 0);
  t.createdAt = (unsigned long)(doc["createdAt"] | 0UL);
  t.nextRetryAt = (unsigned long)(doc["nextRetryAt"] | 0UL);
  return true;
}

bool parseTaskFromLegacyCsvLine(const String& line, HttpTask& t)
{
  int p1 = line.indexOf(',');
  int p2 = line.indexOf(',', p1 + 1);
  int p3 = line.indexOf(',', p2 + 1);
  int pLast = line.lastIndexOf(',');
  int pSecondLast = line.lastIndexOf(',', pLast - 1);
  int pThirdLast = line.lastIndexOf(',', pSecondLast - 1);

  if (p1 < 0 || p2 < 0 || p3 < 0 || pThirdLast < 0 || pSecondLast < 0 || pLast < 0) {
    Serial.println("[HTQ] skipping malformed legacy queue line");
    return false;
  }

  t.id = line.substring(0, p1);
  t.type = (HttpTask::Type)line.substring(p1 + 1, p2).toInt();
  t.priority = (HttpTask::Priority)line.substring(p2 + 1, p3).toInt();
  t.payload = line.substring(p3 + 1, pThirdLast);
  t.retryCount = line.substring(pThirdLast + 1, pSecondLast).toInt();
  t.createdAt = strtoul(line.substring(pSecondLast + 1, pLast).c_str(), nullptr, 10);
  t.nextRetryAt = strtoul(line.substring(pLast + 1).c_str(), nullptr, 10);
  return true;
}

void normalizeLoadedTask(HttpTask& task)
{
  // Reboot resets millis(), so the next retry must be recalculated after load.
  task.nextRetryAt = 0;
  task.retryCount = 0;
}
} // namespace

std::vector<HttpTask> HttpTaskQueue::queue;

void HttpTaskQueue::begin() {
  if (CLEAR_QUEUE_ON_BOOT) {
    queue.clear();
    if (save()) {
      Serial.println("[HTQ] queue cleared on boot");
    } else {
      Serial.println("[HTQ] queue clear on boot failed to persist");
    }
  } else {
    load();
    Serial.printf("[HTQ] loaded %d tasks from flash\n", queue.size());
  }
}

bool HttpTaskQueue::enqueue(const HttpTask& task) {
  // Dedup: only one REGISTER or HEARTBEAT in queue at a time
  if (task.type == HttpTask::REGISTER && hasType(HttpTask::REGISTER)) {
    Serial.println("[HTQ] REGISTER already queued, skipping");
    return true;
  }
  if (task.type == HttpTask::HEARTBEAT && hasType(HttpTask::HEARTBEAT)) {
    Serial.println("[HTQ] HEARTBEAT already queued, skipping");
    return true;
  }

  queue.push_back(task);
  sortQueue();
  bool saved = save();
  Serial.printf("[HTQ] enqueued %s (priority=%d, retry=%d)\n",
    HttpTask::typeStr(task.type), task.priority, task.retryCount);
  return saved;
}

bool HttpTaskQueue::hasReadyTask() {
  if (queue.empty()) return false;
  return (millis() >= queue.front().nextRetryAt);
}

HttpTask HttpTaskQueue::peek() {
  return queue.front();
}

void HttpTaskQueue::removeFirst() {
  if (!queue.empty()) {
    queue.erase(queue.begin());
    if (!save()) {
      Serial.println("[HTQ] failed to persist queue after removing head");
    }
  }
}

void HttpTaskQueue::requeueFront(unsigned long backoffMs) {
  if (queue.empty()) return;
  HttpTask task = queue.front();
  queue.erase(queue.begin());

  task.retryCount++;
  task.nextRetryAt = millis() + backoffMs;

  queue.push_back(task);
  sortQueue();
  if (!save()) {
    Serial.println("[HTQ] failed to persist queue after requeue");
  }
  Serial.printf("[HTQ] requeued %s (retry=%d, backoff=%lums)\n",
    HttpTask::typeStr(task.type), task.retryCount, backoffMs);
}

int HttpTaskQueue::size() {
  return queue.size();
}

// --- Convenience enqueuers ---

bool HttpTaskQueue::enqueueRegistration() {
  HttpTask task;
  task.id = generateId();
  task.type = HttpTask::REGISTER;
  task.priority = HttpTask::priorityFor(HttpTask::REGISTER);
  task.payload = "";
  task.retryCount = 0;
  task.createdAt = millis();
  task.nextRetryAt = 0;
  return enqueue(task);
}

bool HttpTaskQueue::enqueueCoinEvent(const CoinEvent& event) {
  HttpTask task;
  task.id = event.id;
  task.type = HttpTask::COIN_EVENT;
  task.priority = HttpTask::priorityFor(HttpTask::COIN_EVENT);
  
  char payload[128];
  snprintf(payload, sizeof(payload),
      "{\"mac\":\"%s\",\"session_uid\":\"%s\",\"pulses\":%d}",
      event.macAddress.c_str(), event.sessionUid.c_str(), event.pulses);
  task.payload = payload;
  
  task.retryCount = 0;
  task.createdAt = event.createdAt;
  task.nextRetryAt = 0;
  return enqueue(task);
}

bool HttpTaskQueue::enqueueHeartbeat() {
  HttpTask task;
  task.id = generateId();
  task.type = HttpTask::HEARTBEAT;
  task.priority = HttpTask::priorityFor(HttpTask::HEARTBEAT);
  task.payload = "";
  task.retryCount = 0;
  task.createdAt = millis();
  task.nextRetryAt = 0;
  return enqueue(task);
}

void HttpTaskQueue::clear() {
  queue.clear();
  if (!save()) {
    Serial.println("[HTQ] failed to persist queue clear");
  }
  Serial.println("[HTQ] queue cleared");
}

const std::vector<HttpTask>& HttpTaskQueue::getAllTasks() {
  return queue;
}

// --- Private helpers ---

void HttpTaskQueue::sortQueue() {
  std::sort(queue.begin(), queue.end(), [](const HttpTask& a, const HttpTask& b) {
    if (a.priority != b.priority) return a.priority < b.priority;
    return a.nextRetryAt < b.nextRetryAt;
  });
}

bool HttpTaskQueue::save() {
  File file = LittleFS.open(QUEUE_TMP_PATH, "w");
  if (!file) {
    Serial.println("[HTQ] failed to open queue temp file for write");
    return false;
  }

  for (const auto& t : queue) {
    JsonDocument doc;
    doc["id"] = t.id;
    doc["type"] = HttpTask::typeStr(t.type);
    doc["priority"] = (int)t.priority;
    doc["payload"] = t.payload;
    doc["retryCount"] = t.retryCount;
    doc["createdAt"] = t.createdAt;
    doc["nextRetryAt"] = t.nextRetryAt;

    if (serializeJson(doc, file) == 0 || file.write('\n') != 1) {
      Serial.println("[HTQ] failed while writing queue file");
      file.close();
      LittleFS.remove(QUEUE_TMP_PATH);
      return false;
    }
  }

  file.flush();
  file.close();

  LittleFS.remove(QUEUE_PATH);
  if (!LittleFS.rename(QUEUE_TMP_PATH, QUEUE_PATH)) {
    Serial.println("[HTQ] failed to replace queue file atomically");
    LittleFS.remove(QUEUE_TMP_PATH);
    return false;
  }

  return true;
}

bool HttpTaskQueue::load() {
  queue.clear();
  File file = LittleFS.open(QUEUE_PATH, "r");
  if (!file) {
    File legacy = LittleFS.open("/http_queue.csv", "r");
    if (!legacy) {
      return false;
    }

    while (legacy.available()) {
      String line = legacy.readStringUntil('\n');
      line.trim();
      if (line.length() == 0) continue;

      HttpTask t;
      if (!parseTaskFromLegacyCsvLine(line, t)) {
        continue;
      }
      normalizeLoadedTask(t);
      queue.push_back(t);
    }
    legacy.close();
    sortQueue();
    if (!queue.empty()) {
      save();
    }
    return true;
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    HttpTask t;
    if (isJsonLine(line)) {
      if (!parseTaskFromJsonLine(line, t)) {
        continue;
      }
    } else {
      if (!parseTaskFromLegacyCsvLine(line, t)) {
        continue;
      }
    }

    normalizeLoadedTask(t);
    queue.push_back(t);
  }
  file.close();

  sortQueue();
  return true;
}

String HttpTaskQueue::generateId() {
  char id[32];
  snprintf(id, sizeof(id), "%u-%lu", ESP.getChipId(), millis());
  return String(id);
}

bool HttpTaskQueue::hasType(HttpTask::Type type) {
  for (auto& t : queue) {
    if (t.type == type) return true;
  }
  return false;
}
