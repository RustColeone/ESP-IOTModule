#ifndef HARDWARE_H
#define HARDWARE_H

#include "config.h"

void initHardware();

void applyFanSpeed(uint8_t fan, uint8_t percent);  // set PWM only (no persist)
void setFanSpeed(uint8_t fan, uint8_t percent);    // set + persist
void setAllFanSpeed(uint8_t percent);
uint8_t getFanSpeed(uint8_t fan);

void updateFanTelemetry();  // compute RPM from tach pulses (call every loop)
void checkButtons();

#endif
