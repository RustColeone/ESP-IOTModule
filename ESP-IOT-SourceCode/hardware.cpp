#include "hardware.h"
#include "storage.h"

// ============================================================================
// Power Output Control
// ============================================================================
void setPowerJackState(bool state) {
  powerJackState = state;
  digitalWrite(POWER_JACK_PIN, state ? HIGH : LOW);
  config.powerJackState = state;
  saveConfig();
  Serial.print(F("Power Jack state: "));
  Serial.println(state ? F("ON (Enabled)") : F("OFF (Disabled)"));
}

void setUSBOutputState(bool state) {
  usbOutputState = state;
  digitalWrite(USB_OUTPUT_PIN, state ? LOW : HIGH);  // Inverted: LOW=enable, HIGH=disable
  config.usbOutputState = state;
  saveConfig();
  Serial.print(F("USB Output state: "));
  Serial.println(state ? F("ON (Enabled)") : F("OFF (Disabled)"));
}

// ============================================================================
// PD Voltage Control via CH224K
// ============================================================================
void setPDVoltage(uint8_t voltage) {
  // CH224K CFG pins control PD voltage request
  // CFG1 CFG2 CFG3 mapping:
  // 1XX = 5V
  // 000 = 9V
  // 001 = 12V
  // 011 = 15V
  // 010 = 20V
  
  switch (voltage) {
    case 5:   // 1XX (we'll use 100)
      digitalWrite(CFG1_PIN, HIGH);
      digitalWrite(CFG2_PIN, LOW);
      digitalWrite(CFG3_PIN, LOW);
      break;
    case 9:   // 000
      digitalWrite(CFG1_PIN, LOW);
      digitalWrite(CFG2_PIN, LOW);
      digitalWrite(CFG3_PIN, LOW);
      break;
    case 12:  // 001
      digitalWrite(CFG1_PIN, LOW);
      digitalWrite(CFG2_PIN, LOW);
      digitalWrite(CFG3_PIN, HIGH);
      break;
    case 15:  // 011
      digitalWrite(CFG1_PIN, LOW);
      digitalWrite(CFG2_PIN, HIGH);
      digitalWrite(CFG3_PIN, HIGH);
      break;
    case 20:  // 010
      digitalWrite(CFG1_PIN, LOW);
      digitalWrite(CFG2_PIN, HIGH);
      digitalWrite(CFG3_PIN, LOW);
      break;
    default:
      Serial.println(F("ERR: Invalid PD voltage. Use 5, 9, 12, 15, or 20."));
      return;
  }
  
  config.pdVoltage = voltage;
  saveConfig();
  Serial.print(F("PD voltage set to: "));
  Serial.print(voltage);
  Serial.println(F("V"));
  
  // Wait a bit and verify
  delay(500);
  float measuredVBus = getVBusVoltage();
  float measuredVOut = getVOutVoltage();
  Serial.print(F("Measured VBUS: "));
  Serial.print(measuredVBus, 2);
  Serial.println(F("V"));
  Serial.print(F("Measured VOUT: "));
  Serial.print(measuredVOut, 2);
  Serial.println(F("V"));
}

// ============================================================================
// Voltage Sensing
// ============================================================================
float getVBusVoltage() {
  int rawValue = analogRead(VBUS_ADC_PIN);
  float adcVoltage = (rawValue * ADC_VREF / ADC_RESOLUTION);
  float vbus = adcVoltage * VBUS_DIVIDER_RATIO;
  return vbus;
}

float getVOutVoltage() {
  int rawValue = analogRead(VOUT_ADC_PIN);
  float adcVoltage = (rawValue * ADC_VREF / ADC_RESOLUTION);
  float vout = adcVoltage * VOUT_DIVIDER_RATIO;
  return vout;
}

// ============================================================================
// Button Handling
// ============================================================================
void checkButtons() {
  if (millis() - lastButtonCheck < BUTTON_DEBOUNCE) return;
  lastButtonCheck = millis();
  
  // Button 1 - Toggle Power Jack
  bool btn1 = digitalRead(BUTTON1_PIN);
  if (btn1 == LOW && lastButton1 == HIGH) {
    setPowerJackState(!powerJackState);
  }
  lastButton1 = btn1;
  
  // Button 2 - Toggle USB Output
  bool btn2 = digitalRead(BUTTON2_PIN);
  if (btn2 == LOW && lastButton2 == HIGH) {
    setUSBOutputState(!usbOutputState);
  }
  lastButton2 = btn2;
  
  // Button 3 - Cycle PD Voltage (5V -> 9V -> 12V -> 15V -> 20V -> 5V)
  bool btn3 = digitalRead(BUTTON3_PIN);
  if (btn3 == LOW && lastButton3 == HIGH) {
    uint8_t nextVoltage;
    switch (config.pdVoltage) {
      case 5:  nextVoltage = 9;  break;
      case 9:  nextVoltage = 12; break;
      case 12: nextVoltage = 15; break;
      case 15: nextVoltage = 20; break;
      case 20: nextVoltage = 5;  break;
      default: nextVoltage = 9;  break;
    }
    setPDVoltage(nextVoltage);
  }
  lastButton3 = btn3;
  
  // Button 4 - Turn everything ON
  bool btn4 = digitalRead(BUTTON4_PIN);
  if (btn4 == LOW && lastButton4 == HIGH) {
    Serial.println(F("Button 4: Enabling all outputs"));
    setPowerJackState(true);
    setUSBOutputState(true);
  }
  lastButton4 = btn4;
}
