
/*
 * FILE        : readings.cpp
 * PROJECT     : SENG3030 Assignment 4 - Device B
 * DESCRIPTION : Sensor data storage and stale-data calculations.
 */

#include "readings.h"

#include <Arduino.h>
#include <stdlib.h>
#include <math.h>


// ================= sensor readings =================

Reading shtTemp, shtHumi;

Reading qmpTemp, qmpPres, qmpAlt;

Reading accelX, accelY, accelZ;

Reading gyroX, gyroY, gyroZ;

Reading batteryMv;


// ================= device status =================

bool aStatusKnown = false;
bool aOnline = false;


// ================= reading functions =================

void setReading(Reading& reading, const char* text) {
    reading.value = atof(text);
    reading.updatedAt = millis();
}


// Returns the timestamp of the oldest reading on a page.
// Returns 0 if a required reading has never arrived.
uint32_t pageUpdatedAt(Page page) {
    switch (page) {
        case PAGE_SHT:
            return min(shtTemp.updatedAt, shtHumi.updatedAt);

        case PAGE_QMP:
            return min(
                qmpTemp.updatedAt,
                min(qmpPres.updatedAt, qmpAlt.updatedAt)
            );

        case PAGE_ACCEL:
            return accelX.updatedAt;

        case PAGE_GYRO:
            return gyroX.updatedAt;

        case PAGE_POWER:
            return batteryMv.updatedAt;

        case PAGE_MIC:
            return 0;

        default:
            return 0;
    }
}


// Checks whether the current page's data is stale.
bool isPageStale(Page page, uint32_t nowMs) {
    if (page == PAGE_MIC) {
        return false;
    }

    uint32_t updatedAt = pageUpdatedAt(page);

    if (updatedAt == 0) {
        return false;
    }

    uint32_t staleLimit = page == PAGE_POWER
        ? STALE_BATTERY
        : STALE_AFTER;

    return nowMs - updatedAt > staleLimit;
}
