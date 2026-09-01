#include "app_webserver.h"
#include "hardware.h"
#include "storage.h"
#include "app_network.h"
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

ESP8266WebServer server(80);
WiFiClient sseClient;
bool sseClientActive = false;
String lastSseSignature = "";
unsigned long lastSseHeartbeat = 0;
static const unsigned long SSE_HEARTBEAT_INTERVAL = 10000;

// Read-only dashboard endpoints are accessible from an HTTPS-hosted static
// page after the browser grants Local Network Access permission. Control
// endpoints intentionally do not opt in to cross-origin access.
void addDashboardCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Access-Control-Allow-Private-Network", "true");
  server.sendHeader("Cache-Control", "no-store");
}

void handleDashboardOptions() {
  addDashboardCorsHeaders();
  server.send(204, "text/plain", "");
}

String buildStatusJson() {
  char timeStr[30] = "Not synced";
  if (currentTime > 100000) {
    struct tm *timeinfo = localtime(&currentTime);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
  }

  char json[400];
  snprintf(json, sizeof(json),
    "{\"powerJack\":%s,\"usbOutput\":%s,\"vbus\":%.2f,"
    "\"wifi\":\"%s\",\"ip\":\"%s\",\"timezone\":\"%s\","
    "\"time\":\"%s\",\"pdVoltage\":%d,\"schedules\":%d,\"ssid\":\"%s\"}",
    powerJackState ? "true" : "false",
    usbOutputState ? "true" : "false",
    getVBusVoltage(),
    wifiConnected ? "Connected" : "Disconnected",
    WiFi.localIP().toString().c_str(),
    config.timezone,
    timeStr,
    config.pdVoltage,
    config.scheduleCount,
    config.ssid
  );
  return String(json);
}

String buildSseSignature() {
  char sig[40];
  snprintf(sig, sizeof(sig), "%d:%d:%d:%d:%d:%s",
    powerJackState ? 1 : 0,
    usbOutputState ? 1 : 0,
    config.pdVoltage,
    config.scheduleCount,
    wifiConnected ? 1 : 0,
    config.timezone
  );
  return String(sig);
}

void sendSseStatusIfNeeded(bool force) {
  if (!sseClientActive) return;

  if (!sseClient.connected()) {
    sseClient.stop();
    sseClientActive = false;
    lastSseSignature = "";
    return;
  }

  String signature = buildSseSignature();
  if (force || signature != lastSseSignature) {
    String statusJson = buildStatusJson();
    sseClient.print("event: status\n");
    sseClient.print("data: ");
    sseClient.print(statusJson);
    sseClient.print("\n\n");
    lastSseSignature = signature;
    lastSseHeartbeat = millis();
  }

  unsigned long now = millis();
  if (now - lastSseHeartbeat >= SSE_HEARTBEAT_INTERVAL) {
    sseClient.print(": keep-alive\n\n");
    lastSseHeartbeat = now;
  }
}

void handleEvents() {
  if (sseClientActive && sseClient.connected()) sseClient.stop();

  WiFiClient client = server.client();
  client.setNoDelay(true);
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/event-stream");
  client.println("Cache-Control: no-cache");
  client.println("Connection: keep-alive");
  client.println("Access-Control-Allow-Origin: *");
  client.println();

  sseClient = client;
  sseClientActive = true;
  lastSseSignature = "";
  lastSseHeartbeat = millis();

  sseClient.print("event: connected\n");
  sseClient.print("data: {\"ok\":true}\n\n");
  sendSseStatusIfNeeded(true);
}

