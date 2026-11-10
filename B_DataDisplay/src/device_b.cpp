
/*
 * FILE        : device_b.cpp
 * PROJECT     : SENG3030 Assignment 4 - Device B
 * DESCRIPTION : MQTT subscriptions and incoming message routing.
 */

#include "device_b.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <string.h>

#include "topics.h"

#include "readings.h"
#include "audio_player.h"
#include "ui_b.h"
#include "net_common.h"


// ================= network callbacks =================

// Called right after MQTT connects and "online" is published.
void onConnected() {
    netSubscribe((BASE_ACQ + "#").c_str(), 0);
    netSubscribe(T_CTRL_DISPLAY.c_str(), 1);
}


// Called for every message on a subscribed topic.
void onMessage(char* topic, uint8_t* payload, unsigned int length) {
    // Audio messages are binary, not text.
    if (T_MIC_CLIP == topic) {
        audioPlayerHandleClip(topic, payload, length);
        return;
    }

    // All other messages are short text.
    char message[200];

    size_t messageLen = length < sizeof(message) - 1
        ? length
        : sizeof(message) - 1;

    memcpy(message, payload, messageLen);
    message[messageLen] = '\0';

    // ---- display control ----
    if (T_CTRL_DISPLAY == topic) {
        if (strcmp(message, "next") == 0) {
            uiBChangePage(+1);
        }
        else if (strcmp(message, "prev") == 0) {
            uiBChangePage(-1);
        }
    }

    // ---- SHT30 ----
    else if (T_SHT_TEMP == topic) {
        setReading(shtTemp, message);
    }
    else if (T_SHT_HUMI == topic) {
        setReading(shtHumi, message);
    }

    // ---- QMP6988 ----
    else if (T_QMP_TEMP == topic) {
        setReading(qmpTemp, message);
    }
    else if (T_QMP_PRES == topic) {
        setReading(qmpPres, message);
    }
    else if (T_QMP_ALT == topic) {
        setReading(qmpAlt, message);
    }

    // ---- battery ----
    else if (T_BATTERY == topic) {
        setReading(batteryMv, message);
    }

    // ---- Device A status ----
    else if (T_ACQ_STATUS == topic) {
        aStatusKnown = true;
        aOnline = (strcmp(message, "online") == 0);
    }

    // ---- IMU ----
    else if (T_IMU == topic) {
        JsonDocument doc;

        if (deserializeJson(doc, message) == DeserializationError::Ok) {
            uint32_t nowMs = millis();

            accelX.value = doc["ax"] | NAN;
            accelY.value = doc["ay"] | NAN;
            accelZ.value = doc["az"] | NAN;

            gyroX.value = doc["gx"] | NAN;
            gyroY.value = doc["gy"] | NAN;
            gyroZ.value = doc["gz"] | NAN;

            accelX.updatedAt = nowMs;
            accelY.updatedAt = nowMs;
            accelZ.updatedAt = nowMs;

            gyroX.updatedAt = nowMs;
            gyroY.updatedAt = nowMs;
            gyroZ.updatedAt = nowMs;
        }
    }
}
