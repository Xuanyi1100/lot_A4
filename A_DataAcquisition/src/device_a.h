
#ifndef DEVICE_A_H
#define DEVICE_A_H

#include <Arduino.h>

void deviceABegin();
void deviceAUpdate(uint32_t nowMs);

bool deviceADataEnabled();

#endif
