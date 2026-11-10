
/*
 * FILE        : commands.h
 * PROJECT     : SENG3030 Assignment 4 - Device C (Control)
 * DESCRIPTION : Button handling and command declarations.
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include <Arduino.h>

void handleButtons();

bool sendCommand(
    const std::string& topic,
    const char* payload,
    bool retained,
    const char* label
);

void beep(bool ok);

#endif
