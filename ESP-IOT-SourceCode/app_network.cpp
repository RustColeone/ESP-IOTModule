#include "app_network.h"
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

// Maps stored timezone code to a POSIX TZ string.
// POSIX sign is inverted from UTC notation:
//   UTC+8 (east) → POSIX "HKT-8"
//   UTC-5 (west) → POSIX "EST5"
// DST rules encoded as Mm.w.d (month.week.weekday).
static const char* tzToPosix(const char* stored) {
  String tz = String(stored);
  tz.toUpperCase();

  // UTC+X / UTC-X numeric offset (no DST)
  if (tz.startsWith("UTC") && tz.length() > 3) {
    return nullptr; // handled by caller with dynamic buffer
  }

  if (tz == "UTC" || tz == "GMT0")               return "UTC0";
  // UK/Ireland: GMT winter, BST (+1) summer
  if (tz == "GMT")                               return "GMT0BST,M3.5.0/1,M10.5.0";
  // US Eastern: EST (UTC-5) / EDT (UTC-4)
  if (tz == "EST" || tz == "EDT")               return "EST5EDT,M3.2.0,M11.1.0";
  // US Central: CST (UTC-6) / CDT (UTC-5)
  if (tz == "CST" || tz == "CDT")               return "CST6CDT,M3.2.0,M11.1.0";
  // US Mountain: MST (UTC-7) / MDT (UTC-6)
  if (tz == "MST" || tz == "MDT")               return "MST7MDT,M3.2.0,M11.1.0";
  // US Pacific: PST (UTC-8) / PDT (UTC-7)
  if (tz == "PST" || tz == "PDT")               return "PST8PDT,M3.2.0,M11.1.0";
  // Central Europe: CET (UTC+1) / CEST (UTC+2)
  if (tz == "CET"  || tz == "CEST")             return "CET-1CEST,M3.5.0,M10.5.0/3";
  // Eastern Europe: EET (UTC+2) / EEST (UTC+3)
  if (tz == "EET"  || tz == "EEST")             return "EET-2EEST,M3.5.0/3,M10.5.0/4";
  // Moscow / Turkey: UTC+3, no DST since 2014/2016
  if (tz == "MSK"  || tz == "TRT")             return "MSK-3";
  // India: IST UTC+5:30, no DST
  if (tz == "IST")                              return "IST-5:30";
  // China: CST UTC+8, no DST
  if (tz == "CNST" || tz == "CST8")            return "CST-8";
  // Hong Kong: HKT UTC+8, no DST
  if (tz == "HKT")                             return "HKT-8";
  // Singapore/Malaysia: SGT UTC+8, no DST
  if (tz == "SGT"  || tz == "MYT")            return "SGT-8";
  // Japan: JST UTC+9, no DST
  if (tz == "JST")                             return "JST-9";
  // Korea: KST UTC+9, no DST
  if (tz == "KST")                             return "KST-9";
  // Eastern Australia: AEST (UTC+10) / AEDT (UTC+11)
  if (tz == "AEST" || tz == "AEDT")           return "AEST-10AEDT,M10.1.0,M4.1.0/3";
  // Central Australia: ACST (UTC+9:30) / ACDT (UTC+10:30)
  if (tz == "ACST" || tz == "ACDT")           return "ACST-9:30ACDT,M10.1.0,M4.1.0/3";
  // Western Australia: AWST UTC+8, no DST
  if (tz == "AWST")                           return "AWST-8";
  // New Zealand: NZST (UTC+12) / NZDT (UTC+13)
  if (tz == "NZST" || tz == "NZDT")          return "NZST-12NZDT,M9.5.0,M4.1.0/3";

  return nullptr; // unknown
}

void updateTime() {
  if (!wifiConnected) return;

  String tz = String(config.timezone);
  tz.toUpperCase();

  char posixBuf[40] = "UTC0";
  const char* posix = posixBuf;

  if (tz.startsWith("UTC") && tz.length() > 3) {
    // Numeric UTC offset: sign is inverted in POSIX format
    int off = tz.substring(3).toInt();
    if (off == 0) {
      strcpy(posixBuf, "UTC0");
    } else {
      // UTC+8 → POSIX "UTC-8"; UTC-5 → POSIX "UTC+5"
      snprintf(posixBuf, sizeof(posixBuf), "UTC%+d", -off);
    }
  } else {
    const char* p = tzToPosix(config.timezone);
    if (p) {
      posix = p;
    } else {
      Serial.print(F("WARN: Unknown timezone '"));
      Serial.print(config.timezone);
      Serial.println(F("', using UTC"));
    }
  }

  Serial.print(F("Timezone: "));
  Serial.print(config.timezone);
  Serial.print(F(" -> "));
  Serial.println(posix);

  configTime(posix, "pool.ntp.org", "time.nist.gov");

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
