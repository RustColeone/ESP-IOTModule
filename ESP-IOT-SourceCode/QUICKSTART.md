# Quick Start Guide

## 🚀 Get Started in 5 Minutes!

### Step 1: Upload the Code
1. Open `IOT8266.ino` in Arduino IDE
2. Select **Tools** → **Board** → **ESP8266 Boards** → **Generic ESP8266 Module** (or your specific board)
3. Select your **Port**
4. Click **Upload** ⬆️

### Step 2: Connect via Serial
1. Open **Serial Monitor** (Ctrl+Shift+M)
2. Set baud rate to **115200**
3. You'll see the startup message and help screen

### Step 3: Configure WiFi
Type in Serial Monitor:
```
/wifi YourNetworkName YourPassword
```
Wait for "WiFi connected!" message and note the IP address shown.

### Step 4: Access Web Interface
Open your browser and go to:
```
http://[IP_ADDRESS_FROM_STEP_3]
```

**Example:** `http://192.168.1.100`

### Step 5: Start Using!
You can now:
- ✅ Click buttons to turn power ON/OFF
- ✅ Set PD voltage (9V, 12V, 15V, 20V)
- ✅ Add schedules for automatic control
- ✅ Monitor system status in real-time

---

## 📱 Web Interface Overview

### Dashboard Sections

#### 1. System Status
- Shows power state, voltage, WiFi status
- Auto-refreshes every 10 seconds
- Click 🔄 Refresh button for manual update

#### 2. Power Control
- **⚡ Turn ON** - Powers device ON
- **🔴 Turn OFF** - Powers device OFF

#### 3. PD Voltage
- Click voltage buttons: 9V, 12V, 15V, or 20V
- Device will automatically configure CH224K
- Measured voltage displayed in status

#### 4. Schedule Manager
- View all scheduled actions
- Add new schedule with time picker
- Remove unwanted schedules

#### 5. WiFi Configuration
- Change WiFi network
- Device will restart after saving

#### 6. Timezone
- Set your timezone
- Supports UTC+X format and named zones

---

## 🔘 Physical Buttons

| Button | Location | Function |
|--------|----------|----------|
| Button 1 | D2 | Toggle power ON/OFF |
| Button 2 | D3 | Power ON |
| Button 3 | D4 | Power OFF |
| Button 4 | D5 | Reserved (future use) |

---

## 💻 Command Line Usage

### Essential Commands

```bash
# View help
/help

# Check status
/status

# Power control
/on
/off

# Set timezone
/timezone UTC+8
/timezone PST

# Add schedule (7:30 AM turn ON)
/do_at 0730 on

# Add schedule (11:15 PM turn OFF)
/do_at 2315 off

# List all schedules
/do_list

# Remove schedule at index 0
/do_remove_at 0

# Clear all schedules
/do_remove_at -1

# Set PD voltage
/pd 12
```

---

## 🔌 API Quick Reference

### Turn Power ON
```bash
curl -X POST http://192.168.1.100/api/power \
  -H "Content-Type: application/json" \
  -d '{"state":true}'
```

### Get Status
```bash
curl http://192.168.1.100/api/status
```

### Add Schedule
```bash
curl -X POST http://192.168.1.100/api/schedule \
  -H "Content-Type: application/json" \
  -d '{"time":"0730","action":1}'
```

See [API.md](API.md) for complete API documentation.

---

## 🐛 Troubleshooting

### Can't Connect to WiFi
1. Check SSID and password are correct
2. Make sure you're on 2.4GHz network (ESP8266 doesn't support 5GHz)
3. Move ESP8266 closer to router
4. Try restarting: `/wifi YourSSID YourPassword`

### Web Page Won't Load
1. Verify WiFi is connected: `/status`
2. Check IP address matches what you're accessing
3. Ping the device: `ping [IP_ADDRESS]`
4. Try different browser

### Power Won't Turn ON/OFF
1. Check D1 pin connection
2. Verify hardware is properly wired
3. Check `/status` for power state
4. Try button control (D2, D3, D4)

### Schedule Not Executing
1. Verify time is synchronized: `/status`
2. Check timezone is correct: `/timezone UTC+8`
3. List schedules: `/do_list`
4. Make sure WiFi is connected for accurate time

### PD Voltage Not Changing
1. Ensure CFG1 is set to LOW (physical switch)
2. Check CH224K wiring to D6 and D7
3. Wait 0.5s after setting for voltage to stabilize
4. Verify with multimeter

---

## 📊 System Status Indicators

### Power State
- 🟢 **Green "ON"** - Device powered
- 🔴 **Red "OFF"** - Device not powered

### WiFi Status
- ✅ **Connected** - Shows IP address
- ❌ **Disconnected** - No network connection

### Time Status
- ⏰ **Synchronized** - Shows current date/time
- ⚠️ **Not synchronized** - Needs WiFi connection

---

## 🎯 Common Use Cases

### 1. Turn ON at 7:00 AM, OFF at 11:00 PM
```
/do_at 0700 on
/do_at 2300 off
```

### 2. Remote Control from PC
```python
import requests
requests.post('http://192.168.1.100/api/power', json={'state': True})
```

### 3. Home Assistant Integration
Add REST command in configuration.yaml:
```yaml
rest_command:
  iot_switch_on:
    url: 'http://192.168.1.100/api/power'
    method: POST
    payload: '{"state":true}'
    content_type: 'application/json'
```

### 4. Monitor Voltage
Access web UI and watch "VBUS Voltage" in status section.

---

## 🛡️ Safety Tips

1. **Don't expose to internet** - No authentication implemented
2. **Use proper power supply** - Respect PD voltage limits
3. **Check connections** - Verify wiring before powering ON
4. **CFG1 physical switch** - Keep as safety override
5. **Test schedules** - Verify behavior before leaving unattended

---

## 📚 Learn More

- [README.md](README.md) - Full documentation
- [API.md](API.md) - Complete API reference
- Check comments in source files for implementation details

---

**Need Help?** Check the serial monitor output for detailed error messages and status updates.

**Happy Switching! ⚡**
