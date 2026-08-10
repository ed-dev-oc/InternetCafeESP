# InternetCafeESP

ESP8266 firmware for an internet cafe coin slot controller.

## Overview

This project is an Arduino-based firmware for a NodeMCU v2 / ESP-12E module on the ESP8266 platform.

It is intended for both developers working on the firmware and operators deploying the device on-site.

## Features

- Wi-Fi station mode with setup AP fallback
- Web-based device configuration page
- Persistent device settings stored in LittleFS
- Coin pulse detection and aggregation
- Outbound task queue persisted on flash
- HMAC-signed requests for protected endpoints
- Registration, heartbeat, coin enable/disable, reboot, and debug routes
- Remote configuration endpoints

## Tech Stack

- Language: C++
- Framework: Arduino for ESP8266
- Build system: PlatformIO
- Target board: `nodemcuv2`
- JSON library: ArduinoJson 7
- Storage: LittleFS
- Networking: `ESP8266WiFi`, `ESP8266WebServer`, `ESP8266HTTPClient`

## Build Profiles

`platformio.ini` defines two build environments:

- `development` - builds with `-D ENV_DEVELOPMENT`
- `release` - builds with `-D ENV_RELEASE`

The default environment is `release`.

Development builds enable debug-only routes such as:

- `POST /debug/coin-insert`
- `GET /debug/queue`

Release builds exclude those debug routes.

## Hardware Setup

The firmware currently uses one input and one output pin on the ESP8266:

| Function | ESP8266 Pin | GPIO | Direction | Behavior |
| --- | --- | --- | --- | --- |
| Coin pulse input | D2 | GPIO4 | Input | Uses `INPUT_PULLUP` and counts falling-edge pulses |
| Coin relay output | D1 | GPIO5 | Output | Active-low relay control: `LOW` enables, `HIGH` disables |

Hardware components expected by the firmware:

- ESP8266 board such as NodeMCU v2 / ESP-12E
- Coin selector / coin acceptor with a pulse output
- Relay module for coin gating

Notes:

- The coin pulse line is debounced in firmware.
- The relay line is initialized as an output and defaults to disabled.
- `DRY_RUN` in `src/config/AppConfig.h` can be used to disable actual hardware I/O for bench testing.

## Prerequisites

- PlatformIO with the ESP8266 toolchain installed
- An ESP8266 board compatible with `nodemcuv2`
- A serial connection for flashing and monitoring

## Build

Build the default release firmware:

```bash
platformio run
```

Build the development profile with debug routes enabled:

```bash
platformio run -e development
```

## Configuration

The firmware stores device settings in LittleFS and does not use a `.env` file.

Relevant persistent files:

- `/device_settings.json` - device configuration
- `/secret.txt` - registration secret and lock state
- `/http_queue.jsonl` - queued outbound tasks

Important device settings managed by the firmware:

- `wifi_ssid`
- `wifi_password`
- `device_name`
- `server_url`
- `admin_password`
- `use_static_ip`
- `local_ip`
- `gateway`
- `subnet`
- `primary_dns`
- `secondary_dns`

## Running

Typical development flow:

1. Build with `platformio run -e development` when you need debug routes.
2. Flash the firmware to the ESP8266 board.
3. Open the serial monitor to observe Wi-Fi, registration, queue, and coin-service logs.
4. Use the local HTTP configuration routes to adjust settings when needed.

## Routes

The route table is defined in `src/routes/ApiRoutes.h`.

Routes available in all builds:

- `GET /`
- `GET /config`
- `POST /config`
- `GET /api/device/config`
- `POST /api/device/config`
- `GET /ping`
- `POST /coin/enable`
- `POST /coin/disable`
- `POST /reboot`

Development-only routes, enabled only when `ENV_DEVELOPMENT` is defined:

- `POST /debug/coin-insert`
- `GET /debug/queue`

Protected endpoints use HMAC headers:

- `X-SIGNATURE`
- `X-TIMESTAMP`

## Project Structure

- `src/main.cpp` - application entry point
- `src/config/` - device settings and board configuration
- `src/controllers/` - HTTP handlers
- `src/core/` - Wi-Fi and HMAC helpers
- `src/hardware/` - coin detector and gate control
- `src/models/` - task and coin event data structures
- `src/routes/` - HTTP route registration
- `src/services/` - registration, queueing, sending, sync, and session state
- `platformio.ini` - PlatformIO environment definitions
- `test/` - placeholder for future tests

## Testing

No automated test suite is currently defined.

The current verification step is a PlatformIO build:

```bash
platformio run
```

## Notes

- The repository currently uses LittleFS for persistence.
- Development-only debug routes are compiled out of release builds.
- No license file was present at the time of writing.
