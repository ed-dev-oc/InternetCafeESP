#include "HttpTaskQueue.h"
#include "RailsClient.h"
#include "../config/DeviceConfig.h"
#include "../config/AppConfig.h"
#include <LittleFS.h>
#include <ESP8266WiFi.h>

std::vector<HttpTask> HttpTaskQueue::queue;

void HttpTaskQueue::begin() {
  if (CLEAR_QUEUE_ON_BOOT) {
    queue.clear();
    save();
    Serial.println("[HTQ] queue cleared on boot");
  } else {
    load();
    Serial.printf("[HTQ] loaded %d tasks from flash\n", queue.size());
  }
  // load();
  // Serial.printf("[HTQ] loaded %d tasks from flash\n", queue.size());
}

void HttpTaskQueue::enqueue(const HttpTask& task) {
  // Dedup: only one REGISTER or HEARTBEAT in queue at a time
  if (task.type == HttpTask::REGISTER && hasType(HttpTask::REGISTER)) {
    Serial.println("[HTQ] REGISTER already queued, skipping");
    return;
  }
  if (task.type == HttpTask::HEARTBEAT && hasType(HttpTask::HEARTBEAT)) {
    Serial.println("[HTQ] HEARTBEAT already queued, skipping");
    return;
  }

  queue.push_back(task);
  sortQueue();
  save();
  Serial.printf("[HTQ] enqueued %s (priority=%d, retry=%d)\n",
    HttpTask::typeStr(task.type), task.priority, task.retryCount);
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
    save();
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
  save();
  Serial.printf("[HTQ] requeued %s (retry=%d, backoff=%lums)\n",
    HttpTask::typeStr(task.type), task.retryCount, backoffMs);
}

int HttpTaskQueue::size() {
  return queue.size();
}

// --- Convenience enqueuers ---

void HttpTaskQueue::enqueueRegistration() {
  HttpTask task;
  task.id = generateId();
  task.type = HttpTask::REGISTER;
  task.priority = HttpTask::priorityFor(HttpTask::REGISTER);
  task.payload = "";
  task.retryCount = 0;
  task.createdAt = millis();
  task.nextRetryAt = 0;
  enqueue(task);
}

void HttpTaskQueue::enqueueCoinEvent(const CoinEvent& event) {
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
  enqueue(task);
}

void HttpTaskQueue::enqueueHeartbeat() {
  HttpTask task;
  task.id = generateId();
  task.type = HttpTask::HEARTBEAT;
  task.priority = HttpTask::priorityFor(HttpTask::HEARTBEAT);
  task.payload = "";
  task.retryCount = 0;
  task.createdAt = millis();
  task.nextRetryAt = 0;
  enqueue(task);
}

void HttpTaskQueue::clear() {
  queue.clear();
  save();
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

void HttpTaskQueue::save() {
  File file = LittleFS.open("/http_queue.csv", "w");
  if (!file) return;
  for (auto& t : queue) {
    file.printf("%s,%d,%d,%s,%d,%lu,%lu\n",
      t.id.c_str(),
      (int)t.type,
      (int)t.priority,
      t.payload.c_str(),
      t.retryCount,
      t.createdAt,
      t.nextRetryAt
    );
  }
  file.close();
}

void HttpTaskQueue::load() {
  queue.clear();
  File file = LittleFS.open("/http_queue.csv", "r");
  if (!file) return;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    HttpTask t;
    int p1 = line.indexOf(',');
    int p2 = line.indexOf(',', p1 + 1);
    int p3 = line.indexOf(',', p2 + 1);
    int pLast = line.lastIndexOf(',');
    int pSecondLast = line.lastIndexOf(',', pLast - 1);
    int pThirdLast = line.lastIndexOf(',', pSecondLast - 1);

    t.id = line.substring(0, p1);
    t.type = (HttpTask::Type)line.substring(p1 + 1, p2).toInt();
    t.priority = (HttpTask::Priority)line.substring(p2 + 1, p3).toInt();
    t.payload = line.substring(p3 + 1, pThirdLast);
    t.retryCount = line.substring(pThirdLast + 1, pSecondLast).toInt();
    t.createdAt = strtoul(line.substring(pSecondLast + 1, pLast).c_str(), nullptr, 10);
    t.nextRetryAt = strtoul(line.substring(pLast + 1).c_str(), nullptr, 10);

    queue.push_back(t);
  }
  file.close();

  // Fix: reset nextRetryAt so tasks send immediately on reboot
  // (absolute timestamps don't survive reboots since millis() resets)
  for (auto& task : queue) {
    task.nextRetryAt = 0;
    task.retryCount = 0;
  }

  sortQueue();
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
