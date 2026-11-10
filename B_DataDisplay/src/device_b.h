
/*
 * FILE        : device_b.h
 * PROJECT     : SENG3030 Assignment 4 - Device B
 * DESCRIPTION : Device B MQTT callbacks and message routing declarations.
 */

#ifndef DEVICE_B_H
#define DEVICE_B_H

#include <stdint.h>


// ================= network callbacks =================

void onConnected();

void onMessage(
    char* topic,
    uint8_t* payload,
    unsigned int length
);


#endif // DEVICE_B_H
