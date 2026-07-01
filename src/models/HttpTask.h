#ifndef HTTP_TASK_H
#define HTTP_TASK_H

#include <Arduino.h>

struct HttpTask {
  enum Type { REGISTER, COIN_EVENT, HEARTBEAT };
  enum Priority { PRI_HIGH = 0, PRI_MEDIUM = 1, PRI_LOW = 2 };

  String id;
  Type type;
  Priority priority;
  String payload;          // JSON string for the request body
  int retryCount;
  unsigned long createdAt;
  unsigned long nextRetryAt; // millis() value when this task becomes ready

  static Priority priorityFor(Type t) {
    switch (t) {
      case REGISTER:   return PRI_HIGH;
      case COIN_EVENT: return PRI_MEDIUM;
      case HEARTBEAT:  return PRI_LOW;
      default:         return PRI_LOW;
    }
  }

  static const char* typeStr(Type t) {
    switch (t) {
      case REGISTER:   return "REGISTER";
      case COIN_EVENT: return "COIN_EVENT";
      case HEARTBEAT:  return "HEARTBEAT";
      default:         return "UNKNOWN";
    }
  }

  String toJson(char* buf, size_t bufsize) const {
    snprintf(buf, bufsize,
        "{\"id\":\"%s\",\"type\":\"%s\",\"priority\":%d,"
        "\"payload\":%s,\"retryCount\":%d,"
        "\"createdAt\":%lu,\"nextRetryAt\":%lu}",
        id.c_str(), typeStr(type), priority,
        payload.c_str(), retryCount,
        createdAt, nextRetryAt);
    return String(buf);
  }
};

#endif
