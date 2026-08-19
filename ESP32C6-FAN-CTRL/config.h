#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <time.h>

// ============================================================================
// ESP32-C6 FAN CONTROLLER - Configuration
// Board: Seeed Studio XIAO ESP32C6 (esp32 core >= 3.0.0)
// ============================================================================

static const uint32_t BAUD = 115200;

// ---------------------------------------------------------------------------
// XIAO ESP32C6 pin map (silkscreen label -> GPIO):
//   D0=GPIO0, D1=GPIO1, D2=GPIO2,  D3=GPIO21, D4=GPIO22, D5=GPIO23,
//   D6=GPIO16, D7=GPIO17, D8=GPIO19, D9=GPIO20, D10=GPIO18
// The D0..D10 constants are provided by the XIAO_ESP32C6 board variant.
// A fallback is included below for generic ESP32C6 boards.
// ---------------------------------------------------------------------------
#ifndef D0
  #define D0   0
  #define D1   1
  #define D2   2
  #define D3  21
  #define D4  22
  #define D5  23
  #define D6  16
  #define D7  17
  #define D8  19
  #define D9  20
  #define D10 18
#endif

#define FAN_COUNT 4

// Fan wiring: Control = 4-pin fan PWM input, Sense = fan tachometer output.
#define FAN0_SENSE_PIN    D0    // GPIO0  - Fan 0 tachometer input
#define FAN0_CONTROL_PIN  D1    // GPIO1  - Fan 0 PWM output (LP_GPIO)
#define FAN1_SENSE_PIN    D2    // GPIO2  - Fan 1 tachometer input (LP_GPIO)
#define FAN1_CONTROL_PIN  D3    // GPIO21 - Fan 1 PWM output
#define FAN2_SENSE_PIN    D4    // GPIO22 - Fan 2 tachometer input (I2C SDA)
#define FAN2_CONTROL_PIN  D5    // GPIO23 - Fan 2 PWM output (I2C SCL)
#define FAN3_SENSE_PIN    D6    // GPIO16 - Fan 3 tachometer input (UART0 TX)
#define FAN3_CONTROL_PIN  D7    // GPIO17 - Fan 3 PWM output (UART0 RX)

static const uint8_t FAN_SENSE_PINS[FAN_COUNT]   = { FAN0_SENSE_PIN,   FAN1_SENSE_PIN,   FAN2_SENSE_PIN,   FAN3_SENSE_PIN   };
static const uint8_t FAN_CONTROL_PINS[FAN_COUNT] = { FAN0_CONTROL_PIN, FAN1_CONTROL_PIN, FAN2_CONTROL_PIN, FAN3_CONTROL_PIN };

// Buttons (external pull-ups, active LOW)
#define BUTTON_DEC_PIN    D8    // GPIO19 (SPI SCK)  - all fans -25%
#define BUTTON_INC_PIN    D9    // GPIO20 (SPI MISO) - all fans +25%
#define BUTTON_TOGGLE_PIN D10   // GPIO18 (SPI MOSI) - toggle all fans

// Fan PWM
#define FAN_PWM_FREQ 25000      // Intel 4-wire fan spec: 21-28 kHz
#define FAN_PWM_RES  8          // 8-bit analogWrite resolution (0-255)
#define FAN_PULSES_PER_REV 2    // typical 4-wire fan tachometer: 2 pulses/rev

// Buttons
#define BUTTON_DEBOUNCE_MS 50

// Network
#define WIFI_RETRY_INTERVAL 60000    // 1 minute
#define TIME_UPDATE_INTERVAL 3600000 // 1 hour

// ============================================================================
// Persistent configuration (NVS via Preferences)
// ============================================================================
struct Config {
  char ssid[64];
  char password[64];
  char timezone[16];            // e.g. "UTC+8"
  uint8_t fanSpeed[FAN_COUNT];  // 0-100 (%)
};

extern Config config;
extern bool wifiConnected;
extern time_t currentTime;
extern unsigned long lastWifiAttempt;
extern uint32_t fanRpm[FAN_COUNT];

#endif
