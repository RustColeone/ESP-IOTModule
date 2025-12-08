#include "scheduler.h"
#include "hardware.h"
#include <time.h>

void checkSchedules() {
  if (currentTime < 100000) return;  // Time not set
  
  struct tm *timeinfo = localtime(&currentTime);
  uint16_t currentTimeVal = timeinfo->tm_hour * 100 + timeinfo->tm_min;
  
  for (int i = 0; i < config.scheduleCount; i++) {
    if (config.schedules[i].time == currentTimeVal) {
      // Check if we haven't already executed this minute
      static uint16_t lastExecutedTime = 9999;
      if (currentTimeVal != lastExecutedTime) {
        lastExecutedTime = currentTimeVal;
        setPowerState(config.schedules[i].action == 1);
        Serial.print(F("Schedule executed: "));
        Serial.print(config.schedules[i].time);
        Serial.print(F(" -> "));
        Serial.println(config.schedules[i].action ? F("ON") : F("OFF"));
      }
    }
  }
}
