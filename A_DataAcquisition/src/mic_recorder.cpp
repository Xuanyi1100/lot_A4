
/*
 * FILE          : mic_recorder.cpp
 * PROJECT       : SENG3030 Assignment 4 - Device A
 * DESCRIPTION   : Microphone recording and audio transmission.
 */

#include <Arduino.h>
#include <M5Unified.h>
#include <esp_heap_caps.h>

#include <cstring>

#include "mic_recorder.h"
#include "topics.h"
#include "net_common.h"

// ---------------- audio configuration ----------------

static const size_t RECORD_NUMBER = 20;
static const size_t RECORD_LENGTH = 1200;
static const size_t RECORD_SIZE = RECORD_NUMBER * RECORD_LENGTH;

static const uint32_t RECORD_SAMPLERATE = 22050;

static const size_t AUDIO_WRITE_CHUNK = 2048;

// ---------------- recording state ----------------

static int16_t* recData = nullptr;
static size_t recIdx = 0;


// ================= initialization =================

void setupMicRecorder() {
    recData = (int16_t*)heap_caps_malloc(
        RECORD_SIZE * sizeof(int16_t),
        MALLOC_CAP_8BIT
    );

    if (recData == nullptr) {
        Serial.println("Mic: not enough memory for the audio buffer");
        return;
    }

    memset(
        recData,
        0,
        RECORD_SIZE * sizeof(int16_t)
    );

    // The microphone and speaker cannot run at the same time.
    M5.Speaker.end();
    M5.Mic.begin();
}


// ================= recording =================

void recordAudio() {
    if (recData == nullptr || !M5.Mic.isEnabled()) {
        return;
    }

    if (M5.Mic.record(
        &recData[recIdx * RECORD_LENGTH],
        RECORD_LENGTH,
        RECORD_SAMPLERATE
    )) {
        recIdx = (recIdx + 1) % RECORD_NUMBER;
    }
}

// ================= audio writing =================

static bool writeAudio(const uint8_t* data, size_t length) {
    while (length > 0) {
        size_t chunk = length < AUDIO_WRITE_CHUNK
            ? length
            : AUDIO_WRITE_CHUNK;

        if (netWrite(data, chunk) != chunk) {
            return false;
        }

        data += chunk;
        length -= chunk;
    }

    return true;
}

// ================= transmission =================

void sendAudio() {
    if (recData == nullptr) {
        netSetEvent("no audio buffer");
        return;
    }

    // Wait for queued recordings to finish before accessing the buffer.
    while (M5.Mic.isRecording()) {
        yield();
    }

    M5.Mic.end();

    const size_t total = RECORD_SIZE * sizeof(int16_t);
    const size_t split = recIdx * RECORD_LENGTH;

    const uint8_t* bytes = (const uint8_t*)recData;

    // Stream the payload instead of allocating a second 48 KB buffer.
    bool ok = netBeginPublish(
        T_MIC_CLIP.c_str(),
        total,
        false
    );

    if (ok) {
        ok = writeAudio(
            bytes + split * sizeof(int16_t),
            (RECORD_SIZE - split) * sizeof(int16_t)
        ) && writeAudio(
            bytes,
            split * sizeof(int16_t)
        );

        ok = netEndPublish() && ok;
    }

    // Restart recording regardless of transmission success.
    M5.Mic.begin();

    netCountPublish(ok);
    netSetEvent(ok ? "audio sent" : "audio FAILED");

    Serial.printf(
        "Audio %u bytes: %s\n",
        (unsigned)total,
        ok ? "sent" : "FAILED"
    );
}

// ================= status =================

bool micRecorderAvailable() {
    return recData != nullptr;
}
