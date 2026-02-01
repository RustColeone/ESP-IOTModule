// ============================================================================
// ESP32-C6 IOT SWITCH v3.0 - Main File
// Updated for ESP32-C6 with dual output and enhanced PD control
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
bool powerJackState = false;
bool usbOutputState = false;
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
  Serial.println(F("    ESP32-C6 IOT SWITCH v3.0"));
  Serial.println(F("    Dual Output + PD Control"));
  Serial.println(F("========================================\n"));
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  loadConfig();
  
  // Initialize output pins
  pinMode(POWER_JACK_PIN, OUTPUT);
  pinMode(USB_OUTPUT_PIN, OUTPUT);
  digitalWrite(POWER_JACK_PIN, LOW);
  digitalWrite(USB_OUTPUT_PIN, HIGH);  // HIGH = disabled (inverted)
  
  // Initialize CH224K CFG pins
  pinMode(CFG1_PIN, OUTPUT);
  pinMode(CFG2_PIN, OUTPUT);
  pinMode(CFG3_PIN, OUTPUT);
  
  // Initialize button pins with internal pullup
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  pinMode(BUTTON4_PIN, INPUT_PULLUP);
  
  // Initialize ADC pins
  pinMode(VBUS_ADC_PIN, INPUT);
  pinMode(VOUT_ADC_PIN, INPUT);
  
  // Set ADC resolution for ESP32-C6
  analogReadResolution(12);  // 12-bit resolution
  
  // Set PD voltage from config
  setPDVoltage(config.pdVoltage);
  
  // Restore output states from config
  setPowerJackState(config.powerJackState);
  setUSBOutputState(config.usbOutputState);
  
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
