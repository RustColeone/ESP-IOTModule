# API Documentation

Base URL: `http://[ESP8266_IP_ADDRESS]`

## Authentication
No authentication required (local network only recommended)

## Browser dashboard access

`GET /api/device` and `GET /api/status` return read-only CORS headers for the
optional HTTPS/GitHub Pages dashboard. Their `OPTIONS` preflight routes are also
supported. Control, WiFi, timezone, and schedule mutation endpoints do not
enable cross-origin access; the dashboard opens this device's own UI for those
operations.

## Hardware Overview
- **ESP8266** with dual output control
- **Power Jack Output**: GPIO15 (HIGH=enable, LOW=disable)
- **USB Output**: GPIO10 (LOW=enable, HIGH=disable)
- **CH224K PD Control**: GPIO14 (CFG1), GPIO12 (CFG2), GPIO13 (CFG3)
- **Voltage Sensing**: A0 (VBUS only — no separate VOUT)
- **Button Inputs**: GPIO5, GPIO4, GPIO0, GPIO2 with internal pull-up

## Endpoints

### GET /
Returns the web UI HTML page.

**Response:** HTML page

---

### GET /api/device

Returns stable manager-discovery metadata. The `id` is derived from the full
Wi-Fi MAC address and remains stable across reboots, reflashing, and DHCP
address changes.

```json
{
  "protocol": "esp-iot/1",
  "id": "esp8266-84f3eba1b2c3",
  "name": "Power Switch a1b2c3",
  "type": "power-switch",
  "model": "ESP8266-IOT-Switch",
  "firmware": "3.1",
  "hostname": "iot-switch-84f3eba1b2c3",
  "ui": "/",
  "status": "/api/status",
  "capabilities": ["power-jack", "usb-output", "pd-voltage", "schedules"]
}
```

### GET /api/status
Get current system status.

**Response:**
```json
{
  "powerJack": true,
  "usbOutput": false,
  "vbus": 12.34,
  "wifi": "Connected",
  "ip": "192.168.1.100",
  "timezone": "UTC+8",
  "time": "2026-01-31 15:30:45",
  "pdVoltage": 12,
  "schedules": 3,
  "ssid": "MyNetwork"
}
```

**Fields:**
- `powerJack` (boolean) - Power jack output state
- `usbOutput` (boolean) - USB output state
- `vbus` (float) - Measured VBUS voltage (PD input)
- `wifi` (string) - Connection status
- `ip` (string) - Device IP address
- `timezone` (string) - Configured timezone
- `time` (string) - Current time or "Not synced"
- `pdVoltage` (int) - PD voltage setting (5/9/12/15/20)
- `schedules` (int) - Number of active schedules
- `ssid` (string) - Configured WiFi SSID

---

### GET /api/schedules
List all configured schedules.

**Response:**
```json
{
  "schedules": [
    {
      "time": "07:30",
      "action": "ON"
    },
    {
      "time": "23:15",
      "action": "OFF"
    }
  ]
}
```

---

### POST /api/powerjack
Control power jack output state.

**Request Body:**
```json
{
  "state": true
}
```

**Parameters:**
- `state` (boolean) - `true` for ON, `false` for OFF

**Response:**
```json
{
  "success": true
}
```

**Error Response:**
```json
{
  "error": "Missing body"
}
```

---

### POST /api/usboutput
Control USB output state.

**Request Body:**
```json
{
  "state": true
}
```

**Parameters:**
- `state` (boolean) - `true` for ON, `false` for OFF

**Response:**
```json
{
  "success": true
}
```

---

### POST /api/pd
Set PD voltage.

**Request Body:**
```json
{
  "voltage": 12
}
```

**Parameters:**
- `voltage` (int) - PD voltage (5, 9, 12, 15, or 20)

**Response:**
```json
{
  "success": true
}
```

**CH224K CFG Pin Configuration:**
- **5V**: CFG1=HIGH, CFG2=LOW, CFG3=LOW (1XX)
- **9V**: CFG1=LOW, CFG2=LOW, CFG3=LOW (000)
- **12V**: CFG1=LOW, CFG2=LOW, CFG3=HIGH (001)
- **15V**: CFG1=LOW, CFG2=HIGH, CFG3=HIGH (011)
- **20V**: CFG1=LOW, CFG2=HIGH, CFG3=LOW (010)

**Notes:**
- Device will verify voltage after setting
- VBUS voltage can be monitored via /api/status

---

### POST /api/schedule
Add a new schedule.

**Request Body:**
```json
{
  "time": "2315",
  "action": 1,
  "target": 0
}
```

**Parameters:**
- `time` (string) - Time in HHMM format (0000-2359)
- `action` (int) - `1` for ON, `0` for OFF
- `target` (int, optional) - `0` for both outputs, `1` for power jack only, `2` for USB output only

**Response:**
```json
{
  "success": true
}
```

**Error Response:**
```json
{
  "error": "Schedule list full"
}
```