// HTML page
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>IOT Switch</title>
  <script>
    (function(){
      var t=localStorage.getItem('iot-theme')||
        (window.matchMedia('(prefers-color-scheme:dark)').matches?'dark':'light');
      document.documentElement.setAttribute('data-theme',t);
    })();
  </script>
  <style>
    :root {
      --bg:#f0f2f5; --surface:#fff; --surface2:#f5f6f7;
      --border:#e2e4e8; --border-s:#c0c2c8;
      --text:#18181b; --text2:#6b7280;
      --accent:#2563eb; --accent-bg:rgba(37,99,235,.1);
      --en:#15803d; --en-bg:rgba(21,128,61,.1);
      --dis:#b91c1c; --dis-bg:rgba(185,28,28,.1);
      --on-bg:#dcfce7; --off-bg:#fee2e2;
      --shadow:0 1px 4px rgba(0,0,0,.08),0 0 0 1px rgba(0,0,0,.04);
      --r:10px;
    }
    [data-theme="dark"] {
      --bg:#0f0f11; --surface:#1a1a1d; --surface2:#242428;
      --border:#38383f; --border-s:#52525c;
      --text:#f0f0f2; --text2:#8b8b9a;
      --accent:#4a8ef5; --accent-bg:rgba(74,142,245,.14);
      --en:#4ade80; --en-bg:rgba(74,222,128,.12);
      --dis:#f87171; --dis-bg:rgba(248,113,113,.12);
      --on-bg:#052e16; --off-bg:#3b0a0a;
      --shadow:0 1px 4px rgba(0,0,0,.4),0 0 0 1px rgba(255,255,255,.04);
    }
    *{margin:0;padding:0;box-sizing:border-box;}
    body{
      font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;
      background:var(--bg);color:var(--text);
      padding:20px;min-height:100vh;
      transition:background .2s,color .2s;
    }
    .container{
      max-width:760px;margin:0 auto;
      background:var(--surface);
      border-radius:var(--r);
      border:1px solid var(--border);
      box-shadow:var(--shadow);
      overflow:hidden;
    }
    .hdr{
      padding:18px 22px 14px;
      border-bottom:1px solid var(--border);
      display:flex;justify-content:space-between;align-items:flex-start;
    }
    .hdr h1{font-size:1.25em;font-weight:600;letter-spacing:-.02em;}
    .hdr .sub{font-size:.78em;color:var(--text2);margin-top:2px;}
    .theme-btn{
      background:var(--surface2);border:1px solid var(--border);
      border-radius:6px;padding:5px 10px;
      font-size:.8em;color:var(--text2);cursor:pointer;
      flex-shrink:0;margin-left:12px;
      transition:border-color .15s,color .15s;
    }
    .theme-btn:hover{border-color:var(--accent);color:var(--text);}
    .sec{padding:18px 22px;border-bottom:1px solid var(--border);}
    .sec:last-child{border-bottom:none;}
    .sec-title{
      font-size:.68em;font-weight:600;letter-spacing:.07em;
      text-transform:uppercase;color:var(--text2);
      margin-bottom:12px;
      display:flex;justify-content:space-between;align-items:center;
    }
    /* Status grid */
    .sgrid{
      display:grid;
      grid-template-columns:repeat(auto-fit,minmax(140px,1fr));
      gap:8px;
    }
    .si{
      background:var(--surface2);border:1px solid var(--border);
      border-radius:7px;padding:10px 12px;
    }
    .sl{font-size:.7em;color:var(--text2);margin-bottom:3px;}
    .sv{font-size:.92em;font-weight:600;}
    .s-on{color:var(--en);}
    .s-off{color:var(--dis);}
    /* Buttons */
    .brow{display:flex;gap:8px;flex-wrap:wrap;}
    .brow.nw{flex-wrap:nowrap;}
    .brow.nw button{min-width:0;}
    button{
      padding:9px 16px;font-size:.875em;font-weight:500;
      border:1px solid var(--border);border-radius:6px;
      background:var(--surface2);color:var(--text);
      cursor:pointer;flex:1;min-width:90px;
      transition:border-color .15s,background .15s,color .15s,opacity .15s;
    }
    button:hover{border-color:var(--border-s);}
    /* Enable = green tint */
    .btn-en{
      background:var(--en-bg);border-color:var(--en);color:var(--en);
    }
    .btn-en:hover{opacity:.82;}
    /* Disable = red tint */
    .btn-dis{
      background:var(--dis-bg);border-color:var(--dis);color:var(--dis);
    }
    .btn-dis:hover{opacity:.82;}
    /* Primary (save/add actions) */
    button.prim{
      background:var(--accent);border-color:var(--accent);color:#fff;
    }
    button.prim:hover{opacity:.88;}
    .bd{opacity:.28;}
    /* Active PD voltage */
    .pa{background:var(--accent-bg);border-color:var(--accent);color:var(--accent);}
    /* Schedule rows */
    .si-row{
      display:flex;justify-content:space-between;align-items:center;
      background:var(--surface2);border:1px solid var(--border);
      border-radius:6px;padding:8px 10px;margin-bottom:6px;
    }
    .st{font-weight:600;font-size:.9em;}
    .sa{
      font-size:.72em;padding:2px 7px;border-radius:20px;
      font-weight:600;margin-left:6px;
    }
    .a-on{background:var(--on-bg);color:var(--en);}
    .a-off{background:var(--off-bg);color:var(--dis);}
    .stgt{font-size:.72em;color:var(--text2);margin-left:6px;}
    .sdel{
      background:none;border:none;color:var(--text2);
      font-size:.78em;cursor:pointer;padding:3px 8px;
      flex:none;min-width:0;border-radius:4px;
      transition:color .15s;
    }
    .sdel:hover{color:var(--dis);border:none;}
    /* Forms */
    .fg{margin-bottom:12px;}
    .fg label{display:block;font-size:.78em;color:var(--text2);font-weight:500;margin-bottom:4px;}
    input,select{
      width:100%;padding:8px 11px;
      background:var(--surface2);border:1px solid var(--border);
      border-radius:6px;font-size:.88em;color:var(--text);
      transition:border-color .15s;
    }
    input:focus,select:focus{outline:none;border-color:var(--accent);}
    small{font-size:.73em;color:var(--text2);display:block;margin-top:4px;}
    .row2{display:flex;gap:8px;}
    .row2>*{flex:1;}
    /* Toast */
    #toast{
      position:fixed;bottom:22px;left:50%;transform:translateX(-50%);
      background:var(--text);color:var(--surface);
      padding:9px 20px;border-radius:999px;font-size:.83em;
      opacity:0;transition:opacity .2s;z-index:9999;
      pointer-events:none;white-space:nowrap;
    }
    #toast.show{opacity:1;}
    .rfbtn{
      background:none;border:none;color:var(--text2);
      font-size:.8em;cursor:pointer;padding:2px 6px;
      flex:none;min-width:0;border-radius:4px;
    }
    .rfbtn:hover{color:var(--accent);border:none;}
    @media(max-width:480px){
      body{padding:0;}
      .container{border-radius:0;border-left:none;border-right:none;}
      .hdr,.sec{padding:14px 16px;}
      .sgrid{grid-template-columns:repeat(3,1fr);gap:6px;}
      .si{padding:8px 9px;}
      .sl{font-size:.65em;}
      .sv{font-size:.84em;}
      button{padding:9px 10px;font-size:.84em;min-width:50px;}
      .row2{flex-direction:column;}
    }
  </style>
