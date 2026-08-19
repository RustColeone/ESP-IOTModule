#include "app_webserver.h"
#include "hardware.h"
#include "storage.h"
#include "app_network.h"
#include <WebServer.h>
#include <WiFi.h>

WebServer server(80);

// ---------------------------------------------------------------------------
// Web UI (single page)
// ---------------------------------------------------------------------------
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Fan Controller</title>
<style>
:root{--bg:#0e1116;--panel:#171b22;--card:#1e232d;--border:#2a3140;--text:#e8ebf1;--muted:#8b94a7;--accent:#3b82f6}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:var(--bg);color:var(--text);padding:18px;min-height:100vh}
.wrap{max-width:780px;margin:0 auto}
h1{font-size:1.25em;font-weight:600}
.sub{color:var(--muted);font-size:.8em;margin:4px 0 18px}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:12px}
.card{background:var(--card);border:1px solid var(--border);border-radius:12px;padding:16px}
.card h2{font-size:.95em;font-weight:600;margin-bottom:10px}
.metrics{display:flex;justify-content:space-between;font-size:.85em;color:var(--muted);margin-bottom:10px}
.metrics b{color:var(--text);font-variant-numeric:tabular-nums}
.rpm b{color:var(--accent)}
input[type=range]{width:100%;accent-color:var(--accent);cursor:pointer}
.btns{display:flex;gap:8px;margin-top:10px}
button{background:var(--accent);border:none;color:#fff;padding:9px 14px;border-radius:8px;font-size:.85em;cursor:pointer}
button.ghost{background:transparent;border:1px solid var(--border);color:var(--text)}
.sec{background:var(--panel);border:1px solid var(--border);border-radius:12px;padding:16px;margin-top:16px}
.sec h3{font-size:.72em;text-transform:uppercase;letter-spacing:.06em;color:var(--muted);margin-bottom:12px}
label{display:block;font-size:.78em;color:var(--muted);margin:10px 0 4px}
input[type=text],input[type=password]{width:100%;padding:9px;background:#11151b;border:1px solid var(--border);border-radius:8px;color:var(--text);font-size:.9em}
#toast{position:fixed;bottom:22px;left:50%;transform:translateX(-50%);background:#fff;color:#000;padding:9px 20px;border-radius:999px;font-size:.85em;opacity:0;transition:opacity .2s;pointer-events:none}
#toast.show{opacity:1}
@media(max-width:520px){.grid{grid-template-columns:1fr}}
</style>
</head>
<body>
<div class="wrap">
  <h1>Fan Controller</h1>
  <div class="sub">ESP32-C6 &middot; 4-fan PWM control &middot; <span id="ip">-</span></div>

  <div class="grid" id="fans"></div>

  <div class="sec">
    <h3>All Fans</h3>
    <input type="range" id="all" min="0" max="100" value="0" oninput="setAll(this.value)">
    <div class="metrics"><span id="allVal">0%</span></div>
  </div>

  <div class="sec">
    <h3>WiFi</h3>
    <label>SSID</label>
    <input type="text" id="ssid" placeholder="Network name">
    <label>Password</label>
    <input type="password" id="pass" placeholder="Password">
    <button onclick="setWiFi()">Save &amp; Restart</button>
  </div>

  <div class="sec">
    <h3>Timezone</h3>
    <label>Timezone</label>
    <input type="text" id="tz" placeholder="UTC+8">
    <button onclick="setTz()">Save</button>
  </div>
</div>
<div id="toast"></div>
<script>
function toast(m){var t=document.getElementById('toast');t.textContent=m;t.classList.add('show');setTimeout(function(){t.classList.remove('show')},2000)}
function render(d){
  document.getElementById('ip').textContent=d.ip;
  var h='';
  for(var i=0;i<d.fans.length;i++){
    var f=d.fans[i];
    h+='<div class="card"><h2>Fan '+i+'</h2>'+
       '<div class="metrics"><span class="rpm">RPM <b>'+f.rpm+'</b></span><span>Speed <b>'+f.speed+'%</b></span></div>'+
       '<input type="range" min="0" max="100" value="'+f.speed+'" oninput="setFan('+i+',this.value)">'+
       '<div class="btns"><button class="ghost" onclick="setFan('+i+',0)">Off</button><button class="ghost" onclick="setFan('+i+',100)">Full</button></div></div>';
  }
  document.getElementById('fans').innerHTML=h;
  document.getElementById('tz').value=d.timezone;
}
function load(){fetch('/api/status').then(function(r){return r.json()}).then(render).catch(function(){})}
function setFan(i,v){fetch('/api/fan/'+i,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({speed:parseInt(v)})}).then(load)}
function setAll(v){document.getElementById('allVal').textContent=v+'%';fetch('/api/all',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({speed:parseInt(v)})}).then(load)}
function setWiFi(){fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:document.getElementById('ssid').value,password:document.getElementById('pass').value})}).then(function(){toast('WiFi saved, restarting\u2026')})}
function setTz(){fetch('/api/timezone',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({timezone:document.getElementById('tz').value})}).then(function(){toast('Timezone saved');load()})}
setInterval(load,1500);load();
</script>
</body>
</html>
)rawliteral";

