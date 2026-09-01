# ESP32-C6 Fan Controller

A 4-channel 4-wire PC fan controller built on the Seeed Studio XIAO ESP32C6, with a web UI, REST API, serial commands, and physical buttons.

## Overview

- **MCU**: Seeed Studio XIAO ESP32C6 (ESP32-C6, RISC-V, Wi-Fi 6).
- **Fan channels**: up to 4 four-wire (PWM) fans — each with an independent PWM output and tachometer (RPM) input.
- **Power**: USB-C PD source. A CH224K sink controller is strapped to always request **12 V**, and a buck-boost converter regulates whatever the charger provides into a steady **12 V** fan rail.
- **Control**: web terminal/UI, REST API, serial commands, or three push buttons.

## Project Structure

```
ESP32C6-FAN-CTRL/
├── ESP32C6-FAN-CTRL.ino  # Main entry point (Arduino sketch)
├── config.h              # Pin definitions & configuration
├── storage.h/cpp         # NVS persistence (Preferences)
├── hardware.h/cpp        # Fan PWM, RPM sensing, buttons
├── app_network.h/cpp     # WiFi & NTP time sync
├── app_webserver.h/cpp   # Web UI & REST API (WebServer)
├── serial_cmd.h/cpp      # Serial command interface
├── API.md                # API documentation
└── README.md             # This file
```

## Hardware

### Pin Map (Seeed Studio XIAO ESP32C6)

| XIAO label | GPIO | Function |
|-----------|------|----------|
| D0 | GPIO0  | Fan 0 Sense (tachometer) |
| D1 | GPIO1  | Fan 0 Control (PWM) |
| D2 | GPIO2  | Fan 1 Sense (tachometer) |
| D3 | GPIO21 | Fan 1 Control (PWM) |
| D4 | GPIO22 | Fan 2 Sense (tachometer) |
| D5 | GPIO23 | Fan 2 Control (PWM) |
| D6 | GPIO16 | Fan 3 Sense (tachometer) |
| D7 | GPIO17 | Fan 3 Control (PWM) |
| D8 | GPIO19 | Button — all fans −25% |
| D9 | GPIO20 | Button — all fans +25% |
| D10 | GPIO18 | Button — toggle all fans |

### 12 V PD Request (CH224K, hardware-strapped)

The CH224K requests a fixed 12 V using only resistors (no GPIO):

| CH224K pin | Strap | Result |
|-----------|-------|--------|
| CFG1 | 10 kΩ to GND (LOW) | 12 V request |
| CFG2 | 10 kΩ to GND (LOW) | 12 V request |
| CFG3 | 10 kΩ to 3V3 (HIGH) | 12 V request |

The PD output feeds a buck-boost converter that regulates whatever the charger provides into a clean 12 V rail for the fans. This means fans still receive 12 V even if the PD source can only supply 5/9/15/20 V.

### Fan Wiring (4-wire)

| Fan pin | Connect to |
|---------|-----------|
| 1 (GND) | GND |
| 2 (12V) | 12 V fan rail (from buck-boost) |
| 3 (TACH / Sense) | XIAO Sense pin (D0/D2/D4/D6) — open-collector, add a 10 kΩ pull-up to 3V3 |
| 4 (PWM / Control) | XIAO Control pin (D1/D3/D5/D7) |

## Features

- Independent **0–100 % PWM** control per fan at 25 kHz.
- **Tachometer RPM** readout for every fan.
- **Web UI** with per-fan sliders and live RPM.
- **REST API** (see [API.md](API.md)).
- **Serial commands** over USB @ 115200 baud.
- **Three buttons** for quick manual control.
- Persistent settings (WiFi, timezone, fan speeds) in NVS.
- Automatic manager discovery at `fan-controller-<12-digit-MAC>.local` using mDNS.
- Read-only status access from the optional HTTPS/GitHub Pages dashboard after
  browser Local Network Access permission.

## Serial Commands

```
/help
/status
/fan <0-3> <0-100>     # set one fan
/all <0-100>           # set all fans
/wifi <SSID> <PASSWORD>
/timezone <CODE>       # e.g. UTC+8
```

## REST API

| Method | Endpoint | Body |
|--------|----------|------|
| GET | `/api/status` | — |
| GET | `/api/device` | — |
| POST | `/api/fan/{0-3}` | `{"speed": 0-100}` |
| POST | `/api/all` | `{"speed": 0-100}` |
| POST | `/api/wifi` | `{"ssid":"...","password":"..."}` |
| POST | `/api/timezone` | `{"timezone":"UTC+8"}` |

See [API.md](API.md) for details.

## Compilation

1. Install **esp32** board support by Espressif (**≥ 3.0.0**, required for ESP32-C6):
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. Select **Board**: `XIAO_ESP32C6`.
3. Open `ESP32C6-FAN-CTRL.ino` and upload.

## Pin Caveats (XIAO ESP32C6)

- **D6/D7** are the hardware UART0 TX/RX — here they are used as plain GPIO, so avoid initializing `Serial0`/`Serial1` on them. The `Serial` monitor is USB CDC and is unaffected.
- **D1/D2** are low-power (LP) GPIOs; PWM and interrupts work on them, but verify fan 0/1 behavior on first bring-up.
- **D8/D9/D10** are the SPI SCK/MISO/MOSI pins — used here as button inputs, which is fine.