</head>
<body>
<div id="toast"></div>
<div class="container">

  <div class="hdr">
    <div>
      <h1>IOT Switch</h1>
      <div class="sub">ESP8266 &middot; Dual Output Power Management</div>
    </div>
    <button class="theme-btn" onclick="toggleTheme()">&#9680; Theme</button>
  </div>

  <div class="sec">
    <div class="sec-title">
      System Status
      <button class="rfbtn" onclick="loadStatus()">&#8635; Refresh</button>
    </div>
    <div class="sgrid" id="statusGrid">
      <div class="si"><div class="sl">Loading&hellip;</div></div>
    </div>
  </div>

  <div class="sec">
    <div class="sec-title">Power Jack</div>
    <div class="brow">
      <button id="jack-on"  class="btn-en"  onclick="setPowerJack(true)">Enable</button>
      <button id="jack-off" class="btn-dis" onclick="setPowerJack(false)">Disable</button>
    </div>
  </div>

  <div class="sec">
    <div class="sec-title">USB Output</div>
    <div class="brow">
      <button id="usb-on"  class="btn-en"  onclick="setUSBOutput(true)">Enable</button>
      <button id="usb-off" class="btn-dis" onclick="setUSBOutput(false)">Disable</button>
    </div>
  </div>

  <div class="sec">
    <div class="sec-title">PD Voltage</div>
    <div class="brow nw">
      <button id="pd-5"  onclick="setPD(5)">5 V</button>
      <button id="pd-9"  onclick="setPD(9)">9 V</button>
      <button id="pd-12" onclick="setPD(12)">12 V</button>
      <button id="pd-15" onclick="setPD(15)">15 V</button>
      <button id="pd-20" onclick="setPD(20)">20 V</button>
    </div>
  </div>

  <div class="sec">
    <div class="sec-title">Schedule Manager</div>
    <div id="scheduleList" style="margin-bottom:14px;"></div>
    <div class="row2">
      <div class="fg">
        <label>Time</label>
        <input type="time" id="schedTime">
      </div>
      <div class="fg">
        <label>Output</label>
        <select id="schedTarget">
          <option value="0">Both Outputs</option>
          <option value="1">Power Jack Only</option>
          <option value="2">USB Output Only</option>
        </select>
      </div>
    </div>
    <div class="fg">
      <label>Action</label>
      <select id="schedAction">
        <option value="1">Turn ON</option>
        <option value="0">Turn OFF</option>
      </select>
    </div>
    <button class="prim" onclick="addSchedule()" style="width:100%;">Add Schedule</button>
  </div>

  <div class="sec">
    <div class="sec-title">WiFi</div>
    <div class="fg">
      <label>SSID</label>
      <input type="text" id="wifiSSID" placeholder="Network name">
    </div>
    <div class="fg">
      <label>Password</label>
      <input type="password" id="wifiPass" placeholder="Password">
    </div>
    <button class="prim" onclick="setWiFi()" style="width:100%;">Save &amp; Restart</button>
  </div>

  <div class="sec">
    <div class="sec-title">Timezone</div>
    <div class="fg">
      <label>Region</label>
      <select id="timezone">
        <option value="UTC">UTC (Coordinated Universal Time)</option>
        <option value="GMT">GMT / BST &mdash; London, Dublin</option>
        <option value="CET">CET / CEST &mdash; Paris, Berlin, Rome, Madrid</option>
        <option value="EET">EET / EEST &mdash; Athens, Helsinki, Bucharest</option>
        <option value="MSK">MSK &mdash; Moscow, Istanbul (UTC+3, no DST)</option>
        <option value="EST">EST / EDT &mdash; New York, Toronto (Eastern US)</option>
        <option value="CST">CST / CDT &mdash; Chicago, Winnipeg (Central US)</option>
        <option value="MST">MST / MDT &mdash; Denver, Calgary (Mountain US)</option>
        <option value="PST">PST / PDT &mdash; Los Angeles, Vancouver (Pacific US)</option>
        <option value="IST">IST &mdash; India (UTC+5:30, no DST)</option>
        <option value="CNST">CST &mdash; China (UTC+8, no DST)</option>
        <option value="HKT">HKT &mdash; Hong Kong (UTC+8, no DST)</option>
        <option value="SGT">SGT &mdash; Singapore, Kuala Lumpur (UTC+8, no DST)</option>
        <option value="JST">JST &mdash; Japan (UTC+9, no DST)</option>
        <option value="KST">KST &mdash; Korea (UTC+9, no DST)</option>
        <option value="AEST">AEST / AEDT &mdash; Sydney, Melbourne (Eastern AU)</option>
        <option value="ACST">ACST / ACDT &mdash; Adelaide (Central AU)</option>
        <option value="AWST">AWST &mdash; Perth (UTC+8, no DST)</option>
        <option value="NZST">NZST / NZDT &mdash; New Zealand</option>
      </select>
    </div>
    <div class="brow" style="margin-top:8px;">
      <button class="prim" onclick="setTimezone()" style="flex:2;">Save Timezone</button>
      <button onclick="detectTZ()" style="flex:1;">Auto Detect</button>
    </div>
    <small style="margin-top:6px;">DST transitions are handled automatically for zones that observe them.</small>
  </div>

