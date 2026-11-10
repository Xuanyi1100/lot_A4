
/*
 * FILE          : ui_a.cpp
 * PROJECT       : SENG3030 Assignment 4 - Device A
 * DESCRIPTION   : Device A display handling.
 */

#include <M5Unified.h>

#include "ui_a.h"
#include "device_a.h"
#include "sensor_env.h"
#include "mic_recorder.h"
#include "net_common.h"


void drawScreen() {
    auto& display = M5.Display;

    display.setCursor(0, 0);

    display.setTextColor(CYAN, BLACK);
    display.printf("A: DATA ACQUISITION\n");

    display.setTextColor(WHITE, BLACK);
    display.printf(
        "WiFi : %-14s\n",
        netWifiOk() ? "connected" : "connecting"
    );

    display.printf(
        "MQTT : %-14s\n",
        netMqttOk() ? "connected" : "connecting"
    );

    bool dataEnabled = deviceADataEnabled();

    display.setTextColor(
        dataEnabled ? GREEN : RED,
        BLACK
    );

    display.printf(
        "Data : %-14s\n",
        dataEnabled ? "ON" : "OFF"
    );

    display.setTextColor(WHITE, BLACK);

    display.printf(
        "ENV  : %-14s\n",
        envSensorsOk() ? "ok" : "check cable"
    );

    display.printf(
        "Mic  : %-14s\n",
        micRecorderAvailable() ? "recording" : "no memory"
    );

    display.printf(
        "Sent : %lu  fail %lu   \n",
        (unsigned long)netPubOk(),
        (unsigned long)netPubFail()
    );

    display.printf(
        "Last : %-14s\n",
        netLastEvent()
    );
}
