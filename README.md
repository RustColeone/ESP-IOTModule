# ESP32-C6 Based IOT Switch v3.0

A modular dual-output IOT switch control system with web interface and serial commands for ESP32-C6.

**Update**: Migrated from ESP8266 to ESP32-C6 with enhanced features including dual output control (Power Jack + USB), full CH224K PD control (all 3 CFG pins), and dual voltage monitoring (VBUS + VOUT).

## Project Structure

```
ESP-IOT-SourceCode/
├── ESP-IOT-SourceCode.ino  # Main entry point (ESP32-C6)
├── config.h                # Configuration & pin definitions
├── storage.h/cpp           # EEPROM storage management
├── hardware.h/cpp          # Hardware control (dual outputs, PD, buttons)
├── network.h/cpp           # WiFi & NTP time synchronization
├── scheduler.h/cpp         # Schedule management & execution
├── webserver.h/cpp         # Web UI & REST API
├── serial_cmd.h/cpp        # Serial command interface
├── API.md                  # Complete API documentation
├── MIGRATION_NOTES.md      # ESP8266 → ESP32-C6 migration details
└── QUICKSTART.md          # Quick start guide
```

## Features

### Dual Output Control
- **Power Jack Output** (GPIO11) - High power output for devices
- **USB Output** (GPIO10) - Secondary USB power output
- Independent control of both outputs via web, API, or buttons
- Persistent state saved to EEPROM

### Voltage Monitoring
- **VBUS Monitoring** (GPIO2 ADC) - Tracks PD input voltage
- **VOUT Monitoring** (GPIO3 ADC) - Tracks output voltage
- Real-time voltage display in web interface
- 12-bit ADC resolution for accurate measurements

### Web Interface
- **Modern responsive UI** - Works on desktop, tablet, and mobile
- **Real-time status monitoring** - Auto-refresh every 10 seconds
- **Dual output control** - Independent jack and USB control
- **Voltage display** - Shows both VBUS and VOUT in real-time
- **PD voltage selection** - Choose 5V, 9V, 12V, 15V, or 20V
- **Schedule management** - Add/remove scheduled actions
- **Configuration** - Set WiFi, timezone via web interface

### Serial Commands
All commands available via Serial @ 115200 baud:
- `/help` - Show command list
- `/wifi <SSID> <PASSWORD>` - Configure WiFi
- `/timezone <CODE>` - Set timezone (UTC+8, PST, JST, etc.)
- `/jack_on` / `/jack_off` - Power jack control
- `/usb_on` / `/usb_off` - USB output control
- `/pd <voltage>` - Set PD voltage (5/9/12/15/20)
- `/vbus` - Read VBUS voltage
- `/vout` - Read VOUT voltage
- `/do_at <HHMM> <on|off>` - Add schedule
- `/do_list` - List schedules
- `/do_remove_at <index>` - Remove schedule
- `/status` - Show system status

### REST API Endpoints

#### GET Endpoints
- `GET /` - Web UI
- `GET /api/status` - System status JSON (includes both outputs and voltages)
- `GET /api/schedules` - List all schedules

#### POST Endpoints
- `POST /api/powerjack` - Control power jack
  ```json
  {"state": true}
  ```
- `POST /api/usboutput` - Control USB output
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

See [API.md](ESP-IOT-SourceCode/API.md) for complete API documentation.

## 🔧 Hardware Setup

### Pin Mapping (ESP32-C6)

#### Button Inputs (Internal Pullup)
- **GPIO4** - Button 1 (Toggle Power Jack)
- **GPIO5** - Button 2 (Toggle USB Output)
- **GPIO6** - Button 3 (Cycle PD Voltage)
- **GPIO7** - Button 4 (Enable All Outputs)

#### Output Control
- **GPIO11** - Power Jack Enable (HIGH=on, LOW=off)
- **GPIO10** - USB Output Enable (LOW=on, HIGH=off) *[Inverted]*

#### CH224K PD Control
- **GPIO18** - CH224K CFG1 (SDIO_CLK)
- **GPIO19** - CH224K CFG2 (SDIO_DATA0)
- **GPIO20** - CH224K CFG3 (SDIO_DATA1)

#### Voltage Sensing (12-bit ADC)
- **GPIO2** - VBUS voltage sensing (PD input)
- **GPIO3** - VOUT voltage sensing (output)

### CH224K PD Voltage Selection
| Voltage | CFG1 | CFG2 | CFG3 | Binary |
|---------|------|------|------|--------|
| 5V      | HIGH | LOW  | LOW  | 1XX    |
| 9V      | LOW  | LOW  | LOW  | 000    |
| 12V     | LOW  | LOW  | HIGH | 001    |
| 15V     | LOW  | HIGH | HIGH | 011    |
| 20V     | LOW  | HIGH | LOW  | 010    |

### Voltage Divider Circuit
Both VBUS and VOUT require voltage dividers to scale down to 3.3V max:
```
VIN ──[R1]──┬──[R2]──┐ GND
            │
          ADC Pin (GPIO2/GPIO3)
```

**Current Configuration:**
- R1 = 47kΩ (high side)
- R2 = 5.1kΩ (low side)
- Ratio = (R1+R2)/R2 = 52.1/5.1 = 10.216
- Max safe input = 3.3V × 10.216 ≈ 33.7V

This configuration is suitable for monitoring up to 20V PD voltages with adequate safety margin.

If you use different resistor values, update `VBUS_DIVIDER_RATIO` and `VOUT_DIVIDER_RATIO` in `config.h`.

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
