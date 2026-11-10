
/*
 * FILE        : readings.h
 * PROJECT     : SENG3030 Assignment 4 - Device B
 * DESCRIPTION : Sensor reading structure, shared readings and stale-data logic.
 */

#ifndef READINGS_H
#define READINGS_H

#include <stdint.h>
#include <math.h>

// ================= constants =================

const uint32_t STALE_AFTER = 5000;
const uint32_t STALE_BATTERY = 25000;


// ================= pages =================

enum Page {
    PAGE_SHT,
    PAGE_QMP,
    PAGE_ACCEL,
    PAGE_GYRO,
    PAGE_POWER,
    PAGE_MIC,
    PAGE_COUNT
};


// ================= reading structure =================

struct Reading {
    float value = NAN;
    uint32_t updatedAt = 0;
};


// ================= sensor readings =================

extern Reading shtTemp;
extern Reading shtHumi;

extern Reading qmpTemp;
extern Reading qmpPres;
extern Reading qmpAlt;

extern Reading accelX;
extern Reading accelY;
extern Reading accelZ;

extern Reading gyroX;
extern Reading gyroY;
extern Reading gyroZ;

extern Reading batteryMv;


// ================= device status =================

extern bool aStatusKnown;
extern bool aOnline;


// ================= reading functions =================

void setReading(Reading& reading, const char* text);

uint32_t pageUpdatedAt(Page page);

bool isPageStale(Page page, uint32_t nowMs);


#endif // READINGS_H
