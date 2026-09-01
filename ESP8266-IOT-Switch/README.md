# ESP8266 IOT Switch

An ESP8266-controlled power jack with USB-C PD negotiation, USB data pass-through, a web interface, serial commands, and scheduling.

## Overview

- **Power Jack output** (GPIO15/D8) — a switched high-power output (barrel jack / terminal block).
- **USB output** (GPIO10, CH217K load switch) — a USB-A port whose 5 V rail is switched; the D+/D− data lines pass through for normal USB data.
- **USB-C PD input** — a CH224K sink controller requests 5/9/12/15/20 V from a USB-C PD charger.
- **VBUS voltage monitoring** via the ESP8266 ADC (A0).
- **Web UI + REST API** and a **serial command** interface for control and configuration.
- Persistent state in EEPROM, an NTP-synced clock, and up to 10 schedules.
- Automatic manager discovery at `iot-switch-<12-digit-MAC>.local` using mDNS.

## Project Structure

```
ESP8266-IOT-Switch/
├── ESP8266-IOT-Switch.ino  # Main entry point (Arduino sketch)
├── config.h                # Configuration & pin definitions
├── storage.h/cpp           # EEPROM storage management
├── hardware.h/cpp          # Hardware control (dual outputs, PD, buttons)
├── app_network.h/cpp       # WiFi & NTP time synchronization
├── scheduler.h/cpp         # Schedule management & execution
├── app_webserver.h/cpp     # Web UI & REST API (ESP8266WebServer)
├── serial_cmd.h/cpp        # Serial command interface
├── API.md                  # Complete API documentation
└── README.md               # This file
```

## Features

### Dual Output Control
- **Power Jack Output** (GPIO15/D8) — high-power output for devices
- **USB Output** (GPIO10) — USB-A port, 5 V switched via CH217K; D+/D− data lines pass through
- Independent control of both outputs via web, API, or buttons
- Persistent state saved to EEPROM

### USB-C PD (Power Delivery)
- CH224K sink controller requests 5 V, 9 V, 12 V, 15 V, or 20 V from a USB-C PD charger
- Selected via web UI, serial command, or button

### Voltage Monitoring
- **VBUS Monitoring** (A0 ADC) — tracks PD input voltage
- 10-bit ADC resolution
- Single ADC on this ESP8266 variant — no separate VOUT sensing

### Web Interface
- Modern responsive UI — desktop, tablet, and mobile
- Real-time status via auto-refresh + Server-Sent Events
- Dual output control, voltage display, and PD selection
- Schedule management and WiFi/timezone configuration

### Serial Commands
All commands available via Serial @ 115200 baud:
- `/help` — show command list
- `/wifi <SSID> <PASSWORD>` — configure WiFi
- `/timezone <CODE>` — set timezone (UTC+8, PST, JST, etc.)
- `/jack_on` / `/jack_off` — power jack control
- `/usb_on` / `/usb_off` — USB output control
- `/pd <voltage>` — set PD voltage (5/9/12/15/20)
- `/vbus` — read VBUS voltage
- `/do_at <HHMM> <on|off>` — add schedule
- `/do_list` — list schedules
- `/do_remove_at <index>` — remove schedule
- `/status` — show system status

## REST API

### GET
- `GET /` — web UI
- `GET /api/device` — stable identity and manager-discovery metadata
- `GET /api/status` — system status JSON
- `GET /api/events` — Server-Sent Events stream for live status
- `GET /api/schedules` — list schedules

### POST / DELETE
- `POST /api/powerjack` — `{"state": true}`
- `POST /api/usboutput` — `{"state": true}`
- `POST /api/pd` — `{"voltage": 12}`
- `POST /api/schedule` — `{"time": "2315", "action": 1, "target": 0}`
- `DELETE /api/schedule/{index}`
- `POST /api/wifi` — `{"ssid": "...", "password": "..."}` (restarts device)
- `POST /api/timezone` — `{"timezone": "UTC+8"}`

See [API.md](API.md) for the complete API reference.

## Hardware Setup

### Pin Mapping (ESP8266)

#### Button Inputs (internal pull-up)
| GPIO | Label | Function |
|------|-------|----------|
| GPIO5 | D1 | Button 1 — toggle power jack |
| GPIO4 | D2 | Button 2 — toggle USB output |
| GPIO0 | D3 | Button 3 — cycle PD voltage *(boot-mode pin)* |
| GPIO2 | D4 | Button 4 — enable all outputs *(UART1 TX)* |

#### Outputs
| GPIO | Label | Function |
|------|-------|----------|
| GPIO15 | D8 | Power jack enable (HIGH = on) |
| GPIO10 | — | USB output enable (LOW = on, inverted) |

