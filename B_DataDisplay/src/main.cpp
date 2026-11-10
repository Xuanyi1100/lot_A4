 /*
 * FILE          : main.cpp
 * PROJECT       : SENG3030 Assignment 4 - Device B (Data Display)
 * PROGRAMMERS   : Alexia Tu, Carlos Gomez Guerrero, Deyi Zou, Juan Jose Bejarano
 * DATE          : 2026-11-10
 * DESCRIPTION   : Main entry point for Device B.
 *   Subscribes to everything device A publishes (SENG3030/Wednesday/<USER_ACQ>/#)
 *   and shows it on the LCD, one sensor per page:
 *       1 SHT30   2 QMP6988   3 Accel   4 Gyro   5 Power   6 Mic
 *   - Front button (BtnA) = next page, side button (BtnB) = previous page.
 *   - Device C can also change the page: "next" / "prev" on <USER_CTRL>/control/display.
 *   - Every new audio clip from A (mic/clip) is played on the speaker.
 *   - A page turns red ("STALE") when its data is older than 5 s (25 s for the
 *     battery, which A sends only every 10 s), e.g. when
 *     C has turned A's data off or A has dropped off.
 *   - Publishes its own "status" (online, with Last Will "offline").
 *
 *   Audio without a second 48 KB buffer:
 *   the MQTT receive buffer is made large enough for one clip. While the clip
 *   plays (~1.1 s), loop() does not call mqtt.loop(), so the clip stays in that
 *   buffer. New messages wait in the network stack and arrive right after.
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
