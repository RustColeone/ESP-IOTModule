#include "storage.h"
#include <Preferences.h>

static Preferences prefs;

void loadConfig() {
  memset(&config, 0, sizeof(config));
  strcpy(config.timezone, "UTC+8");

  prefs.begin("fanctrl", true);  // read-only
  String s = prefs.getString("ssid", "");
  String p = prefs.getString("pass", "");
  String t = prefs.getString("tz", "UTC+8");
  s.toCharArray(config.ssid, sizeof(config.ssid));
  p.toCharArray(config.password, sizeof(config.password));
  t.toCharArray(config.timezone, sizeof(config.timezone));

  char key[8];
  for (int i = 0; i < FAN_COUNT; i++) {
    snprintf(key, sizeof(key), "fan%d", i);
    config.fanSpeed[i] = prefs.getUChar(key, 0);
  }
  prefs.end();

  Serial.println(F("Config loaded."));
}

void saveConfig() {
  prefs.begin("fanctrl", false);  // read-write
  prefs.putString("ssid", config.ssid);
  prefs.putString("pass", config.password);
  prefs.putString("tz", config.timezone);

  char key[8];
  for (int i = 0; i < FAN_COUNT; i++) {
    snprintf(key, sizeof(key), "fan%d", i);
    prefs.putUChar(key, config.fanSpeed[i]);
  }
  prefs.end();
}