#### CH224K PD Control
| GPIO | Label | Function |
|------|-------|----------|
| GPIO14 | D5 | CH224K CFG1 |
| GPIO12 | D6 | CH224K CFG2 |
| GPIO13 | D7 | CH224K CFG3 |

#### Voltage Sensing
- **A0** — VBUS (PD input) sensing, 10-bit ADC

### ⚠️ Hardware Hazards — ESP8266-Specific Pin Caveats
These pins behave differently on ESP8266 than a typical MCU and **must not** be repurposed casually:
- **GPIO16 (D0)** — hard-wired to RST on most ESP8266 dev boards for deep-sleep wake. Driving it LOW at runtime resets the chip. **Do not use as a general output.**
- **GPIO9 / GPIO10 (SD2/SD3)** — SPI flash data lines used by the bootloader in QIO/QOUT flash mode (the common default). GPIO10 is currently used for the USB output and should be watched closely / migrated off if instability appears.
- **GPIO15 (D8)** — must be LOW at boot or the module won't boot. Now used for power jack enable; confirm nothing external holds this pin HIGH before `setup()` runs.
- **GPIO0 / GPIO2** — sampled at boot for flash/UART mode selection; both are used here for buttons with external pull-ups, per the boot-constraint notes in `config.h`.

### CH224K PD Voltage Selection
| Voltage | CFG1 | CFG2 | CFG3 | Binary |
|---------|------|------|------|--------|
| 5 V     | HIGH | LOW  | LOW  | 1XX    |
| 9 V     | LOW  | LOW  | LOW  | 000    |
| 12 V    | LOW  | LOW  | HIGH | 001    |
| 15 V    | LOW  | HIGH | HIGH | 011    |
| 20 V    | LOW  | HIGH | LOW  | 010    |

### Voltage Divider Circuit
VBUS requires a voltage divider to scale down to 3.3 V max:

```
VIN ──[R1]──┬──[R2]──┐ GND
            │
          ADC Pin (A0)
```

**Current configuration:**
- R1 = 47 kΩ (high side)
- R2 = 5.1 kΩ (low side)
- Ratio = (R1+R2)/R2 = 52.1/5.1 = 10.216
- Max safe input = 3.3 V × 10.216 ≈ 33.7 V

Suitable for monitoring up to 20 V PD with adequate safety margin. If you use different resistor values, update `VBUS_DIVIDER_RATIO` in `config.h`.

## Usage

### 1. First-time setup (serial)
1. Connect the ESP8266 to your computer via USB.
2. Open the Serial Monitor at 115200 baud.
3. Configure WiFi: `/wifi YourSSID YourPassword`.
4. Set timezone: `/timezone UTC+8`.

### 2. Web interface
1. Note the IP address shown in the Serial Monitor after connecting.
2. Open `http://[ESP8266_IP_ADDRESS]` in a browser.

### 3. API (Python example)
```python
import requests

requests.post('http://192.168.1.100/api/powerjack', json={'state': True})
requests.post('http://192.168.1.100/api/schedule',
              json={'time': '0730', 'action': 1, 'target': 0})
status = requests.get('http://192.168.1.100/api/status').json()
print(status)
```

## Persistent Storage

All settings are stored in EEPROM and survive power loss:
- WiFi credentials
- Timezone configuration
- Last known time
- PD voltage preference
- Power jack / USB output states
- All schedules (up to 10)

## Scheduler

- Up to **10 scheduled actions**
- **24-hour format** (0000–2359)
- **Persistent** — survives power loss
- **Automatic execution** based on system time
- **Duplicate prevention** — won't execute the same action twice per minute
- Each schedule can target the power jack, USB output, or both

## Timezone Support

- **UTC offsets**: UTC+8, UTC-5, etc.
- **US**: EST, EDT, CST, CDT, MST, MDT, PST, PDT
- **Asia**: JST, KST, HKT, CNST, IST
- **Europe**: CET, CEST, GMT
- **Pacific**: AEST, AEDT, NZST, NZDT

## Compilation

1. Install **ESP8266 board support** in Arduino IDE.
2. Select **Board**: Generic ESP8266 Module (or your specific board).
3. Open `ESP8266-IOT-Switch.ino` and upload.

## Notes

- Web server runs on port 80 (`ESP8266WebServer`).
- NTP servers: `pool.ntp.org`, `time.nist.gov`.
- Time updates hourly when WiFi is connected.
- WiFi reconnection attempts every 60 seconds.
- Button debounce: 50 ms.

## Customization

- **config.h** — change pin assignments, timing constants
- **app_webserver.cpp** — customize UI appearance, add endpoints
- **hardware.cpp** — modify button behaviors, add controls
- **scheduler.cpp** — change scheduling logic
- **serial_cmd.cpp** — add serial commands
