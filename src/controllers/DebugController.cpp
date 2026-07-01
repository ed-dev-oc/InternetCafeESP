#include "DebugController.h"
#include "../hardware/CoinDetector.h"
#include "../services/HttpTaskQueue.h"

void DebugController::coinDetectorSimulatePulse(ESP8266WebServer& server)
{
    CoinDetector::simulatePulse();

    server.send(
        200,
        "application/json",
        R"({"status":"ok"})"
    );
}

void DebugController::getQueue(ESP8266WebServer& server)
{
    const auto& tasks = HttpTaskQueue::getAllTasks();
    
    char json[512];
    int offset = snprintf(json, sizeof(json), "{\"count\":%d,\"tasks\":[", 
                         (int)tasks.size());
    
    for (size_t i = 0; i < tasks.size(); i++) {
        if (i > 0) offset += snprintf(json + offset, sizeof(json) - offset, ",");
        offset += snprintf(json + offset, sizeof(json) - offset,
            "{\"id\":\"%s\",\"type\":\"%s\",\"priority\":%d,\"retryCount\":%d,\"nextRetryAt\":%lu}",
            tasks[i].id.c_str(),
            HttpTask::typeStr(tasks[i].type),
            tasks[i].priority,
            tasks[i].retryCount,
            tasks[i].nextRetryAt);
    }
    
    snprintf(json + offset, sizeof(json) - offset, "]}");
    
    server.send(200, "application/json", json);
}
