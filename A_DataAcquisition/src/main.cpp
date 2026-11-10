 /*
 * FILE          : main.cpp
 * PROJECT       : SENG3030 Assignment 4 - Device A (Data Acquisition)
 * PROGRAMMERS   : Alexia Tu, Carlos Gomez Guerrero, Deyi Zou, Juan Jose Bejarano
 * DATE          : 2026-11-10
 * DESCRIPTION   : This file is the Main entry point for Device A.
 *   Reads every sensor used in the labs and publishes it to the course MQTT
 *   broker under SENG3030/Wednesday/<USER_ACQ>/ :
 *     - ENV IV unit : SHT30 (temperature, humidity)
 *                     QMP6988 (temperature, pressure, altitude)      every 1 s
 *     - 6-axis IMU  : accelerometer + gyroscope, one JSON message     every 200 ms
 *     - battery     : voltage in mV                                   every 10 s
 *     - microphone  : records all the time into a ring buffer (as in Lab 4).
 *                     When device C sends "send" on control/audio, the whole
 *                     buffer is published once to mic/clip.
 *   Device C turns all publishing on or off with control/data ("on"/"off").
 *   "status" is "online" while connected; the broker publishes "offline"
 *   (Last Will) if this device drops off.
 *
 */

#include <Arduino.h>
#include "device_a.h"

void setup() {
    deviceABegin();
}

void loop() {
    deviceAUpdate(millis());
}
