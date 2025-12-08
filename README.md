# ESP8266 IOT Switch v2.0

A modular IOT switch control system with web interface and serial commands.

Some quick notes: I am using 8266 for now, but I am thinking C6 might be a better option, will do that later.

Original Code was written in a single Arduino file so I can just brain dead upload and all that, but I guess that's too long to read so I used LLM to restructure it into the current version, which I haven't test yet.

## Project Structure

```
ESP-IOT-SourceCode/
├── ESP-IOT-SourceCode.ino  # Main entry point
├── config.h                # Configuration & pin definitions
├── storage.h/cpp           # EEPROM storage management
├── hardware.h/cpp          # Hardware control (power, PD, buttons)
├── network.h/cpp           # WiFi & NTP time synchronization
├── scheduler.h/cpp         # Schedule management & execution
├── webserver.h/cpp         # Web UI & REST API
└── serial_cmd.h/cpp        # Serial command interface
```

## Features

### Web Interface
- **Modern responsive UI** - Works on desktop, tablet, and mobile
- **Real-time status monitoring** - Auto-refresh every 10 seconds
- **Power control** - Turn device ON/OFF with a click
- **PD voltage selection** - Choose 9V, 12V, 15V, or 20V
- **Schedule management** - Add/remove scheduled actions
- **Configuration** - Set WiFi, timezone via web interface

### Serial Commands
All commands available via Serial @ 115200 baud:
- `/help` - Show command list
- `/wifi <SSID> <PASSWORD>` - Configure WiFi
- `/timezone <CODE>` - Set timezone (UTC+8, PST, JST, etc.)
- `/on` / `/off` - Power control
- `/pd <voltage>` - Set PD voltage (9/12/15/20)
- `/do_at <HHMM> <on|off>` - Add schedule
- `/do_list` - List schedules
- `/do_remove_at <index>` - Remove schedule
- `/status` - Show system status

### REST API Endpoints

#### GET Endpoints
- `GET /` - Web UI
- `GET /api/status` - System status JSON
- `GET /api/schedules` - List all schedules

#### POST Endpoints
- `POST /api/power` - Set power state
  ```json
  {"state": true}
  ```
- `POST /api/pd` - Set PD voltage
  ```json
  {"voltage": 12}
  ```
- `POST /api/schedule` - Add schedule
  ```json
  {"time": "2315", "action": 1}
  ```
- `POST /api/wifi` - Configure WiFi (restarts device)
  ```json
  {"ssid": "MyNetwork", "password": "password123"}
  ```
- `POST /api/timezone` - Set timezone
  ```json
  {"timezone": "UTC+8"}
  ```

#### DELETE Endpoints
- `DELETE /api/schedule/{index}` - Remove schedule

## 🔧 Hardware Setup

### Pin Mapping
- **D1 (GPIO5)** - Power switch output
- **D2 (GPIO4)** - Button 1 (Toggle)
- **D3 (GPIO0)** - Button 2 (Power ON)
- **D4 (GPIO2)** - Button 3 (Power OFF)
- **D5 (GPIO14)** - Button 4 (Reserved)
- **D6 (GPIO12)** - CH224K CFG2
- **D7 (GPIO13)** - CH224K CFG3
- **A0** - Voltage sensing (VBUS * 10/57)

### CH224K PD Voltage Selection
| CFG1 | CFG2 | CFG3 | Voltage |
|------|------|------|---------|
| 1    | X    | X    | 5V      |
| 0    | 0    | 0    | 9V      |
| 0    | 0    | 1    | 12V     |
| 0    | 1    | 1    | 15V     |
| 0    | 1    | 0    | 20V     |

*Note: CFG1 has an auto switch feature for plug detection*

## 📡 Usage

### 1. First Time Setup (Serial)
1. Connect ESP8266 to computer via USB
2. Open Serial Monitor @ 115200 baud
3. Configure WiFi: `/wifi YourSSID YourPassword`
4. Set timezone: `/timezone UTC+8`

### 2. Web Interface Access
1. After WiFi connection, note the IP address shown in Serial Monitor
2. Open browser and navigate to: `http://[ESP8266_IP_ADDRESS]`
3. Use the web interface to control your device!

### 3. API Integration (Python Example)
```python
import requests

# Set power ON
requests.post('http://192.168.1.100/api/power', 
              json={'state': True})

# Add schedule
requests.post('http://192.168.1.100/api/schedule',
              json={'time': '0730', 'action': 1})

# Get status
status = requests.get('http://192.168.1.100/api/status').json()
print(f"Power: {status['power']}, Voltage: {status['voltage']}V")
```

## Persistent Storage

All settings are stored in EEPROM and survive power loss:
- WiFi credentials
- Timezone configuration
- Last known time
- PD voltage preference
- All schedules (up to 10)

## Web UI Features

- **Beautiful gradient design** - Purple/blue theme
- **Card-based layout** - Organized sections
- **Responsive buttons** - Hover effects and animations
- **Real-time updates** - Live status display
- **Color-coded states** - Green for ON, Red for OFF
- **Mobile-friendly** - Works on any screen size

## Scheduler

- Up to **10 scheduled actions**
- **24-hour format** (0000-2359)
- **Persistent** - Survives power loss
- **Automatic execution** - Based on system time
- **Duplicate prevention** - Won't execute same action twice per minute

## Timezone Support

Supports both named timezones and UTC offsets:
- **UTC Offset**: UTC+8, UTC-5, etc.
- **US**: EST, EDT, CST, CDT, MST, MDT, PST, PDT
- **Asia**: JST, KST, HKT, CNST, IST
- **Europe**: CET, CEST, GMT
- **Pacific**: AEST, AEDT, NZST, NZDT

## Compilation

1. Install **ESP8266 board support** in Arduino IDE
2. Select **Board**: Generic ESP8266 Module (or your specific board)
3. All `.cpp` files will be automatically compiled with the `.ino` file
4. Upload to your ESP8266

## Notes

- Web server runs on port 80
- NTP servers: pool.ntp.org, time.nist.gov
- Time updates hourly when WiFi connected
- WiFi reconnection attempts every 60 seconds
- Button debounce: 50ms
- Watchdog-safe with 10ms loop delay

## Customization

Each module can be modified independently:
- **config.h** - Change pin assignments, timing constants
- **webserver.cpp** - Customize UI appearance, add new endpoints
- **hardware.cpp** - Modify button behaviors, add new controls
- **scheduler.cpp** - Change scheduling logic
- **serial_cmd.cpp** - Add new serial commands

---

**Made with ⚡ for ESP8266 - Enjoy your smart IOT switch!**
