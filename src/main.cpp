#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

#include "core/WifiService.h"
#include "routes/ApiRoutes.h"
#include "hardware/CoinGate.h"
#include "hardware/CoinDetector.h"
#include "services/CoinService.h"
#include "services/SenderService.h"
#include "services/SessionContext.h"
#include "services/HttpTaskQueue.h"
#include "services/RegistrationService.h"
#include "services/HeartbeatService.h"
#include "services/TimeService.h"

ESP8266WebServer server(80);

void setup()
{
  Serial.begin(9600);

  if (!LittleFS.begin())
  {
    Serial.println("LittleFS mount failed");
    return;
  }
  Serial.println("LittleFS ready");

  SessionContext::begin();
  HttpTaskQueue::begin();
  CoinGate::begin();
  CoinDetector::begin();

  WifiService::connect();
  TimeService::begin();
  RegistrationService::begin();
  HeartbeatService::begin();

  // Register custom headers for HMAC verification
  server.collectHeaders("X-SIGNATURE", "X-TIMESTAMP");

  ApiRoutes::registerRoutes(server);

  server.begin();

  Serial.println("HTTP Server started");
  Serial.println("Chip ID: " + String(ESP.getChipId()));
}

void loop()
{
  server.handleClient();

  if (CoinService::isAccepting())
  {
    int pulses = CoinDetector::consumePulses();

    if (pulses > 0)
    {
      CoinService::onCoinPulses(pulses);
    }
  }

  // Process accumulated pulses (sends after timeout)
  CoinService::loop();

  // Retry time sync if not ready
  TimeService::retryIfNeeded();

  // Only run services after time is synced
  if (TimeService::isReady()) {
      RegistrationService::loop();
      HeartbeatService::loop();
      SenderService::loop();
  }
}
