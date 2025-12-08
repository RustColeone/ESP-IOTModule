#include "hardware.h"
#include "storage.h"

void setPowerState(bool state) {
  powerState = state;
  digitalWrite(POWER_SWITCH_PIN, state ? HIGH : LOW);
  Serial.print(F("Power state: "));
  Serial.println(state ? F("ON") : F("OFF"));
}

void setPDVoltage(uint8_t voltage) {
  // CFG1 is physical switch (must be LOW for PD request)
  // We control CFG2 (D6/GPIO12) and CFG3 (D7/GPIO13)
  
  switch (voltage) {
    case 9:   // 0 0 0
      digitalWrite(CFG2_PIN, LOW);
      digitalWrite(CFG3_PIN, LOW);
      break;
    case 12:  // 0 0 1
      digitalWrite(CFG2_PIN, LOW);
      digitalWrite(CFG3_PIN, HIGH);
      break;
    case 15:  // 0 1 1
      digitalWrite(CFG2_PIN, HIGH);
      digitalWrite(CFG3_PIN, HIGH);
      break;
    case 20:  // 0 1 0
      digitalWrite(CFG2_PIN, HIGH);
      digitalWrite(CFG3_PIN, LOW);
      break;
    default:
      Serial.println(F("ERR: Invalid PD voltage. Use 9, 12, 15, or 20."));
      return;
  }
  
  config.pdVoltage = voltage;
  saveConfig();
  Serial.print(F("PD voltage set to: "));
  Serial.print(voltage);
  Serial.println(F("V"));
  
  // Wait a bit and verify
  delay(500);
  float measuredV = getVBusVoltage();
  Serial.print(F("Measured voltage: "));
  Serial.print(measuredV, 2);
  Serial.println(F("V"));
}

float getVBusVoltage() {
  int rawValue = analogRead(ANALOG_PIN);
  float a0Voltage = (rawValue * 3.3 / 1023.0);
  float vbus = a0Voltage * 52.1 / 5.1;
  return vbus;
}

void checkButtons() {
  if (millis() - lastButtonCheck < BUTTON_DEBOUNCE) return;
  lastButtonCheck = millis();
  
  // Button 1 - Toggle power
  bool btn1 = digitalRead(BUTTON1_PIN);
  if (btn1 == LOW && lastButton1 == HIGH) {
    setPowerState(!powerState);
  }
  lastButton1 = btn1;
  
  // Button 2 - Power ON
  bool btn2 = digitalRead(BUTTON2_PIN);
  if (btn2 == LOW && lastButton2 == HIGH) {
    setPowerState(true);
  }
  lastButton2 = btn2;
  
  // Button 3 - Power OFF
  bool btn3 = digitalRead(BUTTON3_PIN);
  if (btn3 == LOW && lastButton3 == HIGH) {
    setPowerState(false);
  }
  lastButton3 = btn3;
  
  // Button 4 - Reserved for future use
  bool btn4 = digitalRead(BUTTON4_PIN);
  if (btn4 == LOW && lastButton4 == HIGH) {
    Serial.println(F("Button 4 pressed (no action assigned)"));
  }
  lastButton4 = btn4;
}
