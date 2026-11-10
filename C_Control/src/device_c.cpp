
/*
 * FILE        : device_c.cpp
 * PROJECT     : SENG3030 Assignment 4 - Device C (Control)
 * DESCRIPTION : Device C state and network callback implementation.
 */

#include "device_c.h"

#include "topics.h"
#include "net_common.h"

// ---------------- device state ----------------
bool dataOn = true;
String lastCmd = "-";
bool lastOk = true;

// ---------------- network callback ----------------

// Called by net_common after MQTT connects and publishes "online".
void onConnected() {
    // Make the retained setting match what this screen shows.
    netPublishText(T_CTRL_DATA.c_str(), dataOn ? "on" : "off", true);
}
