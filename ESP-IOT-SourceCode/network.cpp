#include "network.h"
#include "storage.h"
#include <ESP8266WiFi.h>
#include <time.h>

void connectWiFi() {
  if (strlen(config.ssid) == 0) {
    Serial.println(F("No WiFi credentials configured."));
    return;
  }
  
  Serial.print(F("Connecting to WiFi: "));
  Serial.println(config.ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.ssid, config.password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println(F("\nWiFi connected!"));
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());
    updateTime();
  } else {
    wifiConnected = false;
    Serial.println(F("\nWiFi connection failed."));
  }
}

void updateTime() {
  if (!wifiConnected) return;
  
  // Configure NTP with timezone offset
  int tzOffset = 0;
  String tz = String(config.timezone);
  tz.toUpperCase();
  
  // Check for UTC+X or UTC-X format
  if (tz.startsWith("UTC")) {
    if (tz.length() > 3) {
      String offsetStr = tz.substring(3);
      tzOffset = offsetStr.toInt();
    } else {
      tzOffset = 0;
    }
  }
  // Named timezones
  else if (tz == "EST") tzOffset = -5;
  else if (tz == "EDT") tzOffset = -4;
  else if (tz == "CST") tzOffset = -6;
  else if (tz == "CDT") tzOffset = -5;
  else if (tz == "MST") tzOffset = -7;
  else if (tz == "MDT") tzOffset = -6;
  else if (tz == "PST") tzOffset = -8;
  else if (tz == "PDT") tzOffset = -7;
  else if (tz == "GMT") tzOffset = 0;
  else if (tz == "CET") tzOffset = 1;
  else if (tz == "CEST") tzOffset = 2;
  else if (tz == "JST") tzOffset = 9;
  else if (tz == "KST") tzOffset = 9;
  else if (tz == "CNST") tzOffset = 8;
  else if (tz == "AEST") tzOffset = 10;
  else if (tz == "AEDT") tzOffset = 11;
  else if (tz == "NZST") tzOffset = 12;
  else if (tz == "NZDT") tzOffset = 13;
  else if (tz == "IST") tzOffset = 5;
  else if (tz == "HKT") tzOffset = 8;
  else {
    Serial.print(F("WARN: Unknown timezone '"));
    Serial.print(config.timezone);
    Serial.println(F("', using UTC+0"));
    tzOffset = 0;
  }
  
  configTime(tzOffset * 3600, 0, "pool.ntp.org", "time.nist.gov");
  
  Serial.print(F("Updating time (timezone: "));
  Serial.print(config.timezone);
  Serial.println(F(")..."));
  
  int attempts = 0;
  while (time(nullptr) < 100000 && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  
  currentTime = time(nullptr);
  if (currentTime > 100000) {
    config.lastTime = currentTime;
    saveConfig();
    Serial.println(F("Time synchronized!"));
  } else {
    Serial.println(F("Failed to get time from NTP."));
    if (config.lastTime > 0) {
      currentTime = config.lastTime;
      Serial.println(F("Using last saved time."));
    }
  }
}
