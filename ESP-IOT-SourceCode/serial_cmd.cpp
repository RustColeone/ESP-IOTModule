#include "serial_cmd.h"
#include "hardware.h"
#include "network.h"
#include "storage.h"
#include <ESP8266WiFi.h>

void printHelp() {
  Serial.println(F("\n========== IOT SWITCH HELP =========="));
  Serial.println(F("/help - Show this help manual"));
  Serial.println(F("\n--- WiFi & Time ---"));
  Serial.println(F("/wifi <SSID> <PASSWORD> - Configure WiFi credentials"));
  Serial.println(F("/timezone <CODE> - Set timezone"));
  Serial.println(F("  Formats: UTC+X, UTC-X (e.g., UTC+8, UTC-5)"));
  Serial.println(F("  Named: UTC (UTC+0), GMT (UTC+0)"));
  Serial.println(F("  US: EST (UTC-5), EDT (UTC-4), CST (UTC-6), CDT (UTC-5)"));
  Serial.println(F("      MST (UTC-7), MDT (UTC-6), PST (UTC-8), PDT (UTC-7)"));
  Serial.println(F("  Asia: JST (UTC+9), KST (UTC+9), HKT (UTC+8), CNST (UTC+8)"));
  Serial.println(F("  EU: CET (UTC+1), CEST (UTC+2)"));
  Serial.println(F("  Pacific: AEST (UTC+10), AEDT (UTC+11), NZST (UTC+12)"));
  Serial.println(F("\n--- Power Control ---"));
  Serial.println(F("/on - Turn power ON"));
  Serial.println(F("/off - Turn power OFF"));
  Serial.println(F("/pd <voltage> - Set PD voltage (9, 12, 15, or 20)"));
  Serial.println(F("\n--- Scheduling ---"));
  Serial.println(F("/do_at <HHMM> <on|off> - Add scheduled action (24hr format)"));
  Serial.println(F("  Example: /do_at 2315 on"));
  Serial.println(F("/do_list - List all scheduled actions"));
  Serial.println(F("/do_remove_at <index> - Remove schedule at index (use -1 for all)"));
  Serial.println(F("\n--- Status ---"));
  Serial.println(F("/status - Show system status"));
  Serial.println(F("\n--- Web Interface ---"));
  Serial.print(F("Access the web UI at: http://"));
  Serial.println(WiFi.localIP());
  Serial.println(F("\n=====================================\n"));
}

void handleWiFiCmd(String args) {
  int spaceIdx = args.indexOf(' ');
  if (spaceIdx == -1) {
    Serial.println(F("ERR: Usage: /wifi <SSID> <PASSWORD>"));
    return;
  }
  
  String ssid = args.substring(0, spaceIdx);
  String password = args.substring(spaceIdx + 1);
  
  ssid.trim();
  password.trim();
  
  if (ssid.length() == 0 || ssid.length() >= 64) {
    Serial.println(F("ERR: Invalid SSID length."));
    return;
  }
  
  if (password.length() >= 64) {
    Serial.println(F("ERR: Password too long."));
    return;
  }
  
  ssid.toCharArray(config.ssid, 64);
  password.toCharArray(config.password, 64);
  saveConfig();
  
  Serial.println(F("WiFi credentials saved. Connecting..."));
  connectWiFi();
}

void handleTimezoneCmd(String args) {
  args.trim();
  args.toUpperCase();
  
  if (args.length() == 0 || args.length() >= 8) {
    Serial.println(F("ERR: Invalid timezone code."));
    return;
  }
  
  args.toCharArray(config.timezone, 8);
  saveConfig();
  
  Serial.print(F("Timezone set to: "));
  Serial.println(config.timezone);
  
  if (wifiConnected) {
    updateTime();
  }
}

void handleDoAtCmd(String args) {
  int spaceIdx = args.indexOf(' ');
  if (spaceIdx == -1) {
    Serial.println(F("ERR: Usage: /do_at <HHMM> <on|off>"));
    return;
  }
  
  String timeStr = args.substring(0, spaceIdx);
  String actionStr = args.substring(spaceIdx + 1);
  
  timeStr.trim();
  actionStr.trim();
  actionStr.toLowerCase();
  
  uint16_t schedTime = timeStr.toInt();
  if (schedTime > 2359 || (schedTime % 100) > 59) {
    Serial.println(F("ERR: Invalid time format. Use HHMM (0000-2359)."));
    return;
  }
  
  uint8_t action;
  if (actionStr == "on" || actionStr == "1") {
    action = 1;
  } else if (actionStr == "off" || actionStr == "0") {
    action = 0;
  } else {
    Serial.println(F("ERR: Action must be 'on' or 'off'."));
    return;
  }
  
  if (config.scheduleCount >= 10) {
    Serial.println(F("ERR: Schedule list full (max 10 entries)."));
    return;
  }
  
  config.schedules[config.scheduleCount].time = schedTime;
  config.schedules[config.scheduleCount].action = action;
  config.scheduleCount++;
  saveConfig();
  
  Serial.print(F("Schedule added: "));
  Serial.print(schedTime);
  Serial.print(F(" -> "));
  Serial.println(action ? F("ON") : F("OFF"));
}

