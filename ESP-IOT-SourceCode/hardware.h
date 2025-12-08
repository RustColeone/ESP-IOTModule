#ifndef HARDWARE_H
#define HARDWARE_H

#include "config.h"

void setPowerState(bool state);
void setPDVoltage(uint8_t voltage);
float getVBusVoltage();
void checkButtons();

#endif
