// ============================================================================
// ESP32-C6 FAN CONTROLLER
// 4x 4-wire fans (PWM + tachometer) on a Seeed Studio XIAO ESP32C6
// ============================================================================
#include <Arduino.h>
#include "config.h"
#include "storage.h"
#include "hardware.h"
#include "app_network.h"
#include "app_webserver.h"
#include "serial_cmd.h"

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================
Config config;
bool wifiConnected = false;
time_t currentTime = 0;
unsigned long lastWifiAttempt = 0;

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(BAUD);
  Serial.setTimeout(50);
  delay(200);

  Serial.println(F("\n========================================"));
  Serial.println(F("    ESP32-C6 FAN CONTROLLER"));
  Serial.println(F("    4x PWM Fan Controller"));
  Serial.println(F("========================================\n"));

  loadConfig();
  initHardware();

  // Restore persisted fan speeds (without re-saving config).
  for (int i = 0; i < FAN_COUNT; i++) {
    applyFanSpeed(i, config.fanSpeed[i]);
  }

  if (strlen(config.ssid) > 0) {
    connectWiFi();
    if (wifiConnected) setupWebServer();
  }

  Serial.println(F("Ready.\n"));
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
  updateFanTelemetry();
  checkButtons();

  // WiFi reconnection
  if (!wifiConnected && strlen(config.ssid) > 0) {
    if (millis() - lastWifiAttempt >= WIFI_RETRY_INTERVAL) {
      lastWifiAttempt = millis();
      Serial.println(F("Retrying WiFi..."));
      connectWiFi();
      if (wifiConnected) setupWebServer();
    }
  }

  if (wifiConnected) {
    handleWebServer();

    static unsigned long lastTimeUpdate = 0;
    if (millis() - lastTimeUpdate >= TIME_UPDATE_INTERVAL) {
      lastTimeUpdate = millis();
      updateTime();
    }
  }

  handleSerialCommand();
}
