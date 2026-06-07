#include "scheduler.h"
#include "hardware.h"
#include <time.h>

// action byte packing: bits[1:0] = state (0=off, 1=on), bits[3:2] = target (0=both, 1=jack, 2=usb)
// Backward compatible: old action=0 → off/both, old action=1 → on/both.

void checkSchedules() {
  if (currentTime < 100000) return;

  struct tm *timeinfo = localtime(&currentTime);
  uint16_t currentTimeVal = timeinfo->tm_hour * 100 + timeinfo->tm_min;

  for (int i = 0; i < config.scheduleCount; i++) {
    if (config.schedules[i].time == currentTimeVal) {
      static uint16_t lastExecutedTime = 9999;
      if (currentTimeVal != lastExecutedTime) {
        lastExecutedTime = currentTimeVal;

        bool on     = (config.schedules[i].action & 0x01) == 1;
        uint8_t tgt = (config.schedules[i].action >> 1) & 0x03; // 0=both,1=jack,2=usb

        if (tgt == 0 || tgt == 1) setPowerJackState(on);
        if (tgt == 0 || tgt == 2) setUSBOutputState(on);

        const char* tgtStr = (tgt == 1) ? "Jack" : (tgt == 2) ? "USB" : "Both";
        Serial.print(F("Schedule: "));
        Serial.print(config.schedules[i].time);
        Serial.print(F(" -> "));
        Serial.print(on ? F("ON") : F("OFF"));
        Serial.print(F(" ("));
        Serial.print(tgtStr);
        Serial.println(F(")"));
      }
    }
  }
}
