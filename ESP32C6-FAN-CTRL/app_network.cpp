#include "app_network.h"
#include <WiFi.h>
#include <time.h>
#include <stdlib.h>

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
    Serial.print('.');
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println();
    Serial.print(F("WiFi connected. IP: "));
    Serial.println(WiFi.localIP());
    updateTime();
  } else {
    wifiConnected = false;
    Serial.println();
    Serial.println(F("WiFi connection failed."));
  }
}

// Convert stored timezone code ("UTC+8", "PST", ...) to a POSIX TZ string.
static const char* tzToPosix(const char* tz) {
  String t = String(tz);
  t.toUpperCase();

  if (t == "UTC" || t == "GMT")         return "GMT0";
  if (t == "EST" || t == "EDT")         return "EST5EDT,M3.2.0,M11.1.0";
  if (t == "CST" || t == "CDT")         return "CST6CDT,M3.2.0,M11.1.0";
  if (t == "MST" || t == "MDT")         return "MST7MDT,M3.2.0,M11.1.0";
  if (t == "PST" || t == "PDT")         return "PST8PDT,M3.2.0,M11.1.0";
  if (t == "CET" || t == "CEST")        return "CET-1CEST,M3.5.0,M10.5.0/3";
  if (t == "JST")                       return "JST-9";
  if (t == "KST")                       return "KST-9";
  if (t == "HKT")                       return "HKT-8";
  if (t == "CNST" || t == "CST8")       return "CST-8";
  if (t == "SGT")                       return "SGT-8";
  if (t == "IST")                       return "IST-5:30";
  if (t == "AEST" || t == "AEDT")       return "AEST-10AEDT,M10.1.0,M4.1.0/3";
  if (t == "NZST" || t == "NZDT")       return "NZST-12NZDT,M9.5.0,M4.1.0/3";
  return nullptr;
}

void updateTime() {
  if (!wifiConnected) return;

  char posix[40] = "GMT0";
  String t = String(config.timezone);
  t.toUpperCase();

  if (t.startsWith("UTC") && t.length() > 3) {
    // POSIX sign is inverted from UTC notation: UTC+8 -> "UTC-8"
    int off = t.substring(3).toInt();
    snprintf(posix, sizeof(posix), "UTC%+d", -off);
  } else {
    const char* p = tzToPosix(config.timezone);
    if (p) strncpy(posix, p, sizeof(posix) - 1);
  }

  setenv("TZ", posix, 1);
  tzset();
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  int attempts = 0;
  while (time(nullptr) < 100000 && attempts < 20) {
    delay(500);
    attempts++;
  }

  currentTime = time(nullptr);
  if (currentTime > 100000) {
    Serial.println(F("Time synchronized."));
  } else {
    Serial.println(F("NTP failed."));
  }
}
