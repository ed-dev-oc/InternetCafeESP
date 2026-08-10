#pragma once

#include <vector>
#include "../models/HttpTask.h"
#include "../models/CoinEvent.h"

class HttpTaskQueue {
public:
  static void begin();
  static bool enqueue(const HttpTask& task);
  static bool hasReadyTask();
  static HttpTask peek();
  static void removeFirst();
  static void requeueFront(unsigned long backoffMs);
  static int size();

  // Convenience enqueuers
  static bool enqueueRegistration();
  static bool enqueueCoinEvent(const CoinEvent& event);
  static bool enqueueHeartbeat();
  static void clear();
  static const std::vector<HttpTask>& getAllTasks();
  
private:
  static std::vector<HttpTask> queue;
  static void sortQueue();
  static bool save();
  static bool load();
  static String generateId();
  static bool hasType(HttpTask::Type type);
};
