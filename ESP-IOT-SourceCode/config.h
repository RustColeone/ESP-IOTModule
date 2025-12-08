#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <time.h>

// ============================================================================
// CONFIGURATION
// ============================================================================
static const uint32_t BAUD = 115200;

// Pin definitions
#define POWER_SWITCH_PIN 5   // D1 (GPIO5) - Controls downstream device power
#define BUTTON1_PIN 4        // D2 (GPIO4)
#define BUTTON2_PIN 0        // D3 (GPIO0)
#define BUTTON3_PIN 2        // D4 (GPIO2)
#define BUTTON4_PIN 14       // D5 (GPIO14)
#define CFG2_PIN 12          // D6 (GPIO12) - CH224K CFG2
#define CFG3_PIN 13          // D7 (GPIO13) - CH224K CFG3
#define ANALOG_PIN A0        // Voltage sensing

// Timing
#define WIFI_RETRY_INTERVAL 60000      // 1 minute
#define BUTTON_DEBOUNCE 50
#define TIME_UPDATE_INTERVAL 3600000   // 1 hour

// EEPROM Layout
#define EEPROM_SIZE 512
#define EEPROM_MAGIC 0xAB
#define ADDR_MAGIC 0
#define ADDR_SSID 1
#define ADDR_PASSWORD 65
#define ADDR_TIMEZONE 129
#define ADDR_LAST_TIME 133
#define ADDR_PD_VOLTAGE 141
#define ADDR_SCHEDULE_COUNT 142
#define ADDR_SCHEDULES 143  // 10 schedules * 3 bytes each = 30 bytes

// ============================================================================
// DATA STRUCTURES
// ============================================================================
struct Schedule {
  uint16_t time;   // 0-2359 in 24hr format
  uint8_t action;  // 0=off, 1=on
};

struct Config {
  char ssid[64];
  char password[64];
  char timezone[8];
  time_t lastTime;
  uint8_t pdVoltage;  // 9, 12, 15, or 20
  uint8_t scheduleCount;
  Schedule schedules[10];
};

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================
extern Config config;
extern bool powerState;
extern unsigned long lastWifiAttempt;
extern unsigned long lastTimeUpdate;
extern unsigned long lastButtonCheck;
extern bool wifiConnected;
extern time_t currentTime;

// Button states
extern bool lastButton1;
extern bool lastButton2;
extern bool lastButton3;
extern bool lastButton4;

#endif
