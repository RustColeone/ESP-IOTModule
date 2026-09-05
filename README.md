# ESP-IOT Modules

A collection of self-contained ESP-based Internet-of-Things module projects.

Each firmware project lives in its own folder with its own `README.md` and
`API.md`. The local manager is a companion host application. Use the table
below to navigate.

## Projects

| Project | Description | MCU |
|---------|-------------|-----|
| [ESP8266-IOT-Switch](ESP8266-IOT-Switch/README.md) | Power-jack switch with USB-C PD negotiation (CH224K), USB data pass-through (CH217K), web UI, serial commands, and scheduling | ESP8266 |
| [ESP32C6-FAN-CTRL](ESP32C6-FAN-CTRL/README.md) | 4-channel 4-wire fan controller (PWM + tach RPM) with web UI, REST API, serial commands, and buttons | ESP32-C6 |
| [ESP-IOT-Manager](ESP-IOT-Manager/README.md) | Local mDNS discovery dashboard for finding and opening every ESP-IOT device | Python host |
| [ESP-IOT Web](docs/README.md) | GitHub Pages dashboard for manually pairing known local devices without running a local server | Browser |

## Repository Layout

```
ESP-IOTModule/
├── ESP8266-IOT-Switch/        # Individual project (Arduino sketch)
│   ├── ESP8266-IOT-Switch.ino
│   ├── config.h
│   ├── hardware.h/cpp
│   ├── app_network.h/cpp
│   ├── app_webserver.h/cpp
│   ├── scheduler.h/cpp
│   ├── serial_cmd.h/cpp
│   ├── storage.h/cpp
│   ├── README.md
│   └── API.md
├── ESP32C6-FAN-CTRL/          # Individual project (Arduino sketch)
│   ├── ESP32C6-FAN-CTRL.ino
│   ├── config.h
│   ├── hardware.h/cpp
│   ├── app_network.h/cpp
│   ├── app_webserver.h/cpp
│   ├── serial_cmd.h/cpp
│   ├── storage.h/cpp
│   ├── README.md
│   └── API.md
├── ESP-IOT-Manager/           # Automatic local mDNS discovery dashboard
│   ├── iot_manager/
│   ├── static/
│   ├── tests/
│   └── manager.py
└── docs/                      # Static GitHub Pages dashboard
    ├── index.html
    ├── app.js
    └── styles.css
```

## Dashboard Options

- Use **ESP-IOT Manager** on an always-on local computer for automatic mDNS
  discovery.
- Use **ESP-IOT Web** from GitHub Pages when users should not need to run a
  local service. The browser remembers devices after they are added once by
  `.local` hostname or IP address.

The web dashboard reads identity and status only. Device controls remain on the
device-hosted page opened by the dashboard.

The local manager also renders the simple controls advertised by each device:
power-switch toggles and fan-speed sliders. Full configuration is available in
an embedded, dismissible device panel or by navigating directly to the device.

## Conventions

- Each firmware folder has one Arduino sketch whose `.ino` filename matches the folder name.
- Firmware projects are self-contained with their own `config.h`, `README.md`, and `API.md`.
- To add a new module, create a firmware folder, implement the discovery contract, and add a row above.