void handleDoListCmd() {
  Serial.println(F("\n--- Scheduled Actions ---"));
  if (config.scheduleCount == 0) {
    Serial.println(F("No schedules configured."));
  } else {
    for (int i = 0; i < config.scheduleCount; i++) {
      Serial.print(i);
      Serial.print(F(": "));
      
      uint16_t t = config.schedules[i].time;
      uint8_t hours = t / 100;
      uint8_t mins = t % 100;
      
      if (hours < 10) Serial.print('0');
      Serial.print(hours);
      Serial.print(':');
      if (mins < 10) Serial.print('0');
      Serial.print(mins);
      
      Serial.print(F(" -> "));
      Serial.println(config.schedules[i].action ? F("ON") : F("OFF"));
    }
  }
  Serial.println(F("-------------------------\n"));
}

void handleDoRemoveAtCmd(String args) {
  args.trim();
  int index = args.toInt();
  
  if (index == -1) {
    config.scheduleCount = 0;
    saveConfig();
    Serial.println(F("All schedules cleared."));
    return;
  }
  
  if (index < 0 || index >= config.scheduleCount) {
    Serial.println(F("ERR: Invalid schedule index."));
    return;
  }
  
  for (int i = index; i < config.scheduleCount - 1; i++) {
    config.schedules[i] = config.schedules[i + 1];
  }
  config.scheduleCount--;
  saveConfig();
  
  Serial.print(F("Schedule "));
  Serial.print(index);
  Serial.println(F(" removed."));
}

void handleStatusCmd() {
  Serial.println(F("\n========== SYSTEM STATUS =========="));
  
  Serial.print(F("Power State: "));
  Serial.println(powerState ? F("ON") : F("OFF"));
  
  float vbus = getVBusVoltage();
  Serial.print(F("VBUS Voltage: "));
  Serial.print(vbus, 2);
  Serial.println(F("V"));
  
  Serial.print(F("WiFi SSID: "));
  if (strlen(config.ssid) > 0) {
    Serial.println(config.ssid);
  } else {
    Serial.println(F("Not configured"));
  }
  
  Serial.print(F("WiFi Status: "));
  if (wifiConnected) {
    Serial.print(F("Connected ("));
    Serial.print(WiFi.localIP());
    Serial.println(F(")"));
  } else {
    Serial.println(F("Disconnected"));
  }
  
  Serial.print(F("Timezone: "));
  Serial.println(config.timezone);
  
  if (currentTime > 100000) {
    Serial.print(F("Current Time: "));
    struct tm *timeinfo = localtime(&currentTime);
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    Serial.println(buffer);
  } else {
    Serial.println(F("Current Time: Not synchronized"));
  }
  
  Serial.print(F("PD Voltage Setting: "));
  Serial.print(config.pdVoltage);
  Serial.println(F("V"));
  
  Serial.print(F("Scheduled Actions: "));
  Serial.print(config.scheduleCount);
  Serial.println(F(" configured"));
  
  Serial.println(F("===================================\n"));
}

void handleSerialCommand() {
  if (!Serial.available()) return;
  
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;
  
  int spaceIdx = line.indexOf(' ');
  String cmd = (spaceIdx == -1) ? line : line.substring(0, spaceIdx);
  String args = (spaceIdx == -1) ? "" : line.substring(spaceIdx + 1);
  
  cmd.toLowerCase();
  args.trim();
  
  if (cmd == "/help") {
    printHelp();
  } else if (cmd == "/wifi") {
    handleWiFiCmd(args);
  } else if (cmd == "/timezone") {
    handleTimezoneCmd(args);
  } else if (cmd == "/on") {
    setPowerState(true);
  } else if (cmd == "/off") {
    setPowerState(false);
  } else if (cmd == "/pd") {
    uint8_t voltage = args.toInt();
    setPDVoltage(voltage);
  } else if (cmd == "/do_at") {
    handleDoAtCmd(args);
  } else if (cmd == "/do_list") {
    handleDoListCmd();
  } else if (cmd == "/do_remove_at") {
    handleDoRemoveAtCmd(args);
  } else if (cmd == "/status") {
    handleStatusCmd();
  } else {
    Serial.println(F("ERR: Unknown command. Type /help for available commands."));
  }
}
