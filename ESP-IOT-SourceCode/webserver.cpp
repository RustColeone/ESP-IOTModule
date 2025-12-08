#include "webserver.h"
#include "hardware.h"
#include "storage.h"
#include "network.h"
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

ESP8266WebServer server(80);

// HTML page
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>IOT Switch Control</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { 
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      padding: 20px;
    }
    .container { 
      max-width: 800px; 
      margin: 0 auto; 
      background: white; 
      border-radius: 12px; 
      padding: 30px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
    }
    h1 { 
      color: #667eea; 
      margin-bottom: 10px;
      font-size: 2em;
    }
    .subtitle { 
      color: #666; 
      margin-bottom: 30px; 
      font-size: 0.9em;
    }
    .section { 
      margin-bottom: 30px; 
      padding: 20px; 
      background: #f8f9fa; 
      border-radius: 8px;
    }
    .section h2 { 
      color: #333; 
      margin-bottom: 15px;
      font-size: 1.3em;
      border-bottom: 2px solid #667eea;
      padding-bottom: 8px;
    }
    .status-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
      gap: 15px;
      margin-top: 15px;
    }
    .status-item {
      background: white;
      padding: 12px;
      border-radius: 6px;
      border-left: 4px solid #667eea;
    }
    .status-label { 
      font-size: 0.85em; 
      color: #666; 
      margin-bottom: 5px;
    }
    .status-value { 
      font-size: 1.1em; 
      font-weight: bold; 
      color: #333;
    }
    .button-group { 
      display: flex; 
      gap: 10px; 
      flex-wrap: wrap;
      margin-top: 15px;
    }
    button { 
      padding: 12px 24px; 
      font-size: 16px; 
      border: none; 
      border-radius: 6px; 
      cursor: pointer;
      font-weight: 600;
      transition: all 0.3s;
      flex: 1;
      min-width: 120px;
    }
    .btn-on { 
      background: #10b981; 
      color: white;
    }
    .btn-on:hover { 
      background: #059669;
      transform: translateY(-2px);
      box-shadow: 0 4px 12px rgba(16, 185, 129, 0.4);
    }
    .btn-off { 
      background: #ef4444; 
      color: white;
    }
    .btn-off:hover { 
      background: #dc2626;
      transform: translateY(-2px);
      box-shadow: 0 4px 12px rgba(239, 68, 68, 0.4);
    }
    .btn-primary { 
      background: #667eea; 
      color: white;
    }
    .btn-primary:hover { 
      background: #5568d3;
      transform: translateY(-2px);
      box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
    }
    .btn-danger { 
      background: #f59e0b; 
      color: white;
    }
    .btn-danger:hover { 
      background: #d97706;
      transform: translateY(-2px);
      box-shadow: 0 4px 12px rgba(245, 158, 11, 0.4);
    }
    input, select { 
      padding: 10px; 
      width: 100%; 
      margin-top: 8px; 
      border: 2px solid #e5e7eb; 
      border-radius: 6px;
      font-size: 14px;
      transition: border-color 0.3s;
    }
    input:focus, select:focus { 
      outline: none; 
      border-color: #667eea;
    }
    .form-group { 
      margin-bottom: 15px;
    }
    .form-group label { 
      display: block; 
      margin-bottom: 5px; 
      font-weight: 600;
      color: #333;
    }
    .schedule-item {
      background: white;
      padding: 12px;
      margin-bottom: 10px;
      border-radius: 6px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      border-left: 4px solid #667eea;
    }
    .schedule-time { 
      font-weight: bold; 
      color: #667eea;
      font-size: 1.1em;
    }
    .schedule-action { 
      padding: 4px 12px; 
      border-radius: 12px; 
      font-size: 0.85em;
      font-weight: 600;
    }
    .action-on { 
      background: #d1fae5; 
      color: #065f46;
    }
    .action-off { 
      background: #fee2e2; 
      color: #991b1b;
    }
    .power-state {
      display: inline-block;
      padding: 8px 20px;
      border-radius: 20px;
      font-weight: bold;
      font-size: 1.2em;
    }
    .power-on { 
      background: #d1fae5; 
      color: #065f46;
    }
    .power-off { 
      background: #fee2e2; 
      color: #991b1b;
    }
    .refresh-btn {
      background: #8b5cf6;
      color: white;
      padding: 8px 16px;
      border-radius: 6px;
      border: none;
      cursor: pointer;
      font-size: 0.9em;
      float: right;
    }
    .refresh-btn:hover {
      background: #7c3aed;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>⚡ IOT Switch Control</h1>
    <p class="subtitle">ESP8266 Power Management System</p>

    <div class="section">
      <h2>📊 System Status 
        <button class="refresh-btn" onclick="loadStatus()">🔄 Refresh</button>
      </h2>
      <div class="status-grid" id="statusGrid">
        <div class="status-item">
          <div class="status-label">Loading...</div>
        </div>
      </div>
    </div>

    <div class="section">
      <h2>🔌 Power Control</h2>
      <div class="button-group">
        <button class="btn-on" onclick="setPower(true)">⚡ Turn ON</button>
        <button class="btn-off" onclick="setPower(false)">🔴 Turn OFF</button>
      </div>
    </div>

    <div class="section">
      <h2>⚙️ PD Voltage</h2>
      <div class="button-group">
        <button class="btn-primary" onclick="setPD(9)">9V</button>
        <button class="btn-primary" onclick="setPD(12)">12V</button>
        <button class="btn-primary" onclick="setPD(15)">15V</button>
        <button class="btn-primary" onclick="setPD(20)">20V</button>
      </div>
    </div>

    <div class="section">
      <h2>⏰ Schedule Manager</h2>
      <div id="scheduleList"></div>
      <div class="form-group" style="margin-top: 20px;">
        <label>Add New Schedule (24-hour format):</label>
        <input type="text" id="schedTime" placeholder="HHMM (e.g., 0730, 2315)" maxlength="4" 
               pattern="[0-2][0-9][0-5][0-9]" style="font-family: monospace;">
        <small style="display: block; margin-top: 5px; color: #666;">
          Enter time as 4 digits: 0000-2359 (e.g., 0730 = 7:30 AM, 2315 = 11:15 PM)
        </small>
        <select id="schedAction" style="margin-top: 10px;">
          <option value="1">Turn ON</option>
          <option value="0">Turn OFF</option>
        </select>
        <button class="btn-primary" onclick="addSchedule()" style="margin-top: 10px; width: 100%;">
          ➕ Add Schedule
        </button>
      </div>
    </div>

    <div class="section">
      <h2>📡 WiFi Configuration</h2>
      <div class="form-group">
        <label>SSID:</label>
        <input type="text" id="wifiSSID" placeholder="Network name">
      </div>
      <div class="form-group">
        <label>Password:</label>
        <input type="password" id="wifiPass" placeholder="Network password">
      </div>
      <button class="btn-primary" onclick="setWiFi()" style="width: 100%;">
        💾 Save WiFi Settings
      </button>
    </div>

    <div class="section">
      <h2>🌍 Timezone</h2>
      <div class="form-group">
        <label>Timezone Code:</label>
        <input type="text" id="timezone" placeholder="e.g., UTC+8, PST, JST">
        <small style="display: block; margin-top: 5px; color: #666;">
          Examples: UTC+8, UTC-5, EST, PST, JST, HKT
        </small>
      </div>
      <button class="btn-primary" onclick="setTimezone()" style="width: 100%;">
        💾 Save Timezone
      </button>
    </div>
  </div>

  <script>
    function loadStatus() {
      fetch('/api/status')
        .then(r => r.json())
        .then(data => {
          const grid = document.getElementById('statusGrid');
          const powerClass = data.power ? 'power-on' : 'power-off';
          const powerText = data.power ? 'ON' : 'OFF';
          
          grid.innerHTML = `
            <div class="status-item">
              <div class="status-label">Power State</div>
              <div class="status-value">
                <span class="power-state ${powerClass}">${powerText}</span>
              </div>
            </div>
            <div class="status-item">
              <div class="status-label">VBUS Voltage</div>
              <div class="status-value">${data.voltage.toFixed(2)} V</div>
            </div>
            <div class="status-item">
              <div class="status-label">WiFi</div>
              <div class="status-value">${data.wifi}</div>
            </div>
            <div class="status-item">
              <div class="status-label">IP Address</div>
              <div class="status-value">${data.ip}</div>
            </div>
            <div class="status-item">
              <div class="status-label">Timezone</div>
              <div class="status-value">${data.timezone}</div>
            </div>
            <div class="status-item">
              <div class="status-label">Current Time</div>
              <div class="status-value">${data.time}</div>
            </div>
            <div class="status-item">
              <div class="status-label">PD Voltage</div>
              <div class="status-value">${data.pdVoltage} V</div>
            </div>
            <div class="status-item">
              <div class="status-label">Schedules</div>
              <div class="status-value">${data.schedules} active</div>
            </div>
          `;
          loadSchedules();
        });
    }

    function loadSchedules() {
      fetch('/api/schedules')
        .then(r => r.json())
        .then(data => {
          const list = document.getElementById('scheduleList');
          if (data.schedules.length === 0) {
            list.innerHTML = '<p style="color: #666;">No schedules configured.</p>';
            return;
          }
          list.innerHTML = data.schedules.map((s, i) => `
            <div class="schedule-item">
              <div>
                <span class="schedule-time">${s.time}</span>
                <span class="schedule-action action-${s.action}">${s.action}</span>
              </div>
              <button class="btn-danger" onclick="removeSchedule(${i})">🗑️ Remove</button>
            </div>
          `).join('');
        });
    }

    function setPower(state) {
      fetch('/api/power', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({state: state})
      }).then(() => loadStatus());
    }

    function setPD(voltage) {
      fetch('/api/pd', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({voltage: voltage})
      }).then(() => {
        alert('PD voltage set to ' + voltage + 'V');
        loadStatus();
      });
    }

    function addSchedule() {
      const time = document.getElementById('schedTime').value;
      const action = document.getElementById('schedAction').value;
      if (!time) {
        alert('Please enter a time');
        return;
      }
      
      // Validate 24-hour format (0000-2359)
      const timeNum = parseInt(time);
      if (time.length !== 4 || isNaN(timeNum)) {
        alert('Please enter time as 4 digits (e.g., 0730, 2315)');
        return;
      }
      
      const hours = Math.floor(timeNum / 100);
      const mins = timeNum % 100;
      
      if (hours > 23 || mins > 59) {
        alert('Invalid time! Hours must be 00-23, minutes must be 00-59');
        return;
      }
      
      fetch('/api/schedule', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({time: time, action: parseInt(action)})
      }).then(() => {
        document.getElementById('schedTime').value = '';
        loadStatus();
      });
    }

    function removeSchedule(index) {
      fetch('/api/schedule/' + index, {method: 'DELETE'})
        .then(() => loadStatus());
    }

    function setWiFi() {
      const ssid = document.getElementById('wifiSSID').value;
      const pass = document.getElementById('wifiPass').value;
      if (!ssid) {
        alert('Please enter SSID');
        return;
      }
      fetch('/api/wifi', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({ssid: ssid, password: pass})
      }).then(() => {
        alert('WiFi settings saved. Device will restart.');
      });
    }

    function setTimezone() {
      const tz = document.getElementById('timezone').value;
      if (!tz) {
        alert('Please enter timezone');
        return;
      }
      fetch('/api/timezone', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({timezone: tz})
      }).then(() => {
        alert('Timezone saved');
        loadStatus();
      });
    }

    // Auto-refresh status every 10 seconds
    setInterval(loadStatus, 10000);
    loadStatus();
  </script>
