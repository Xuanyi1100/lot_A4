
/*
 * FILE          : main.cpp
 * PROJECT       : SENG3030 Assignment 4 - Device B (Data Display)
 * PROGRAMMERS   : Alexia Tu, Carlos Gomez Guerrero, Deyi Zou, Juan Jose Bejarano
 * FIRST VERSION : 2026-11-05
 * DESCRIPTION   : Main entry point for Device B.
 */

#include <M5Unified.h>

#include "topics.h"
#include "secrets.h"
#include "net_common.h"

#include "device_b.h"
#include "audio_player.h"
#include "ui_b.h"


// ================= setup =================

void setup() {
    auto config = M5.config();

    M5.begin(config);
    Serial.begin(115200);

    // Initialize audio hardware.
    M5.Mic.end();
    audioPlayerBegin();

    // Configure MQTT.
    NetConfig net = {};

    net.clientPrefix = "seng3030-B-";
    net.user = USER_DISP.c_str();
    net.pass = MQTT_PASS_DISP;
    net.statusTopic = T_DISP_STATUS.c_str();

    net.bufferSize = AUDIO_MQTT_BUFFER;

    net.onMessage = onMessage;
    net.onConnected = onConnected;

    bool audioBufferOk = netBegin(&net);

    audioPlayerSetBufferStatus(audioBufferOk);

    Serial.printf(
        "MQTT buffer %s. Free heap: %lu bytes\n",
        audioBufferOk ? "48 KB ok" : "FAILED (audio off)",
        (unsigned long)ESP.getFreeHeap()
    );

    // Initialize display.
    M5.Display.setRotation(1);
    M5.Display.fillScreen(BLACK);
}


// ================= main loop =================

void loop() {
    M5.update();

    uint32_t nowMs = millis();

    // Front button: next page.
    if (M5.BtnA.wasPressed()) {
        uiBChangePage(+1);
    }

    // Side button: previous page.
    if (M5.BtnB.wasPressed()) {
        uiBChangePage(-1);
    }

    netWifiTask(nowMs);

    audioPlayerTask();

    // Pause MQTT while the audio clip is playing.
    if (!audioPlayerIsPlaying()) {
        netMqttTask(nowMs);
    }

    uiBDrawScreen(nowMs);
}