</div>
<script>
  // ── Theme ──────────────────────────────────────────────────────────────────
  function toggleTheme() {
    var cur = document.documentElement.getAttribute('data-theme');
    var next = cur === 'dark' ? 'light' : 'dark';
    document.documentElement.setAttribute('data-theme', next);
    localStorage.setItem('iot-theme', next);
  }

  // ── Toast ──────────────────────────────────────────────────────────────────
  var _tt;
  function toast(msg, ms) {
    var t = document.getElementById('toast');
    t.textContent = msg;
    t.classList.add('show');
    clearTimeout(_tt);
    _tt = setTimeout(function(){ t.classList.remove('show'); }, ms || 2500);
  }

  // ── Clock ──────────────────────────────────────────────────────────────────
  var clockBase = null, clockAt = 0;
  var evtSrc = null;

  function parseSrvTime(s) {
    if (!s || s === 'Not synced') return null;
    var d = new Date(s.replace(' ','T'));
    return isNaN(d.getTime()) ? null : d;
  }
  function fmtTime(d) {
    if (!d || isNaN(d.getTime())) return 'Not synced';
    function p(n){ return String(n).padStart(2,'0'); }
    return d.getFullYear()+'-'+p(d.getMonth()+1)+'-'+p(d.getDate())+
           ' '+p(d.getHours())+':'+p(d.getMinutes())+':'+p(d.getSeconds());
  }
  function tickClock() {
    var el = document.getElementById('cur-time');
    if (!el || !clockBase) return;
    el.textContent = fmtTime(new Date(clockBase.getTime() + Date.now() - clockAt));
  }

  // ── Status rendering ───────────────────────────────────────────────────────
  function si(label, val) {
    return '<div class="si"><div class="sl">'+label+'</div><div class="sv">'+val+'</div></div>';
  }
  function setDim(onId, offId, state) {
    var on = document.getElementById(onId), off = document.getElementById(offId);
    if (on && off) { on.classList.toggle('bd',!state); off.classList.toggle('bd',state); }
  }

  function renderStatus(d) {
    var parsed = parseSrvTime(d.time);
    if (parsed) { clockBase = parsed; clockAt = Date.now(); }

    var jc = d.powerJack ? 's-on' : 's-off';
    var uc = d.usbOutput ? 's-on' : 's-off';

    document.getElementById('statusGrid').innerHTML =
      si('Power Jack','<span class="'+jc+'">'+(d.powerJack?'ON':'OFF')+'</span>')+
      si('USB Output','<span class="'+uc+'">'+(d.usbOutput?'ON':'OFF')+'</span>')+
      si('VBUS', d.vbus.toFixed(2)+' V')+
      si('WiFi', d.wifi)+
      si('IP', d.ip)+
      si('Timezone', d.timezone)+
      '<div class="si"><div class="sl">Time</div><div class="sv" id="cur-time">'+d.time+'</div></div>'+
      si('PD', d.pdVoltage+' V')+
      si('Schedules', d.schedules);

    loadSchedules();
    tickClock();

    setDim('jack-on','jack-off', d.powerJack);
    setDim('usb-on','usb-off',  d.usbOutput);
    [5,9,12,15,20].forEach(function(v){
      var b = document.getElementById('pd-'+v);
      if (b) b.classList.toggle('pa', d.pdVoltage === v);
    });

    var se = document.getElementById('wifiSSID');
    if (se && d.ssid) se.placeholder = d.ssid;

    // Sync timezone dropdown to current stored value
    var tzSel = document.getElementById('timezone');
    if (tzSel && d.timezone) {
      for (var i = 0; i < tzSel.options.length; i++) {
        if (tzSel.options[i].value === d.timezone) {
          tzSel.selectedIndex = i; break;
        }
      }
    }
  }

  function loadStatus() {
    fetch('/api/status').then(function(r){ return r.json(); })
      .then(renderStatus)
      .catch(function(){});
  }

  function connectEvents() {
    if (typeof EventSource === 'undefined') return;
    if (evtSrc) { evtSrc.close(); evtSrc = null; }
    evtSrc = new EventSource('/api/events');
    evtSrc.addEventListener('status', function(e){
      try { renderStatus(JSON.parse(e.data)); } catch(_){}
    });
    evtSrc.onerror = function(){
      if (evtSrc) { evtSrc.close(); evtSrc = null; }
      setTimeout(connectEvents, 3000);
    };
  }

  // ── Schedules ──────────────────────────────────────────────────────────────
  var TGT_LABEL = ['Both','Jack','USB'];

  function loadSchedules() {
    fetch('/api/schedules').then(function(r){ return r.json(); })
      .then(function(data){
        var el = document.getElementById('scheduleList');
        if (!data.schedules.length) {
          el.innerHTML = '<p style="color:var(--text2);font-size:.84em;">No schedules configured.</p>';
          return;
        }
        el.innerHTML = data.schedules.map(function(s, i){
          var tgt = TGT_LABEL[s.target] || 'Both';
          return '<div class="si-row">'+
            '<div>'+
              '<span class="st">'+s.time+'</span>'+
              '<span class="sa a-'+s.action.toLowerCase()+'">'+s.action+'</span>'+
              '<span class="stgt">'+tgt+'</span>'+
            '</div>'+
            '<button class="sdel" onclick="removeSchedule('+i+')">Remove</button>'+
          '</div>';
        }).join('');
      });
  }

  function addSchedule() {
    var tv = document.getElementById('schedTime').value;
    if (!tv) { toast('Please select a time'); return; }
    var p = tv.split(':');
    var target = parseInt(document.getElementById('schedTarget').value);
    var action = parseInt(document.getElementById('schedAction').value);
    fetch('/api/schedule',{method:'POST',headers:{'Content-Type':'application/json'},
      body:JSON.stringify({time:p[0]+p[1], action:action, target:target})})
      .then(function(){ document.getElementById('schedTime').value=''; loadStatus(); });
  }

  function removeSchedule(i) {
    fetch('/api/schedule/'+i,{method:'DELETE'}).then(loadStatus);
  }

  // ── Power / PD controls ────────────────────────────────────────────────────
  function setPowerJack(state) {
    fetch('/api/powerjack',{method:'POST',headers:{'Content-Type':'application/json'},
      body:JSON.stringify({state:state})}).then(loadStatus);
  }
  function setUSBOutput(state) {
    fetch('/api/usboutput',{method:'POST',headers:{'Content-Type':'application/json'},
      body:JSON.stringify({state:state})}).then(loadStatus);
  }
  function setPD(v) {
    fetch('/api/pd',{method:'POST',headers:{'Content-Type':'application/json'},
      body:JSON.stringify({voltage:v})})
      .then(function(){ toast('PD set to '+v+' V'); loadStatus(); });
  }

  // ── WiFi / Timezone ────────────────────────────────────────────────────────
  function setWiFi() {
    var ssid = document.getElementById('wifiSSID').value;
    if (!ssid) { toast('Please enter SSID'); return; }
    fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},
      body:JSON.stringify({ssid:ssid,password:document.getElementById('wifiPass').value})})
      .then(function(){ toast('WiFi saved. Restarting…',4000); });
  }

  function setTimezone() {
    var tz = document.getElementById('timezone').value;
    fetch('/api/timezone',{method:'POST',headers:{'Content-Type':'application/json'},
      body:JSON.stringify({timezone:tz})})
      .then(function(){ toast('Timezone saved'); loadStatus(); });
  }

  // IANA timezone name → our stored code
  var IANA_MAP = {
    'UTC':'UTC','Etc/UTC':'UTC','Etc/GMT':'UTC',
    'Europe/London':'GMT','Europe/Dublin':'GMT','Europe/Lisbon':'GMT',
    'Europe/Paris':'CET','Europe/Berlin':'CET','Europe/Rome':'CET',
    'Europe/Madrid':'CET','Europe/Brussels':'CET','Europe/Amsterdam':'CET',
    'Europe/Vienna':'CET','Europe/Zurich':'CET','Europe/Warsaw':'CET',
    'Europe/Prague':'CET','Europe/Budapest':'CET','Europe/Stockholm':'CET',
    'Europe/Athens':'EET','Europe/Helsinki':'EET','Europe/Bucharest':'EET',
    'Europe/Riga':'EET','Europe/Vilnius':'EET','Europe/Tallinn':'EET',
    'Europe/Moscow':'MSK','Europe/Istanbul':'MSK','Europe/Minsk':'MSK',
    'America/New_York':'EST','America/Toronto':'EST','America/Detroit':'EST',
    'America/Boston':'EST','America/Montreal':'EST',
    'America/Chicago':'CST','America/Winnipeg':'CST','America/Mexico_City':'CST',
    'America/Denver':'MST','America/Boise':'MST','America/Calgary':'MST',
    'America/Los_Angeles':'PST','America/Vancouver':'PST','America/Seattle':'PST',
    'Asia/Kolkata':'IST','Asia/Calcutta':'IST','Asia/Colombo':'IST',
    'Asia/Shanghai':'CNST','Asia/Chongqing':'CNST','Asia/Urumqi':'CNST',
    'Asia/Hong_Kong':'HKT',
    'Asia/Singapore':'SGT','Asia/Kuala_Lumpur':'SGT',
    'Asia/Tokyo':'JST',
    'Asia/Seoul':'KST',
    'Australia/Sydney':'AEST','Australia/Melbourne':'AEST','Australia/Brisbane':'AEST',
    'Australia/Adelaide':'ACST',
    'Australia/Perth':'AWST',
    'Pacific/Auckland':'NZST','Pacific/Wellington':'NZST',
  };

  function detectTZ() {
    var iana = (Intl && Intl.DateTimeFormat)
      ? Intl.DateTimeFormat().resolvedOptions().timeZone
      : null;
    if (!iana) { toast('Browser does not support timezone detection'); return; }
    var code = IANA_MAP[iana];
    if (code) {
      document.getElementById('timezone').value = code;
      toast('Detected: ' + iana);
    } else {
      toast(iana + ' not in list — set manually');
    }
  }

  // ── Init ───────────────────────────────────────────────────────────────────
  setInterval(loadStatus, 15000);
  setInterval(tickClock, 1000);
  connectEvents();
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
  addDashboardCorsHeaders();
  server.send(200, "application/json", buildStatusJson());
}