</body>
</html>
)rawliteral";

// API Handlers
void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleStatus() {
  char timeStr[30] = "Not synced";
  if (currentTime > 100000) {
    struct tm *timeinfo = localtime(&currentTime);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
  }
  
  String json = "{";
  json += "\"power\":" + String(powerState ? "true" : "false") + ",";
  json += "\"voltage\":" + String(getVBusVoltage(), 2) + ",";
  json += "\"wifi\":\"" + String(wifiConnected ? "Connected" : "Disconnected") + "\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"timezone\":\"" + String(config.timezone) + "\",";
  json += "\"time\":\"" + String(timeStr) + "\",";
  json += "\"pdVoltage\":" + String(config.pdVoltage) + ",";
  json += "\"schedules\":" + String(config.scheduleCount);
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleGetSchedules() {
  String json = "{\"schedules\":[";
  for (int i = 0; i < config.scheduleCount; i++) {
    if (i > 0) json += ",";
    uint16_t t = config.schedules[i].time;
    char timeStr[6];
    sprintf(timeStr, "%02d:%02d", t / 100, t % 100);
    json += "{\"time\":\"" + String(timeStr) + "\",";
    json += "\"action\":\"" + String(config.schedules[i].action ? "ON" : "OFF") + "\"}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleSetPower() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    bool state = body.indexOf("true") > 0;
    setPowerState(state);
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing body\"}");
  }
}