// ---------------------------------------------------------------------------
// JSON helpers & handlers
// ---------------------------------------------------------------------------
String buildStatusJson() {
  String json = "{\"fans\":[";
  for (int i = 0; i < FAN_COUNT; i++) {
    if (i > 0) json += ',';
    json += "{\"speed\":" + String(config.fanSpeed[i]) +
            ",\"rpm\":" + String((unsigned long)fanRpm[i]) + '}';
  }
  json += "],\"wifi\":\"";
  json += wifiConnected ? "Connected" : "Disconnected";
  json += "\",\"ip\":\"";
  json += WiFi.localIP().toString();
  json += "\",\"timezone\":\"";
  json += String(config.timezone);
  json += "\",\"time\":\"";

  char tbuf[32] = "Not synced";
  if (currentTime > 100000) {
    struct tm* ti = localtime(&currentTime);
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", ti);
  }
  json += tbuf;
  json += "\"}";
  return json;
}

void handleRoot()   { server.send(200, "text/html", INDEX_HTML); }
void handleStatus() { server.send(200, "application/json", buildStatusJson()); }

void handleSetFan() {
  // URI is "/api/fan/<index>" — parse the index from the path instead of
  // using UriBraces (not available on all WebServer library versions).
  String uri = server.uri();
  int idx = uri.substring(uri.lastIndexOf('/') + 1).toInt();
  if (idx < 0 || idx >= FAN_COUNT) {
    server.send(400, "application/json", "{\"error\":\"Invalid fan index\"}");
    return;
  }
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"Missing body\"}");
    return;
  }
  String body = server.arg("plain");
  int speed = 0;
  int pos = body.indexOf("\"speed\":");
  if (pos >= 0) speed = body.substring(pos + 8).toInt();
  setFanSpeed(idx, speed);
  server.send(200, "application/json", "{\"success\":true}");
}

void handleSetAll() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"Missing body\"}");
    return;
  }
  String body = server.arg("plain");
  int speed = 0;
  int pos = body.indexOf("\"speed\":");
  if (pos >= 0) speed = body.substring(pos + 8).toInt();
  setAllFanSpeed(speed);
  server.send(200, "application/json", "{\"success\":true}");
}

void handleSetWiFi() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"Missing body\"}");
    return;
  }
  String body = server.arg("plain");

  int ssidIdx = body.indexOf("\"ssid\":\"") + 8;
  int ssidEnd = body.indexOf("\"", ssidIdx);
  String ssid = body.substring(ssidIdx, ssidEnd);

  int passIdx = body.indexOf("\"password\":\"") + 12;
  int passEnd = body.indexOf("\"", passIdx);
  String password = body.substring(passIdx, passEnd);

  ssid.toCharArray(config.ssid, sizeof(config.ssid));
  password.toCharArray(config.password, sizeof(config.password));
  saveConfig();

  server.send(200, "application/json", "{\"success\":true}");
  delay(500);
  ESP.restart();
}

void handleSetTimezone() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"Missing body\"}");
    return;
  }
  String body = server.arg("plain");
  int tzIdx = body.indexOf("\"timezone\":\"") + 12;
  int tzEnd = body.indexOf("\"", tzIdx);
  String tz = body.substring(tzIdx, tzEnd);
  tz.toUpperCase();
  tz.toCharArray(config.timezone, sizeof(config.timezone));
  saveConfig();

  if (wifiConnected) updateTime();

  server.send(200, "application/json", "{\"success\":true}");
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/all", HTTP_POST, handleSetAll);
  server.on("/api/wifi", HTTP_POST, handleSetWiFi);
  server.on("/api/timezone", HTTP_POST, handleSetTimezone);
  // Register one route per fan index (avoids needing UriBraces support).
  for (int i = 0; i < FAN_COUNT; i++) {
    server.on("/api/fan/" + String(i), HTTP_POST, handleSetFan);
  }
  server.begin();
  Serial.print(F("Web server started: http://"));
  Serial.println(WiFi.localIP());
}

void handleWebServer() {
  server.handleClient();
}