**Limits:**
- Maximum 10 schedules
- Time must be valid (HH: 00-23, MM: 00-59)

---

### DELETE /api/schedule/{index}
Remove a schedule by index.

**URL Parameters:**
- `index` (int) - Schedule index (0-9)

**Response:**
```json
{
  "success": true
}
```

**Error Response:**
```json
{
  "error": "Invalid index"
}
```

---

### POST /api/wifi
Configure WiFi credentials.

**Request Body:**
```json
{
  "ssid": "MyNetwork",
  "password": "password123"
}
```

**Parameters:**
- `ssid` (string) - WiFi network name (max 63 chars)
- `password` (string) - WiFi password (max 63 chars)

**Response:**
```json
{
  "success": true
}
```

**Notes:**
- Device will restart after saving WiFi settings
- Connection attempt made on restart

---

### POST /api/timezone
Set timezone.

**Request Body:**
```json
{
  "timezone": "UTC+8"
}
```

**Parameters:**
- `timezone` (string) - Timezone code (max 7 chars)

**Supported Formats:**
- UTC offset: `UTC+8`, `UTC-5`
- Named: `EST`, `PST`, `JST`, `HKT`, etc.

**Response:**
```json
{
  "success": true
}
```

---

## Code Examples

### cURL

```bash
# Get status
curl http://192.168.1.100/api/status

# Turn power jack ON
curl -X POST http://192.168.1.100/api/powerjack \
  -H "Content-Type: application/json" \
  -d '{"state":true}'

# Add schedule
curl -X POST http://192.168.1.100/api/schedule \
  -H "Content-Type: application/json" \
  -d '{"time":"0730","action":1}'

# Remove schedule
curl -X DELETE http://192.168.1.100/api/schedule/0
```

### Python (requests)

```python
import requests

BASE_URL = "http://192.168.1.100"

# Get status
status = requests.get(f"{BASE_URL}/api/status").json()
print(f"Power: {status['powerJack']}")

# Turn power jack ON
requests.post(f"{BASE_URL}/api/powerjack", json={"state": True})

# Set PD voltage
requests.post(f"{BASE_URL}/api/pd", json={"voltage": 12})

# Add schedule
requests.post(f"{BASE_URL}/api/schedule", 
              json={"time": "0730", "action": 1})

# Get schedules
schedules = requests.get(f"{BASE_URL}/api/schedules").json()
for i, sched in enumerate(schedules["schedules"]):
    print(f"{i}: {sched['time']} -> {sched['action']}")

# Remove schedule
requests.delete(f"{BASE_URL}/api/schedule/0")

# Set timezone
requests.post(f"{BASE_URL}/api/timezone", 
              json={"timezone": "UTC+8"})
```

### JavaScript (Fetch API)

```javascript
const BASE_URL = "http://192.168.1.100";

// Get status
fetch(`${BASE_URL}/api/status`)
  .then(r => r.json())
  .then(data => console.log("Power:", data.powerJack));

// Turn power jack ON
fetch(`${BASE_URL}/api/powerjack`, {
  method: "POST",
  headers: {"Content-Type": "application/json"},
  body: JSON.stringify({state: true})
});

// Add schedule
fetch(`${BASE_URL}/api/schedule`, {
  method: "POST",
  headers: {"Content-Type": "application/json"},
  body: JSON.stringify({time: "0730", action: 1})
});

// Remove schedule
fetch(`${BASE_URL}/api/schedule/0`, {
  method: "DELETE"
});
```

### Node.js (axios)

```javascript
const axios = require('axios');

const BASE_URL = "http://192.168.1.100";

async function controlDevice() {
  // Get status
  const status = await axios.get(`${BASE_URL}/api/status`);
  console.log("Power:", status.data.powerJack);
  
  // Turn power jack ON
  await axios.post(`${BASE_URL}/api/powerjack`, {state: true});
  
  // Add schedule
  await axios.post(`${BASE_URL}/api/schedule`, {
    time: "0730",
    action: 1
  });
  
  // Get schedules
  const schedules = await axios.get(`${BASE_URL}/api/schedules`);
  console.log("Schedules:", schedules.data.schedules);
  
  // Remove schedule
  await axios.delete(`${BASE_URL}/api/schedule/0`);
}

controlDevice();
```

## Error Handling

All endpoints return appropriate HTTP status codes:
- **200 OK** - Request successful
- **400 Bad Request** - Invalid parameters or missing data
- **404 Not Found** - Endpoint doesn't exist

Error responses include a message:
```json
{
  "error": "Description of the error"
}
```

## Rate Limiting

No rate limiting implemented. For production use, consider:
- Implementing request throttling
- Adding authentication
- Using HTTPS (requires ESP8266 with enough memory)

## CORS

CORS is not configured. The API is intended for same-network access only.

## Live Updates

Live status updates are available via Server-Sent Events at `GET /api/events`. The web UI also polls `/api/status` as a fallback.

---

**Security Note:** This API has no authentication. It's designed for local network use only. Do not expose to the internet without adding proper security measures.
