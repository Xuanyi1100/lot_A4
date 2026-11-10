
/*
 * FILE        : audio_player.cpp
 * PROJECT     : SENG3030 Assignment 4 - Device B
 * DESCRIPTION : Audio buffer management and speaker playback.
 */

#include "audio_player.h"

#include <Arduino.h>
#include <M5Unified.h>
#include <string.h>


// ================= audio state =================

static bool sAudioOk = false;

static uint32_t sClipCount = 0;
static uint32_t sClipAt = 0;

static bool sClipPending = false;
static bool sClipPlaying = false;

static uint8_t* sClipPtr = nullptr;
static size_t sClipLen = 0;


// ================= initialization =================

void audioPlayerBegin() {
    M5.Speaker.begin();
    M5.Speaker.setVolume(255);
}


void audioPlayerSetBufferStatus(bool bufferOk) {
    sAudioOk = bufferOk;
}


// ================= receiving audio =================

// Stores the clip in the MQTT buffer for playback in audioPlayerTask().
void audioPlayerHandleClip(
    char* topic,
    uint8_t* payload,
    unsigned int length
) {
    if (!sAudioOk || length == 0 || (length % 2) != 0) {
        return;
    }

    // Align the payload to a 4-byte boundary.
    // The original topic data is no longer needed.
    uint8_t* alignedPtr = (uint8_t*)((uintptr_t)topic & ~(uintptr_t)3);

    memmove(alignedPtr, payload, length);

    sClipPtr = alignedPtr;
    sClipLen = length;

    sClipPending = true;

    sClipCount++;
    sClipAt = millis();
}


// ================= playback =================

void audioPlayerTask() {
    if (sClipPending) {
        sClipPending = false;

        sClipPlaying = M5.Speaker.playRaw(
            (const int16_t*)sClipPtr,
            sClipLen / 2,
            CLIP_SAMPLERATE,
            false,
            1,
            -1,
            true
        );
    }
    else if (sClipPlaying && !M5.Speaker.isPlaying()) {
        sClipPlaying = false;
    }
}


// ================= getters =================

bool audioPlayerIsPlaying() {
    return sClipPlaying;
}


bool audioPlayerIsReady() {
    return sAudioOk;
}


uint32_t audioPlayerGetClipCount() {
    return sClipCount;
}


uint32_t audioPlayerGetClipAt() {
    return sClipAt;
}
