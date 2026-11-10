/*
 * FILE          : battery.cpp
 * PROJECT       : SENG3030 Assignment 4 - Device A
 * DESCRIPTION   : Reads and publishes battery voltage.
 */

#include <M5Unified.h>

#include "battery.h"
#include "topics.h"
#include "net_common.h"


void publishBattery() {
    char buf[16];

    snprintf(
        buf,
        sizeof(buf),
        "%d",
        (int)M5.Power.getBatteryVoltage()
    );

    netPublishText(T_BATTERY.c_str(), buf, true);
}
