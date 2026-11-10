 /*
 * FILE          : main.cpp
 * PROJECT       : SENG3030 Assignment 4 - Device C (Control)
 * PROGRAMMERS   : Alexia Tu, Carlos Gomez Guerrero, Deyi Zou, Juan Jose Bejarano
 * DATE          : 2026-11-10
 * DESCRIPTION   : Main entry point for Device C.
 *   Controls device A and device B through MQTT. Publishes under
 *   SENG3030/Wednesday/<USER_CTRL>/ :
 *
 *     Button              Topic             Payload        Retained   Effect
 *     BtnA click          control/display   "next"         no         B shows the next page
 *     BtnB click          control/display   "prev"         no         B shows the previous page
 *     BtnA hold           control/data      "on" / "off"   yes        A starts / stops publishing
 *     BtnB hold           control/audio     "send"         no         A publishes its audio buffer
 *
 *   control/data is retained so A gets the last setting after a reboot.
 *   When C connects, it publishes its own setting (ON after power-up).
 *   "status" is "online" while connected; Last Will publishes "offline".
 */

#include <M5Unified.h>

#include "topics.h"
#include "secrets.h"
#include "net_common.h"

#include "device_c.h"
#include "commands.h"
#include "ui_c.h"

// ---------------- timing ----------------
const uint32_t SCREEN_PERIOD = 250;
const uint32_t HOLD_MS = 800;

uint32_t screenTime = 0;

// ---------------- setup ----------------

void setup() {
    auto cfg = M5.config();

    M5.begin(cfg);
    Serial.begin(115200);

    M5.BtnA.setHoldThresh(HOLD_MS);
    M5.BtnB.setHoldThresh(HOLD_MS);

    M5.Speaker.setVolume(80);

    M5.Display.setRotation(1);
    M5.Display.fillScreen(BLACK);

    NetConfig net = {};

    net.clientPrefix = "seng3030-C-";
    net.user = USER_CTRL.c_str();
    net.pass = MQTT_PASS_CTRL;
    net.statusTopic = T_CTRL_STATUS.c_str();
    net.onConnected = onConnected;

    netBegin(&net);
}

// ---------------- main loop ----------------

void loop() {
    M5.update();

    uint32_t now = millis();

    netUpdate(now);
    handleButtons();

    if (now - screenTime >= SCREEN_PERIOD) {
        screenTime = now;
        drawScreen();
    }
}
