#ifndef HARDWARE_H
#define HARDWARE_H

#include "config.h"

// Power output control
void setPowerJackState(bool state);
void setUSBOutputState(bool state);

// PD voltage control
void setPDVoltage(uint8_t voltage);

// Voltage sensing
float getVBusVoltage();
float getVOutVoltage();

// Button handling
void checkButtons();

#endif
