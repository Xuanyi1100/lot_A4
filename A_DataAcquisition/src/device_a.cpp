/*
 * FILE          : device_a.cpp
 * PROJECT       : SENG3030 Assignment 4 - Device A
 * DESCRIPTION   : Coordinates Device A modules and scheduled tasks.
 */

#include <M5Unified.h>

#include "device_a.h"
#include "topics.h"
#include "secrets.h"
#include "net_common.h"

#include "sensor_env.h"
#include "sensor_imu.h"
#include "battery.h"
#include "mic_recorder.h"
#include "ui_a.h"

// ---------------- timing (ms) ----------------

static const uint32_t ENV_PERIOD = 1000;
static const uint32_t IMU_PERIOD = 200;
static const uint32_t BATTERY_PERIOD = 10000;
static const uint32_t SCREEN_PERIOD = 500;

// ---------------- device state ----------------

static bool dataEnabled = true;
static bool audioRequested = false;

// ---------------- task management ----------------

typedef void (*DeviceTaskCallback)();

struct DeviceTask {
    uint32_t period;
    uint32_t lastRun;
    DeviceTaskCallback callback;
    bool requiresDataEnabled;
};

static DeviceTask tasks[] = {
    { ENV_PERIOD,     0, publishEnv,     true  },
    { IMU_PERIOD,     0, publishImu,     true  },
    { BATTERY_PERIOD, 0, publishBattery, true  },
    { SCREEN_PERIOD,  0, drawScreen,     false }
};

static const size_t TASK_COUNT = sizeof(tasks) / sizeof(tasks[0]);

// ---------------- network callbacks ----------------

static void onConnected();
static void onMessage(char* topic, uint8_t* payload, unsigned int length);


// ================= initialization =================

void deviceABegin() {
    auto cfg = M5.config();

    M5.begin(cfg);
    Serial.begin(115200);

    M5.Display.setRotation(1);
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextSize(1.5f);

    setupMicRecorder();
    setupEnv();

    NetConfig net = {};

    net.clientPrefix = "seng3030-A-";
    net.user = USER_ACQ.c_str();
    net.pass = MQTT_PASS_ACQ;
    net.statusTopic = T_ACQ_STATUS.c_str();
    net.bufferSize = 512;
    net.onMessage = onMessage;
    net.onConnected = onConnected;

    netBegin(&net);

    Serial.printf(
        "Setup done. Free heap: %lu bytes\n",
        (unsigned long)ESP.getFreeHeap()
    );
}


// ================= main update =================

void deviceAUpdate(uint32_t nowMs) {
    M5.update();

    netUpdate(nowMs);

    // Continue recording even when publishing is disabled.
    recordAudio();

    bool canPublish = netMqttOk() && dataEnabled;

    for (size_t i = 0; i < TASK_COUNT; i++) {
        DeviceTask& task = tasks[i];

        if (task.requiresDataEnabled && !canPublish) {
            continue;
        }

        if (nowMs - task.lastRun >= task.period) {
            task.lastRun = nowMs;
            task.callback();
        }
    }

    if (canPublish && audioRequested) {
    audioRequested = false;
    sendAudio();
    } else if (!canPublish) {
        audioRequested = false;
    }
}


// ================= public state =================

bool deviceADataEnabled() {
    return dataEnabled;
}


// ================= network callbacks =================

static void onConnected() {
    netSubscribe(T_CTRL_DATA.c_str(), 1);
    netSubscribe(T_CTRL_AUDIO.c_str(), 1);

    // Make sensor tasks run immediately after connecting.
    uint32_t nowMs = millis();

    for (size_t i = 0; i < TASK_COUNT; i++) {
        if (tasks[i].requiresDataEnabled) {
            tasks[i].lastRun = nowMs - tasks[i].period;
        }
    }
}


static void onMessage(
    char* topic,
    uint8_t* payload,
    unsigned int length
) {
    String msg;
    msg.reserve(length);

    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }

    msg.trim();

    if (T_CTRL_DATA == topic) {
        if (msg == "on") {
            dataEnabled = true;
        }

        if (msg == "off") {
            dataEnabled = false;
        }

        String event = "data " + msg;
        netSetEvent(event.c_str());
    } else if (T_CTRL_AUDIO == topic && msg == "send") {
        audioRequested = true;
    }

    Serial.printf("RX %s = %s\n", topic, msg.c_str());
}
