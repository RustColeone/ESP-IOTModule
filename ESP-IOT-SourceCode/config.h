#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <NetworkInterface.h>
#include <NetworkEvents.h>
#include <time.h>

// ============================================================================
// CONFIGURATION
// ============================================================================
static const uint32_t BAUD = 115200;

// Pin definitions - ESP32-C6
#define BUTTON1_PIN 4        // GPIO4 - Button 1 (internal pullup)
#define BUTTON2_PIN 5        // GPIO5 - Button 2 (internal pullup)
#define BUTTON3_PIN 6        // GPIO6 - Button 3 (internal pullup)
#define BUTTON4_PIN 7        // GPIO7 - Button 4 (internal pullup)

// CH224K PD voltage control pins
#define CFG1_PIN 18          // GPIO18 (SDIO_CLK) - CH224K CFG1
#define CFG2_PIN 19          // GPIO19 (SDIO_DATA0) - CH224K CFG2
#define CFG3_PIN 20          // GPIO20 (SDIO_DATA1) - CH224K CFG3

// ADC pins for voltage sensing
#define VBUS_ADC_PIN 2       // GPIO2 - ADC for VBUS voltage sensing
#define VOUT_ADC_PIN 3       // GPIO3 - ADC for VOUT voltage sensing

// Output control pins
#define POWER_JACK_PIN 11    // GPIO11 - Power jack enable (HIGH=enable, LOW=disable)
#define USB_OUTPUT_PIN 10    // GPIO10 - USB output enable (LOW=enable, HIGH=disable)

// Timing
#define WIFI_RETRY_INTERVAL 60000      // 1 minute
#define BUTTON_DEBOUNCE 50
#define TIME_UPDATE_INTERVAL 3600000   // 1 hour

// ADC configuration for ESP32-C6
#define ADC_RESOLUTION 4095            // 12-bit ADC
#define ADC_VREF 3.3                   // Reference voltage
#define VBUS_DIVIDER_RATIO 10.216      // (47k+5.1k)/5.1k = 52.1/5.1 = 10.216
#define VOUT_DIVIDER_RATIO 10.216      // (47k+5.1k)/5.1k = 52.1/5.1 = 10.216

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
#define ADDR_SCHEDULES 143       // 10 schedules * 3 bytes each = 30 bytes
#define ADDR_POWER_JACK_STATE 173
#define ADDR_USB_OUTPUT_STATE 174

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
  uint8_t pdVoltage;       // 5, 9, 12, 15, or 20
  uint8_t scheduleCount;
  Schedule schedules[10];
  bool powerJackState;     // Power jack output state
  bool usbOutputState;     // USB output state
};

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================
extern Config config;
extern bool powerJackState;
extern bool usbOutputState;
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
