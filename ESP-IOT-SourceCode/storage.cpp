#include "storage.h"
#include <EEPROM.h>

void saveConfig() {
  EEPROM.write(ADDR_MAGIC, EEPROM_MAGIC);
  
  // Save WiFi credentials
  for (int i = 0; i < 64; i++) {
    EEPROM.write(ADDR_SSID + i, config.ssid[i]);
    EEPROM.write(ADDR_PASSWORD + i, config.password[i]);
  }
  
  // Save timezone
  for (int i = 0; i < 8; i++) {
    EEPROM.write(ADDR_TIMEZONE + i, config.timezone[i]);
  }
  
  // Save last time (4 bytes)
  EEPROM.write(ADDR_LAST_TIME + 0, (config.lastTime >> 24) & 0xFF);
  EEPROM.write(ADDR_LAST_TIME + 1, (config.lastTime >> 16) & 0xFF);
  EEPROM.write(ADDR_LAST_TIME + 2, (config.lastTime >> 8) & 0xFF);
  EEPROM.write(ADDR_LAST_TIME + 3, config.lastTime & 0xFF);
  
  // Save PD voltage
  EEPROM.write(ADDR_PD_VOLTAGE, config.pdVoltage);
  
  // Save schedule count
  EEPROM.write(ADDR_SCHEDULE_COUNT, config.scheduleCount);
  
  // Save schedules
  for (int i = 0; i < 10; i++) {
    int addr = ADDR_SCHEDULES + (i * 3);
    EEPROM.write(addr + 0, (config.schedules[i].time >> 8) & 0xFF);
    EEPROM.write(addr + 1, config.schedules[i].time & 0xFF);
    EEPROM.write(addr + 2, config.schedules[i].action);
  }
  
  EEPROM.commit();
  Serial.println(F("Config saved to EEPROM."));
}

void loadConfig() {
  if (EEPROM.read(ADDR_MAGIC) != EEPROM_MAGIC) {
    Serial.println(F("No valid config found. Using defaults."));
    memset(&config, 0, sizeof(config));
    strcpy(config.timezone, "UTC");
    config.pdVoltage = 9;
    return;
  }
  
  // Load WiFi credentials
  for (int i = 0; i < 64; i++) {
    config.ssid[i] = EEPROM.read(ADDR_SSID + i);
    config.password[i] = EEPROM.read(ADDR_PASSWORD + i);
  }
  
  // Load timezone
  for (int i = 0; i < 8; i++) {
    config.timezone[i] = EEPROM.read(ADDR_TIMEZONE + i);
  }
  
  // Load last time
  config.lastTime = ((time_t)EEPROM.read(ADDR_LAST_TIME + 0) << 24) |
                    ((time_t)EEPROM.read(ADDR_LAST_TIME + 1) << 16) |
                    ((time_t)EEPROM.read(ADDR_LAST_TIME + 2) << 8) |
                    ((time_t)EEPROM.read(ADDR_LAST_TIME + 3));
  
  // Load PD voltage
  config.pdVoltage = EEPROM.read(ADDR_PD_VOLTAGE);
  
  // Load schedule count
  config.scheduleCount = EEPROM.read(ADDR_SCHEDULE_COUNT);
  if (config.scheduleCount > 10) config.scheduleCount = 0;
  
  // Load schedules
  for (int i = 0; i < 10; i++) {
    int addr = ADDR_SCHEDULES + (i * 3);
    config.schedules[i].time = ((uint16_t)EEPROM.read(addr + 0) << 8) |
                               EEPROM.read(addr + 1);
    config.schedules[i].action = EEPROM.read(addr + 2);
  }
  
  Serial.println(F("Config loaded from EEPROM."));
}
