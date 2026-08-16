# ESP8266 Based IOT Switch v3.1

A modular dual-output IOT switch control system with web interface and serial commands for ESP8266.

**Update**: This project was originally targeting an ESP32-C6, but due to chip stock shortages it has been reverted to run on ESP8266 instead. Pin assignments, ADC layout, and available features below reflect the **ESP8266** hardware actually in use. See [MIGRATION_NOTES.md](ESP-IOT-SourceCode/MIGRATION_NOTES.md) for historical ESP32-C6 details (kept for reference only — do not use its pin numbers).

## Project Structure

```
ESP-IOT-SourceCode/
├── ESP-IOT-SourceCode.ino  # Main entry point (ESP8266)
├── config.h                # Configuration & pin definitions
├── storage.h/cpp           # EEPROM storage management
├── hardware.h/cpp          # Hardware control (dual outputs, PD, buttons)
├── app_network.h/cpp       # WiFi & NTP time synchronization
├── scheduler.h/cpp         # Schedule management & execution
├── app_webserver.h/cpp     # Web UI & REST API (ESP8266WebServer)
├── serial_cmd.h/cpp        # Serial command interface
├── API.md                  # Complete API documentation
├── MIGRATION_NOTES.md      # ESP8266 → ESP32-C6 migration details (historical, stale pin numbers)
└── QUICKSTART.md          # Quick start guide
```

## Features

### Dual Output Control
- **Power Jack Output** (GPIO15/D8) - High power output for devices
- **USB Output** (GPIO10) - Secondary USB power output *(shares a SPI-flash pin — see Hardware Hazards below)*
- Independent control of both outputs via web, API, or buttons
- Persistent state saved to EEPROM

### Voltage Monitoring
- **VBUS Monitoring** (A0 ADC) - Tracks PD input voltage
- 10-bit ADC resolution
- This ESP8266 variant has a single ADC pin, so there is **no separate VOUT sensing** (unlike the original ESP32-C6 design)

### Web Interface
- **Modern responsive UI** - Works on desktop, tablet, and mobile
- **Real-time status monitoring** - Auto-refresh + Server-Sent Events
- **Dual output control** - Independent jack and USB control
- **Voltage display** - Shows VBUS in real-time
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
- `/do_at <HHMM> <on|off>` - Add schedule
- `/do_list` - List schedules
- `/do_remove_at <index>` - Remove schedule
- `/status` - Show system status

### REST API Endpoints

#### GET Endpoints
- `GET /` - Web UI
- `GET /api/status` - System status JSON
- `GET /api/events` - Server-Sent Events stream for live status push
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

See [API.md](ESP-IOT-SourceCode/API.md) for complete API documentation.

## 🔧 Hardware Setup

### Pin Mapping (ESP8266)

#### Button Inputs (Internal Pullup)
- **GPIO5 (D1)** - Button 1 (Toggle Power Jack)
- **GPIO4 (D2)** - Button 2 (Toggle USB Output)
- **GPIO0 (D3)** - Button 3 (Cycle PD Voltage) — *boot-mode pin, held LOW at reset forces flash mode*
- **GPIO2 (D4)** - Button 4 (Enable All Outputs) — *UART1 TX; boot emits 74880-baud noise on this pin*

#### Output Control
- **GPIO15 (D8)** - Power Jack Enable (HIGH=on, LOW=off)
- **GPIO10** - USB Output Enable (LOW=on, HIGH=off) *[Inverted]*

#### CH224K PD Control
- **GPIO14 (D5)** - CH224K CFG1
- **GPIO12 (D6)** - CH224K CFG2
- **GPIO13 (D7)** - CH224K CFG3

#### Voltage Sensing (10-bit ADC)
- **A0** - VBUS voltage sensing (PD input)

### ⚠️ Hardware Hazards — ESP8266-Specific Pin Caveats
These pins behave differently on ESP8266 than a typical MCU and **must not** be repurposed casually:
- **GPIO16 (D0)** - Hard-wired to RST on most ESP8266 dev boards for deep-sleep wake. Driving it LOW at runtime resets the chip. **Do not use as a general output.**
- **GPIO9 / GPIO10 (SD2/SD3)** - SPI flash data lines used by the bootloader in QIO/QOUT flash mode (the common default). Using GPIO9 as an output has caused boot loops; GPIO10 is currently used for USB Output and should be watched closely / migrated off if instability appears.
- **GPIO15 (D8)** - Must be LOW at boot or the module won't boot. Now used for Power Jack Enable; confirm nothing external holds this pin HIGH before `setup()` runs.
- **GPIO0 / GPIO2** - Sampled at boot for flash/UART mode selection; both are used here for buttons with external pull-ups, per the boot-constraint notes in `config.h`.

### CH224K PD Voltage Selection
| Voltage | CFG1 | CFG2 | CFG3 | Binary |
|---------|------|------|------|--------|
| 5V      | HIGH | LOW  | LOW  | 1XX    |
| 9V      | LOW  | LOW  | LOW  | 000    |
| 12V     | LOW  | LOW  | HIGH | 001    |
| 15V     | LOW  | HIGH | HIGH | 011    |
| 20V     | LOW  | HIGH | LOW  | 010    |

### Voltage Divider Circuit
VBUS requires a voltage divider to scale down to 3.3V max:
```
VIN ──[R1]──┬──[R2]──┐ GND
            │
          ADC Pin (A0)
```

**Current Configuration:**
- R1 = 47kΩ (high side)
- R2 = 5.1kΩ (low side)
- Ratio = (R1+R2)/R2 = 52.1/5.1 = 10.216
- Max safe input = 3.3V × 10.216 ≈ 33.7V

This configuration is suitable for monitoring up to 20V PD voltages with adequate safety margin.

If you use different resistor values, update `VBUS_DIVIDER_RATIO` in `config.h`.

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

# Set power jack ON
requests.post('http://192.168.1.100/api/powerjack',
              json={'state': True})

# Add schedule
requests.post('http://192.168.1.100/api/schedule',
              json={'time': '0730', 'action': 1})

# Get status
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
- Each schedule can target the power jack, USB output, or both

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

- Web server runs on port 80 (`ESP8266WebServer`)
- NTP servers: pool.ntp.org, time.nist.gov
- Time updates hourly when WiFi connected
- WiFi reconnection attempts every 60 seconds
- Button debounce: 50ms
- Watchdog-safe with 10ms loop delay

## Customization

Each module can be modified independently:
- **config.h** - Change pin assignments, timing constants
- **app_webserver.cpp** - Customize UI appearance, add new endpoints
- **hardware.cpp** - Modify button behaviors, add new controls
- **scheduler.cpp** - Change scheduling logic
- **serial_cmd.cpp** - Add new serial commands

---

**Made with ⚡ for ESP8266 - Enjoy your smart IOT switch!**

