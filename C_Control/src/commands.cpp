
/*
 * FILE        : commands.cpp
 * PROJECT     : SENG3030 Assignment 4 - Device C (Control)
 * DESCRIPTION : Button handling and MQTT command implementation.
 */

#include <M5Unified.h>

#include "commands.h"
#include "device_c.h"

#include "topics.h"
#include "net_common.h"

// ================= buttons =================

void handleButtons() {
    // wasClicked() fires on release of a short press.
    // wasHold() fires once when a press passes HOLD_MS.
    // A hold never also counts as a click.

    if (M5.BtnA.wasClicked()) {
        sendCommand(T_CTRL_DISPLAY, "next", false, "B: next page");
    }

    if (M5.BtnB.wasClicked()) {
        sendCommand(T_CTRL_DISPLAY, "prev", false, "B: prev page");
    }

    if (M5.BtnA.wasHold()) {
        bool want = !dataOn;

        if (sendCommand(
            T_CTRL_DATA,
            want ? "on" : "off",
            true,
            want ? "A: data ON" : "A: data OFF"
        )) {
            dataOn = want;
        }
    }

    if (M5.BtnB.wasHold()) {
        sendCommand(T_CTRL_AUDIO, "send", false, "A: send audio");
    }
}

// ================= commands =================

bool sendCommand(
    const std::string& topic,
    const char* payload,
    bool retained,
    const char* label
) {
    bool ok = netPublishText(topic.c_str(), payload, retained);

    lastCmd = label;
    lastOk = ok;

    beep(ok);

    Serial.printf(
        "%s %s = %s\n",
        ok ? "TX" : "NOT SENT",
        topic.c_str(),
        payload
    );

    return ok;
}

// ================= speaker feedback =================

void beep(bool ok) {
    // tone() is non-blocking.
    if (ok) {
        M5.Speaker.tone(3000, 40);
    } else {
        M5.Speaker.tone(800, 200);
    }
}