void handleSetPD() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    int voltage = 9;
    if (body.indexOf("\"voltage\":9") > 0) voltage = 9;
    else if (body.indexOf("\"voltage\":12") > 0) voltage = 12;
    else if (body.indexOf("\"voltage\":15") > 0) voltage = 15;
    else if (body.indexOf("\"voltage\":20") > 0) voltage = 20;
    setPDVoltage(voltage);
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing body\"}");
  }
}

void handleAddSchedule() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    
    // Parse time
    int timeIdx = body.indexOf("\"time\":\"") + 8;
    int timeEnd = body.indexOf("\"", timeIdx);
    String timeStr = body.substring(timeIdx, timeEnd);
    uint16_t schedTime = timeStr.toInt();
    
    // Parse action
    int actionIdx = body.indexOf("\"action\":") + 9;
    uint8_t action = body.substring(actionIdx, actionIdx + 1).toInt();
    
    if (config.scheduleCount < 10) {
      config.schedules[config.scheduleCount].time = schedTime;
      config.schedules[config.scheduleCount].action = action;
      config.scheduleCount++;
      saveConfig();
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      server.send(400, "application/json", "{\"error\":\"Schedule list full\"}");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing body\"}");
  }
}

void handleRemoveSchedule() {
  String uri = server.uri();
  int index = uri.substring(uri.lastIndexOf('/') + 1).toInt();
  
  if (index >= 0 && index < config.scheduleCount) {
    for (int i = index; i < config.scheduleCount - 1; i++) {
      config.schedules[i] = config.schedules[i + 1];
    }
    config.scheduleCount--;
    saveConfig();
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid index\"}");
  }
}

