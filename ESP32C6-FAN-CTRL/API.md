# API Documentation

Base URL: `http://[ESP32C6_IP_ADDRESS]`

No authentication (local network use only).

## GET /api/device

Returns stable manager-discovery metadata. The `id` is derived from the full
Wi-Fi MAC address and remains stable across reboots, reflashing, and DHCP
address changes.

```json
{
  "protocol": "esp-iot/1",
  "id": "esp32c6-84f3eba1b2c3",
  "name": "Fan Controller a1b2c3",
  "type": "fan-controller",
  "model": "ESP32C6-FAN-CTRL",
  "firmware": "1.0",
  "hostname": "fan-controller-84f3eba1b2c3",
  "ui": "/",
  "status": "/api/status",
  "capabilities": ["fan-pwm", "fan-rpm"]
}
```

## GET /api/status

Get current status for all fans and the device.

**Response:**
```json
{
  "fans": [
    {"speed": 0,   "rpm": 0},
    {"speed": 50,  "rpm": 1200},
    {"speed": 0,   "rpm": 0},
    {"speed": 100, "rpm": 2400}
  ],
  "wifi": "Connected",
  "ip": "192.168.1.100",
  "timezone": "UTC+8",
  "time": "2026-08-16 12:00:00"
}
```

**Fields:**
- `fans` (array) — one entry per fan, in order 0-3.
- `fans[].speed` (int) — current PWM speed (0-100 %).
- `fans[].rpm` (int) — measured tachometer RPM (0 when fan is off or unconnected).
- `wifi` (string) — `"Connected"` or `"Disconnected"`.
- `ip` (string) — device IP address.
- `timezone` (string) — configured timezone code.
- `time` (string) — current time or `"Not synced"`.

## POST /api/fan/{index}

Set a single fan's speed.

**Path parameter:**
- `index` (int) — fan number `0`-`3`.

**Request body:**
```json
{"speed": 75}
```

**Response:**
```json
{"success": true}
```

## POST /api/all

Set all fans to the same speed.

**Request body:**
```json
{"speed": 50}
```

**Response:**
```json
{"success": true}
```

## POST /api/wifi

Configure WiFi credentials. The device restarts after saving.

**Request body:**
```json
{"ssid": "MyNetwork", "password": "password123"}
```

**Response:**
```json
{"success": true}
```

## POST /api/timezone

Set the timezone code.

**Request body:**
```json
{"timezone": "UTC+8"}
```

**Response:**
```json
{"success": true}
```

## Errors

Endpoints return `400 Bad Request` with a JSON error message on invalid input:

```json
{"error": "Invalid fan index"}
```
