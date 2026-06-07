#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <time.h>

// ============================================================================
// CONFIGURATION
// ============================================================================
static const uint32_t BAUD = 115200;

// Pin definitions - ESP8266
// BOOT CONSTRAINTS (ESP8266 samples these before user code runs):
//   GPIO15 (D8) MUST be LOW  at boot — high level prevents boot entirely
//   GPIO2  (D4) MUST be HIGH at boot — also UART1 TX (74880 baud boot messages)
//   GPIO0  (D3) MUST be HIGH at boot — low level forces flash/download mode
#define BUTTON1_PIN 4        // D2 (GPIO4)  - Button 1, external pullup
#define BUTTON2_PIN 0        // D3 (GPIO0)  - Button 2, external pullup
                             //   *** Boot mode pin: if held LOW at reset → flash mode
#define BUTTON3_PIN 2        // D4 (GPIO2)  - Button 3, external pullup
                             //   *** UART1 TX: bootloader emits 74880-baud data on this
                             //       pin at every reset; Serial1.end() called in setup()
                             //       to suppress firmware-level UART1 traffic
#define BUTTON4_PIN 14       // D5 (GPIO14) - Button 4, external pullup

// CH224K PD voltage control pins (each has physical GND switch + 3V3 pullup)
// *** HARDWARE WARNING — CFG3 on GPIO15 (D8):
//     GPIO15 MUST be LOW at boot. The 3V3 pull-up on CFG3 holds GPIO15 HIGH at
//     rest, which will prevent the ESP8266 from booting when the PCB is connected.
//     Hardware fix required: add a pull-down resistor (e.g. 10kΩ to GND) on GPIO15
//     that dominates the 3V3 pull-up at boot, OR route CFG3 to a different pin.
#define CFG1_PIN 12          // D6 (GPIO12) - CH224K CFG1
#define CFG2_PIN 13          // D7 (GPIO13) - CH224K CFG2
#define CFG3_PIN 15          // D8 (GPIO15) - CH224K CFG3  *** see warning above

// ADC pin for voltage sensing (ESP8266 has single ADC; no VOUT sense on this variant)
#define VBUS_ADC_PIN A0      // A0 - VBUS voltage sensing

// Output control pins
#define POWER_JACK_PIN 5     // D1 (GPIO5)  - Barrel jack VOUT enable (HIGH=enable, LOW=disable)
#define USB_OUTPUT_PIN 16    // D0 (GPIO16) - CH217K USB 5V enable (LOW=enable, HIGH=disable)

// Timing
#define WIFI_RETRY_INTERVAL 60000      // 1 minute
#define BUTTON_DEBOUNCE 50
#define TIME_UPDATE_INTERVAL 3600000   // 1 hour

// ADC configuration for ESP8266 (10-bit, A0 = Vbus * 5.1/(47+5.1))
#define ADC_RESOLUTION 1023            // 10-bit ADC
#define ADC_VREF 3.3                   // Reference voltage
#define VBUS_DIVIDER_RATIO 10.216      // Vbus = A0_voltage * (47+5.1)/5.1 = A0_voltage * 10.216

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
