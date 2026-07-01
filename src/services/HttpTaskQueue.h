#pragma once

#include <vector>
#include "../models/HttpTask.h"
#include "../models/CoinEvent.h"

class HttpTaskQueue {
public:
  static void begin();
  static void enqueue(const HttpTask& task);
  static bool hasReadyTask();
  static HttpTask peek();
  static void removeFirst();
  static void requeueFront(unsigned long backoffMs);
  static int size();

  // Convenience enqueuers
  static void enqueueRegistration();
  static void enqueueCoinEvent(const CoinEvent& event);
  static void enqueueHeartbeat();
  static void clear();
  static const std::vector<HttpTask>& getAllTasks();
  
private:
  static std::vector<HttpTask> queue;
  static void sortQueue();
  static void save();
  static void load();
  static String generateId();
  static bool hasType(HttpTask::Type type);
};
