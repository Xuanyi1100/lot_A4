
/*
 * FILE        : device_c.h
 * PROJECT     : SENG3030 Assignment 4 - Device C (Control)
 * DESCRIPTION : Device C state and callback declarations.
 */

#ifndef DEVICE_C_H
#define DEVICE_C_H

#include <Arduino.h>

// ---------------- device state ----------------
extern bool dataOn;
extern String lastCmd;
extern bool lastOk;

// ---------------- network callback ----------------
void onConnected();

#endif
