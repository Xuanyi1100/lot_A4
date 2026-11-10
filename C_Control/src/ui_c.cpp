
/*
 * FILE        : ui_c.cpp
 * PROJECT     : SENG3030 Assignment 4 - Device C (Control)
 * DESCRIPTION : Device C screen rendering implementation.
 */

#include <M5Unified.h>

#include "ui_c.h"
#include "device_c.h"

#include "net_common.h"

// ================= screen =================

void drawScreen() {
    auto& d = M5.Display;

    // ---------------- header ----------------
    d.setTextSize(1.5f);
    d.setCursor(0, 0);
    d.setTextColor(CYAN, BLACK);
    d.print("C: CONTROL     ");

    d.setCursor(150, 0);

    if (!netWifiOk()) {
        d.setTextColor(ORANGE, BLACK);
        d.print("WiFi ... ");
    } else if (!netMqttOk()) {
        d.setTextColor(ORANGE, BLACK);
        d.print("MQTT ... ");
    } else {
        d.setTextColor(GREEN, BLACK);
        d.print("online   ");
    }

    d.drawFastHLine(0, 16, 240, DARKGREY);

    // ---------------- A data switch ----------------
    d.setTextSize(2.5f);
    d.setCursor(4, 24);
    d.setTextColor(WHITE, BLACK);
    d.print("A data ");

    d.setTextColor(dataOn ? GREEN : RED, BLACK);
    d.print(dataOn ? "ON " : "OFF");

    // ---------------- last command ----------------
    d.setTextSize(1.5f);
    d.setCursor(4, 54);
    d.setTextColor(LIGHTGREY, BLACK);
    d.print("Last: ");

    d.setTextColor(lastOk ? WHITE : RED, BLACK);

    d.printf(
        "%-14s",
        lastOk ? lastCmd.c_str() : (lastCmd + " FAIL").c_str()
    );

    // ---------------- button help ----------------
    d.drawFastHLine(0, 72, 240, DARKGREY);
    d.setTextColor(DARKGREY, BLACK);

    d.setCursor(4, 80);
    d.print("A click : B next page");

    d.setCursor(4, 94);
    d.print("B click : B prev page");

    d.setCursor(4, 108);
    d.print("A hold  : A data on/off");

    d.setCursor(4, 122);
    d.print("B hold  : A send audio");
}
