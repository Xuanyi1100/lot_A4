/*
 * FILE          : main.cpp
 * PROJECT       : SENG3030 Assignment 4 - Device A
 * DESCRIPTION   : Main entry point for Device A.
 */

#include <Arduino.h>
#include "device_a.h"

void setup() {
    deviceABegin();
}

void loop() {
    deviceAUpdate(millis());
}