void handleDeviceInfo() {
  String json = "{\"protocol\":\"esp-iot/1\",\"id\":\"";
  json += getDeviceId();
  json += "\",\"name\":\"Power Switch ";
  String hostname = getDeviceHostname();
  json += hostname.substring(hostname.length() - 6);
  json += "\",\"type\":\"power-switch\",\"model\":\"ESP8266-IOT-Switch\",";
  json += "\"firmware\":\"3.1\",\"hostname\":\"";
  json += hostname;
  json += "\",\"ui\":\"/\",\"status\":\"/api/status\",";
  json += "\"capabilities\":[\"power-jack\",\"usb-output\",\"pd-voltage\",\"schedules\"]}";
  addDashboardCorsHeaders();
  server.send(200, "application/json", json);
}

void handleGetSchedules() {
  char json[640];
  int pos = snprintf(json, sizeof(json), "{\"schedules\":[");
  for (int i = 0; i < config.scheduleCount && pos < (int)sizeof(json) - 60; i++) {
    uint16_t t   = config.schedules[i].time;
    uint8_t  st  = config.schedules[i].action & 0x01;
    uint8_t  tgt = (config.schedules[i].action >> 1) & 0x03;
    pos += snprintf(json + pos, sizeof(json) - pos,
      "%s{\"time\":\"%02d:%02d\",\"action\":\"%s\",\"target\":%d}",
      i > 0 ? "," : "",
      t / 100, t % 100,
      st ? "ON" : "OFF",
      tgt
    );
  }
  snprintf(json + pos, sizeof(json) - pos, "]}");
  server.send(200, "application/json", json);
}

