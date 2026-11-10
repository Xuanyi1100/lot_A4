
/*
 * FILE        : audio_player.h
 * PROJECT     : SENG3030 Assignment 4 - Device B
 * DESCRIPTION : Audio playback module interface.
 */

#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <stdint.h>
#include <stddef.h>


// ================= constants =================

const size_t CLIP_BYTES = 24000 * sizeof(int16_t);
const uint32_t CLIP_SAMPLERATE = 22050;
const size_t AUDIO_MQTT_BUFFER = CLIP_BYTES + 512;


// ================= audio functions =================

void audioPlayerBegin();

void audioPlayerSetBufferStatus(bool bufferOk);

void audioPlayerHandleClip(
    char* topic,
    uint8_t* payload,
    unsigned int length
);

void audioPlayerTask();

bool audioPlayerIsPlaying();

bool audioPlayerIsReady();

uint32_t audioPlayerGetClipCount();

uint32_t audioPlayerGetClipAt();


#endif // AUDIO_PLAYER_H
