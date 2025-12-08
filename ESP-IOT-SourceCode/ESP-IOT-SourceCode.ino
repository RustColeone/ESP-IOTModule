// ============================================================================
// ESP8266 IOT SWITCH v2.0 - Main File
// Modular architecture with web interface and serial commands
// ============================================================================

#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"
#include "storage.h"
#include "hardware.h"
#include "network.h"
#include "scheduler.h"
#include "webserver.h"
#include "serial_cmd.h"

// ============================================================================
// GLOBAL VARIABLES DEFINITION
// ============================================================================
Config config;
bool powerState = false;
unsigned long lastWifiAttempt = 0;
unsigned long lastTimeUpdate = 0;
unsigned long lastButtonCheck = 0;
bool wifiConnected = false;
time_t currentTime = 0;

// Button states
bool lastButton1 = HIGH;
bool lastButton2 = HIGH;
bool lastButton3 = HIGH;
bool lastButton4 = HIGH;

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(BAUD);
  Serial.setTimeout(100);
  delay(100);
  
  Serial.println(F("\n\n========================================"));
  Serial.println(F("    ESP8266 IOT SWITCH v2.0"));
  Serial.println(F("    Modular + Web Interface"));
  Serial.println(F("========================================\n"));
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  loadConfig();
  
  // Initialize pins
  pinMode(POWER_SWITCH_PIN, OUTPUT);
  digitalWrite(POWER_SWITCH_PIN, LOW);
  
  pinMode(CFG2_PIN, OUTPUT);
  pinMode(CFG3_PIN, OUTPUT);
  
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  pinMode(BUTTON4_PIN, INPUT_PULLUP);
  
  // Set PD voltage from config
  setPDVoltage(config.pdVoltage);
  
  // Try to connect to WiFi if configured
  if (strlen(config.ssid) > 0) {
    connectWiFi();
    if (wifiConnected) {
      setupWebServer();
    }
  }
  
  // Initialize time from saved value
  if (config.lastTime > 0) {
    currentTime = config.lastTime;
  }
  
  printHelp();
  Serial.println(F("Ready. Type /help for commands.\n"));
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
  unsigned long now = millis();
  
  // Update current time
  if (currentTime > 0) {
    static unsigned long lastMillis = 0;
    if (lastMillis == 0) lastMillis = now;
    if (now - lastMillis >= 1000) {
      currentTime += (now - lastMillis) / 1000;
      lastMillis = now;
      config.lastTime = currentTime;
    }
  }
  
  // WiFi reconnection
  if (!wifiConnected && strlen(config.ssid) > 0) {
    if (now - lastWifiAttempt >= WIFI_RETRY_INTERVAL) {
      lastWifiAttempt = now;
      Serial.println(F("Retrying WiFi connection..."));
      connectWiFi();
      if (wifiConnected) {
        setupWebServer();
      }
    }
  }
  
  // Time update
  if (wifiConnected && now - lastTimeUpdate >= TIME_UPDATE_INTERVAL) {
    lastTimeUpdate = now;
    updateTime();
  }
  
  // Check buttons
  checkButtons();
  
  // Check schedules
  checkSchedules();
  
  // Handle web clients
  if (wifiConnected) {
    handleWebClient();
  }
  
  // Handle serial commands
  handleSerialCommand();
  
  // Small delay to prevent watchdog issues
  delay(10);
}
