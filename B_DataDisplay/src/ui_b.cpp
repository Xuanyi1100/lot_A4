
/*
 * FILE        : ui_b.cpp
 * PROJECT     : SENG3030 Assignment 4 - Device B
 * DESCRIPTION : Page navigation and LCD rendering.
 */

#include "ui_b.h"

#include <Arduino.h>
#include <M5Unified.h>

#include "net_common.h"

#include "readings.h"
#include "audio_player.h"


// ================= constants =================

const uint32_t SCREEN_PERIOD = 250;

static const char* const PAGE_NAMES[PAGE_COUNT] = {
    "SHT30",
    "QMP6988",
    "Accel",
    "Gyro",
    "Power",
    "Mic"
};


// ================= UI state =================

static Page sCurrentPage = PAGE_SHT;
static bool sPageChanged = true;

static uint32_t sScreenTimer = 0;


// ================= prototypes =================

static void drawValue(
    int y,
    const char* label,
    const Reading& reading,
    const char* format,
    float scale,
    const char* unit
);


// ================= page navigation =================

void uiBChangePage(int step) {
    sCurrentPage = (Page)(
        (sCurrentPage + step + PAGE_COUNT) % PAGE_COUNT
    );

    sPageChanged = true;
}


Page uiBGetCurrentPage() {
    return sCurrentPage;
}


// ================= display =================

void uiBDrawScreen(uint32_t nowMs) {
    if (!sPageChanged && nowMs - sScreenTimer < SCREEN_PERIOD) {
        return;
    }

    sScreenTimer = nowMs;

    auto& display = M5.Display;

    if (sPageChanged) {
        display.fillScreen(BLACK);
        sPageChanged = false;
    }

    // ---- header: page number, page name, A online/offline ----

    display.setTextSize(1.5f);
    display.setCursor(0, 0);

    display.setTextColor(CYAN, BLACK);

    display.printf(
        "%d/%d %-8s",
        sCurrentPage + 1,
        PAGE_COUNT,
        PAGE_NAMES[sCurrentPage]
    );

    display.setCursor(130, 0);

    if (!netMqttOk()) {
        display.setTextColor(ORANGE, BLACK);
        display.print("MQTT ...  ");
    }
    else if (!aStatusKnown) {
        display.setTextColor(LIGHTGREY, BLACK);
        display.print("A: ?      ");
    }
    else if (aOnline) {
        display.setTextColor(GREEN, BLACK);
        display.print("A: online ");
    }
    else {
        display.setTextColor(RED, BLACK);
        display.print("A: offline");
    }

    display.drawFastHLine(0, 16, 240, DARKGREY);


    // ---- page content ----

    switch (sCurrentPage) {
        case PAGE_SHT:
            drawValue(28, "Temp", shtTemp, "%7.2f", 1.0f, "C");
            drawValue(56, "Humi", shtHumi, "%7.2f", 1.0f, "%rH");
            break;

        case PAGE_QMP:
            drawValue(24, "Temp", qmpTemp, "%7.2f", 1.0f, "C");
            drawValue(48, "Pres", qmpPres, "%7.1f", 0.01f, "hPa");
            drawValue(72, "Alt", qmpAlt, "%7.1f", 1.0f, "m");
            break;

        case PAGE_ACCEL:
            drawValue(24, "X", accelX, "%7.3f", 1.0f, "g");
            drawValue(48, "Y", accelY, "%7.3f", 1.0f, "g");
            drawValue(72, "Z", accelZ, "%7.3f", 1.0f, "g");
            break;

        case PAGE_GYRO:
            drawValue(24, "X", gyroX, "%7.1f", 1.0f, "dps");
            drawValue(48, "Y", gyroY, "%7.1f", 1.0f, "dps");
            drawValue(72, "Z", gyroZ, "%7.1f", 1.0f, "dps");
            break;

        case PAGE_POWER:
            drawValue(36, "Batt", batteryMv, "%7.2f", 0.001f, "V");
            break;

        case PAGE_MIC:
            display.setTextSize(2);
            display.setTextColor(WHITE, BLACK);

            display.setCursor(4, 28);

            display.printf(
                "Clips: %-6lu",
                (unsigned long)audioPlayerGetClipCount()
            );

            display.setCursor(4, 56);

            if (!audioPlayerIsReady()) {
                display.setTextColor(RED, BLACK);
                display.print("No memory   ");
            }
            else if (audioPlayerIsPlaying()) {
                display.setTextColor(GREEN, BLACK);
                display.print("Playing...  ");
            }
            else {
                display.setTextColor(LIGHTGREY, BLACK);
                display.print("Waiting     ");
            }
            break;
    }


    // ---- footer: data freshness ----

    display.setTextSize(1.5f);
    display.setCursor(0, 120);

    uint32_t updatedAt = sCurrentPage == PAGE_MIC
        ? audioPlayerGetClipAt()
        : pageUpdatedAt(sCurrentPage);

    if (updatedAt == 0) {
        display.setTextColor(LIGHTGREY, BLACK);

        display.print(
            sCurrentPage == PAGE_MIC
                ? "no clip yet           "
                : "no data yet           "
        );
    }
    else if (isPageStale(sCurrentPage, nowMs)) {
        display.setTextColor(RED, BLACK);

        display.printf(
            "STALE: %lus old        ",
            (unsigned long)((nowMs - updatedAt) / 1000)
        );
    }
    else {
        display.setTextColor(DARKGREY, BLACK);

        display.printf(
            "updated %.1fs ago      ",
            (nowMs - updatedAt) / 1000.0f
        );
    }
}


// ================= value rendering =================

// Draws one sensor value with its label and measurement unit.
static void drawValue(
    int y,
    const char* label,
    const Reading& reading,
    const char* format,
    float scale,
    const char* unit
) {
    auto& display = M5.Display;

    display.setTextSize(2);
    display.setCursor(4, y);

    display.setTextColor(LIGHTGREY, BLACK);
    display.printf("%-5s", label);

    display.setTextColor(WHITE, BLACK);

    if (reading.updatedAt == 0) {
        display.print("     --");
    }
    else {
        display.printf(format, reading.value * scale);
    }

    display.printf(" %-4s", unit);
}
