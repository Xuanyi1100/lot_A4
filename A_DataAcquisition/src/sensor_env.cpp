
/*
 * FILE          : sensor_env.cpp
 * PROJECT       : SENG3030 Assignment 4 - Device A
 * DESCRIPTION   : SHT30 and QMP6988 environmental sensor handling.
 */

#include <Arduino.h>
#include <Wire.h>
#include <M5UnitENV.h>

#include "sensor_env.h"
#include "topics.h"
#include "net_common.h"

// ---------------- hardware ----------------

static const int EXT_SDA = 32;
static const int EXT_SCL = 33;

// ---------------- sensor objects ----------------

static SHT3X sht3x;
static QMP6988 qmp;

static bool shtOk = false;
static bool qmpOk = false;


// ================= initialization =================

void setupEnv() {
    qmpOk = qmp.begin(
        &Wire,
        QMP6988_SLAVE_ADDRESS_L,
        EXT_SDA,
        EXT_SCL,
        400000U
    );

    shtOk = sht3x.begin(
        &Wire,
        SHT3X_I2C_ADDR,
        EXT_SDA,
        EXT_SCL,
        400000U
    );

    Serial.printf(
        "QMP6988: %s   SHT30: %s\n",
        qmpOk ? "ok" : "NOT FOUND",
        shtOk ? "ok" : "NOT FOUND"
    );
}


// ================= publishing =================

void publishEnv() {
    if (shtOk && sht3x.update()) {
        netPublishFloat(T_SHT_TEMP.c_str(), sht3x.cTemp);
        netPublishFloat(T_SHT_HUMI.c_str(), sht3x.humidity);
    }

    if (qmpOk && qmp.update()) {
        netPublishFloat(T_QMP_TEMP.c_str(), qmp.cTemp);
        netPublishFloat(T_QMP_PRES.c_str(), qmp.pressure);
        netPublishFloat(T_QMP_ALT.c_str(), qmp.altitude);
    }
}


// ================= status =================

bool envSensorsOk() {
    return shtOk && qmpOk;
}
