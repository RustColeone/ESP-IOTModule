#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <time.h>

// ============================================================================
// CONFIGURATION
// ============================================================================
static const uint32_t BAUD = 115200;

// Pin definitions - ESP8266 (board swapped from ESP32-C6 to ESP8266 due to stock
// shortage; values below are the actual ESP8266 wiring — GPIO numbers differ from
// the original C6 pinout, see BOOT CONSTRAINTS below for ESP8266-specific caveats)
// BOOT CONSTRAINTS (ESP8266 samples these before user code runs):
//   GPIO15 (D8) MUST be LOW  at boot — high level prevents boot entirely (unused on this board)
//   GPIO2  (D4) MUST be HIGH at boot — also UART1 TX (74880 baud boot messages)
//   GPIO0  (D3) MUST be HIGH at boot — low level forces flash/download mode
#define BUTTON1_PIN 5        // D1 (GPIO5)  - Button 1, external pullup
#define BUTTON2_PIN 4        // D2 (GPIO4)  - Button 2, external pullup
#define BUTTON3_PIN 0        // D3 (GPIO0)  - Button 3, external pullup
                             //   *** Boot mode pin: if held LOW at reset → flash mode
#define BUTTON4_PIN 2        // D4 (GPIO2)  - Button 4, external pullup
                             //   *** UART1 TX: bootloader emits 74880-baud data on this
                             //       pin at every reset; Serial1.end() called in setup()
                             //       to suppress firmware-level UART1 traffic

// CH224K PD voltage control pins (each has physical GND switch + 3V3 pullup)
// NOTE: CFG3 used to be on GPIO15 (D8), which conflicted with the ESP8266 boot
// requirement (GPIO15 must be LOW at boot). CFG3 has been moved to GPIO13, so
// GPIO15 is now unused/free on this board and the old boot-pullup warning no
// longer applies to CFG3.
#define CFG1_PIN 14          // D5 (GPIO14) - CH224K CFG1
#define CFG2_PIN 12          // D6 (GPIO12) - CH224K CFG2
#define CFG3_PIN 13          // D7 (GPIO13) - CH224K CFG3

// ADC pin for voltage sensing (ESP8266 has single ADC; no VOUT sense on this variant)
#define VBUS_ADC_PIN A0      // A0 - VBUS voltage sensing

// Output control pins
// *** HARDWARE HAZARD — DO NOT USE GPIO16 (D0), GPIO9, or GPIO10 AS OUTPUTS:
//     - GPIO16/D0 is hard-wired to RST on virtually all ESP8266 dev boards (for
//       deep-sleep wake). Driving it LOW resets the chip immediately.
//     - GPIO9 (SD2) and GPIO10 (SD3) are SPI flash data lines used by the
//       bootloader on every boot (QIO/QOUT flash mode, the common default).
//       Driving them as general outputs corrupts flash reads and causes boot
//       loops/crashes.
//     GPIO15 (D8) is the only pin left that's free of these hazards (CFG3 moved
//     off it), so POWER_JACK_PIN now lives there. GPIO15 must be LOW at boot,
//     which matches "jack disabled by default" as long as nothing external
//     pulls it high before setup() runs — verify with a multimeter/scope if the
//     board fails to boot after wiring.
#define POWER_JACK_PIN 15    // D8 (GPIO15) - Barrel jack VOUT enable (HIGH=enable, LOW=disable)
// *** CAUTION: GPIO10 (SD3) is a SPI flash pin. It only behaves as a normal GPIO
//     when the flash is in DOUT mode; in QIO/QOUT mode (the common default) it is
//     reserved for the flash chip and will not work as a general output. Verify
//     the board's flash mode before relying on this pin.
#define USB_OUTPUT_PIN 10    // D12/SD3 (GPIO10) - CH217K USB 5V enable (LOW=enable, HIGH=disable)

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
