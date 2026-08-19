#include "serial_cmd.h"
#include "hardware.h"
#include "storage.h"
#include "app_network.h"
#include "app_webserver.h"
#include <WiFi.h>

void printHelp() {
  Serial.println(F("\n========== FAN CONTROLLER HELP =========="));
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
  Serial.println(F("\n--- Fan Control ---"));
  Serial.println(F("/fan <0-3> <0-100> - Set one fan's speed (%)"));
  Serial.println(F("/all <0-100> - Set all fans to the same speed (%)"));
  Serial.println(F("\n--- Status ---"));
  Serial.println(F("/status - Show system status"));
  Serial.println(F("\n--- Web Interface ---"));
  if (wifiConnected) {
    Serial.print(F("Access the web UI at: http://"));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("Not connected. Configure WiFi with /wifi <SSID> <PASSWORD>"));
  }
  Serial.println(F("\n===========================================\n"));
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

  if (ssid.length() == 0 || ssid.length() >= sizeof(config.ssid)) {
    Serial.println(F("ERR: Invalid SSID length."));
    return;
  }

  if (password.length() >= sizeof(config.password)) {
    Serial.println(F("ERR: Password too long."));
    return;
  }

  ssid.toCharArray(config.ssid, sizeof(config.ssid));
  password.toCharArray(config.password, sizeof(config.password));
  saveConfig();

  Serial.println(F("WiFi credentials saved. Connecting..."));
  connectWiFi();
  if (wifiConnected) setupWebServer();
}

void handleTimezoneCmd(String args) {
  args.trim();
  args.toUpperCase();

  if (args.length() == 0 || args.length() >= sizeof(config.timezone)) {
    Serial.println(F("ERR: Invalid timezone code."));
    return;
  }

  args.toCharArray(config.timezone, sizeof(config.timezone));
  saveConfig();

  Serial.print(F("Timezone set to: "));
  Serial.println(config.timezone);

  if (wifiConnected) {
    updateTime();
  }
}

void handleFanCmd(String args) {
  int spaceIdx = args.indexOf(' ');
  if (spaceIdx == -1) {
    Serial.println(F("ERR: Usage: /fan <0-3> <0-100>"));
    return;
  }

  String fanStr = args.substring(0, spaceIdx);
  String speedStr = args.substring(spaceIdx + 1);
  fanStr.trim();
  speedStr.trim();

  int fan = fanStr.toInt();
  int speed = speedStr.toInt();

  if (fan < 0 || fan >= FAN_COUNT) {
    Serial.println(F("ERR: Fan index must be 0-3."));
    return;
  }
  if (speed < 0 || speed > 100) {
    Serial.println(F("ERR: Speed must be 0-100."));
    return;
  }

  setFanSpeed(fan, speed);
  Serial.print(F("Fan "));
  Serial.print(fan);
  Serial.print(F(" set to "));
  Serial.print(speed);
  Serial.println(F("%"));
}

void handleAllCmd(String args) {
  args.trim();
  int speed = args.toInt();

  if (speed < 0 || speed > 100) {
    Serial.println(F("ERR: Speed must be 0-100."));
    return;
  }

  setAllFanSpeed(speed);
  Serial.print(F("All fans set to "));
  Serial.print(speed);
  Serial.println(F("%"));
}

void handleStatusCmd() {
  Serial.println(F("\n========== SYSTEM STATUS =========="));

  for (int i = 0; i < FAN_COUNT; i++) {
    Serial.printf("Fan %d: %3d%%  RPM: %lu\n", i, config.fanSpeed[i], (unsigned long)fanRpm[i]);
  }

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

  Serial.println(F("====================================\n"));
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
  } else if (cmd == "/fan") {
    handleFanCmd(args);
  } else if (cmd == "/all") {
    handleAllCmd(args);
  } else if (cmd == "/status") {
    handleStatusCmd();
  } else {
    Serial.println(F("ERR: Unknown command. Type /help for available commands."));
  }
}
