# ESP-IOT Modules

A collection of self-contained ESP-based Internet-of-Things module projects.

Each project lives in its own folder with its own firmware, `README.md`, and (where applicable) `API.md`. Use the table below to navigate.

## Projects

| Project | Description | MCU |
|---------|-------------|-----|
| [ESP8266-IOT-Switch](ESP8266-IOT-Switch/README.md) | Power-jack switch with USB-C PD negotiation (CH224K), USB data pass-through (CH217K), web UI, serial commands, and scheduling | ESP8266 |
| [ESP32C6-FAN-CTRL](ESP32C6-FAN-CTRL/README.md) | 4-channel 4-wire fan controller (PWM + tach RPM) with web UI, REST API, serial commands, and buttons | ESP32-C6 |

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
└── ESP32C6-FAN-CTRL/          # Individual project (Arduino sketch)
    ├── ESP32C6-FAN-CTRL.ino
    ├── config.h
    ├── hardware.h/cpp
    ├── app_network.h/cpp
    ├── app_webservserver.h/cpp
    ├── scheduler.h/cpp
    ├── serial_cmd.h/cpp
    ├── storage.h/cpp
    ├── README.md
    └── API.md
```

## Conventions

- One Arduino sketch per project folder; the `.ino` filename matches its folder name.
- Each project is self-contained with its own `config.h`, `README.md`, and `API.md`.
- To add a new module, create a new folder and add a row to the table above.