void handleSetWiFi() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    
    // Parse SSID
    int ssidIdx = body.indexOf("\"ssid\":\"") + 8;
    int ssidEnd = body.indexOf("\"", ssidIdx);
    String ssid = body.substring(ssidIdx, ssidEnd);
    
    // Parse password
    int passIdx = body.indexOf("\"password\":\"") + 12;
    int passEnd = body.indexOf("\"", passIdx);
    String password = body.substring(passIdx, passEnd);
    
    ssid.toCharArray(config.ssid, 64);
    password.toCharArray(config.password, 64);
    saveConfig();
    
    server.send(200, "application/json", "{\"success\":true}");
    delay(1000);
    ESP.restart();
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing body\"}");
  }
}

void handleSetTimezone() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    
    int tzIdx = body.indexOf("\"timezone\":\"") + 12;
    int tzEnd = body.indexOf("\"", tzIdx);
    String tz = body.substring(tzIdx, tzEnd);
    
    tz.toCharArray(config.timezone, 8);
    saveConfig();
    
    if (wifiConnected) {
      updateTime();
    }
    
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing body\"}");
  }
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/schedules", HTTP_GET, handleGetSchedules);
  server.on("/api/power", HTTP_POST, handleSetPower);
  server.on("/api/pd", HTTP_POST, handleSetPD);
  server.on("/api/schedule", HTTP_POST, handleAddSchedule);
  server.on("/api/timezone", HTTP_POST, handleSetTimezone);
  server.on("/api/wifi", HTTP_POST, handleSetWiFi);
  
  // Handle DELETE for schedule removal
  server.onNotFound([]() {
    String uri = server.uri();
    if (uri.startsWith("/api/schedule/") && server.method() == HTTP_DELETE) {
      handleRemoveSchedule();
    } else {
      server.send(404, "text/plain", "Not found");
    }
  });
  
  server.begin();
  Serial.println(F("Web server started on port 80"));
}

void handleWebClient() {
  server.handleClient();
}