void handleSetPowerJack() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    bool state = body.indexOf("true") > 0;
    setPowerJackState(state);
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing body\"}");
  }
}

void handleSetUSBOutput() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    bool state = body.indexOf("true") > 0;
    setUSBOutputState(state);
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing body\"}");
  }
}

void handleSetPD() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    int voltage = 9;
    if (body.indexOf("\"voltage\":5") > 0) voltage = 5;
    else if (body.indexOf("\"voltage\":9") > 0) voltage = 9;
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
    uint16_t schedTime = body.substring(timeIdx, timeEnd).toInt();

    // Parse action (on/off)
    int actionIdx = body.indexOf("\"action\":") + 9;
    uint8_t state = body.substring(actionIdx, actionIdx + 1).toInt();

    // Parse target (0=both, 1=jack, 2=usb); defaults to 0 if absent
    int targetIdx = body.indexOf("\"target\":") + 9;
    uint8_t target = 0;
    if (targetIdx >= 9 && targetIdx < (int)body.length()) {
      target = body.substring(targetIdx, targetIdx + 1).toInt();
      if (target > 2) target = 0;
    }

    // Pack: bits[0]=state, bits[2:1]=target — backward-compatible with old schedules
    uint8_t packed = (target << 1) | state;

    if (config.scheduleCount < 10) {
      config.schedules[config.scheduleCount].time   = schedTime;
      config.schedules[config.scheduleCount].action = packed;
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

    int ssidIdx = body.indexOf("\"ssid\":\"") + 8;
    int ssidEnd = body.indexOf("\"", ssidIdx);
    String ssid = body.substring(ssidIdx, ssidEnd);

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
    tz.toUpperCase();

    tz.toCharArray(config.timezone, 8);
    saveConfig();

    if (wifiConnected) updateTime();

    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing body\"}");
  }
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/api/device",    HTTP_GET,  handleDeviceInfo);
  server.on("/api/device",    HTTP_OPTIONS, handleDashboardOptions);
  server.on("/api/status",    HTTP_GET,  handleStatus);
  server.on("/api/status",    HTTP_OPTIONS, handleDashboardOptions);
  server.on("/api/events",    HTTP_GET,  handleEvents);
  server.on("/api/schedules", HTTP_GET,  handleGetSchedules);
  server.on("/api/powerjack", HTTP_POST, handleSetPowerJack);
  server.on("/api/usboutput", HTTP_POST, handleSetUSBOutput);
  server.on("/api/pd",        HTTP_POST, handleSetPD);
  server.on("/api/schedule",  HTTP_POST, handleAddSchedule);
  server.on("/api/timezone",  HTTP_POST, handleSetTimezone);
  server.on("/api/wifi",      HTTP_POST, handleSetWiFi);

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
  sendSseStatusIfNeeded(false);
}
