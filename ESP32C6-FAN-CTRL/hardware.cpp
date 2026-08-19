#include "hardware.h"
#include "storage.h"

uint32_t fanRpm[FAN_COUNT] = {0, 0, 0, 0};
static volatile uint32_t tachPulseCount[FAN_COUNT] = {0, 0, 0, 0};
static uint32_t lastTachCount[FAN_COUNT] = {0, 0, 0, 0};
static uint32_t lastRpmSample = 0;

// Tachometer ISRs (one per fan; attachInterrupt needs a void(*)() callback)
void IRAM_ATTR fan0TachISR() { tachPulseCount[0]++; }
void IRAM_ATTR fan1TachISR() { tachPulseCount[1]++; }
void IRAM_ATTR fan2TachISR() { tachPulseCount[2]++; }
void IRAM_ATTR fan3TachISR() { tachPulseCount[3]++; }

void initHardware() {
  for (int i = 0; i < FAN_COUNT; i++) {
    // Fan PWM: attach the channel first, then raise the frequency to 25 kHz
    // (analogWrite default is 8-bit resolution and ~1 kHz).
    pinMode(FAN_CONTROL_PINS[i], OUTPUT);
    analogWrite(FAN_CONTROL_PINS[i], 0);
    analogWriteFrequency(FAN_CONTROL_PINS[i], FAN_PWM_FREQ);

    // Tachometer is open-collector: pull up to 3V3. Internal pull-up is used
    // here; add a 10k external pull-up to 3V3 for a cleaner signal.
    pinMode(FAN_SENSE_PINS[i], INPUT_PULLUP);
  }

  // Count tachometer pulses on falling edges.
  attachInterrupt(digitalPinToInterrupt(FAN0_SENSE_PIN), fan0TachISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(FAN1_SENSE_PIN), fan1TachISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(FAN2_SENSE_PIN), fan2TachISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(FAN3_SENSE_PIN), fan3TachISR, FALLING);

  // Buttons: external pull-ups on the PCB, so plain inputs (active LOW).
  pinMode(BUTTON_DEC_PIN, INPUT);
  pinMode(BUTTON_INC_PIN, INPUT);
  pinMode(BUTTON_TOGGLE_PIN, INPUT);
}

void applyFanSpeed(uint8_t fan, uint8_t percent) {
  if (fan >= FAN_COUNT) return;
  uint8_t speed = constrain(percent, 0, 100);
  uint32_t duty = ((uint32_t)speed * 255) / 100;
  analogWrite(FAN_CONTROL_PINS[fan], duty);
  config.fanSpeed[fan] = speed;
}

void setFanSpeed(uint8_t fan, uint8_t percent) {
  if (fan >= FAN_COUNT) return;
  applyFanSpeed(fan, percent);
  saveConfig();
  Serial.printf("Fan %d: %d%%\n", fan, config.fanSpeed[fan]);
}

void setAllFanSpeed(uint8_t percent) {
  uint8_t speed = constrain(percent, 0, 100);
  for (int i = 0; i < FAN_COUNT; i++) {
    applyFanSpeed(i, speed);
  }
  saveConfig();
  Serial.printf("All fans: %d%%\n", speed);
}

uint8_t getFanSpeed(uint8_t fan) {
  if (fan >= FAN_COUNT) return 0;
  return config.fanSpeed[fan];
}

void updateFanTelemetry() {
  uint32_t now = millis();
  if (now - lastRpmSample < 1000) return;
  lastRpmSample = now;

  for (int i = 0; i < FAN_COUNT; i++) {
    uint32_t pulses = tachPulseCount[i] - lastTachCount[i];
    lastTachCount[i] = tachPulseCount[i];
    // RPM = pulses/sec * 60 / pulses_per_rev
    fanRpm[i] = (pulses * 60) / FAN_PULSES_PER_REV;
  }
}

void checkButtons() {
  static bool lastDec = HIGH, lastInc = HIGH, lastTgl = HIGH;
  static uint32_t lastCheck = 0;

  if (millis() - lastCheck < BUTTON_DEBOUNCE_MS) return;
  lastCheck = millis();

  bool dec = digitalRead(BUTTON_DEC_PIN);
  bool inc = digitalRead(BUTTON_INC_PIN);
  bool tgl = digitalRead(BUTTON_TOGGLE_PIN);

  if (dec == LOW && lastDec == HIGH) {
    Serial.println(F("Button: all fans -25%"));
    for (int i = 0; i < FAN_COUNT; i++) {
      uint8_t cur = getFanSpeed(i);
      setFanSpeed(i, cur >= 25 ? cur - 25 : 0);
    }
  }

  if (inc == LOW && lastInc == HIGH) {
    Serial.println(F("Button: all fans +25%"));
    for (int i = 0; i < FAN_COUNT; i++) {
      uint8_t cur = getFanSpeed(i);
      setFanSpeed(i, cur <= 75 ? cur + 25 : 100);
    }
  }

  if (tgl == LOW && lastTgl == HIGH) {
    bool anyOn = false;
    for (int i = 0; i < FAN_COUNT; i++) {
      if (getFanSpeed(i) > 0) anyOn = true;
    }
    Serial.println(anyOn ? F("Button: all fans OFF") : F("Button: all fans FULL"));
    setAllFanSpeed(anyOn ? 0 : 100);
  }

  lastDec = dec;
  lastInc = inc;
  lastTgl = tgl;
}
