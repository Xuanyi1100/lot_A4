/*
 * FILE        : topics.h
 * PROJECT     : SENG3030 Assignment 4
 * PROGRAMMERS : Alexia Tu, Carlos Gomez Guerrero, Deyi Zou, Juan Jose Bejarano
 * DESCRIPTION : Every MQTT username and topic used by devices A, B and C.
 *               This is the ONLY place to change a topic. All three
 *               PlatformIO projects include this same file.
 */
#pragma once
#include <string>

// ===== Change these =====
const std::string SECTION   = "Wednesday";   // or "Friday"
const std::string USER_ACQ  = "atu8211";              // device A logs in with this
const std::string USER_DISP = "cgomezguerrero5580";   // device B
const std::string USER_CTRL = "dzou1734";             // device C
// ========================

const char* const MQTT_BROKER = "p07da41d.ala.us-east-1.emqxsl.com";
const int         MQTT_PORT   = 8883;

const std::string BASE_ACQ  = "SENG3030/" + SECTION + "/" + USER_ACQ  + "/";
const std::string BASE_DISP = "SENG3030/" + SECTION + "/" + USER_DISP + "/";
const std::string BASE_CTRL = "SENG3030/" + SECTION + "/" + USER_CTRL + "/";

// ---- published by A ----
const std::string T_SHT_TEMP   = BASE_ACQ + "sht30/temperature";
const std::string T_SHT_HUMI   = BASE_ACQ + "sht30/humidity";
const std::string T_QMP_TEMP   = BASE_ACQ + "qmp/temperature";
const std::string T_QMP_PRES   = BASE_ACQ + "qmp/pressure";
const std::string T_QMP_ALT    = BASE_ACQ + "qmp/altitude";
const std::string T_BATTERY    = BASE_ACQ + "battery";
const std::string T_IMU        = BASE_ACQ + "imu";
const std::string T_MIC_CLIP   = BASE_ACQ + "mic/clip";
const std::string T_ACQ_STATUS = BASE_ACQ + "status";

// ---- published by B ----
const std::string T_DISP_STATUS = BASE_DISP + "status";

// ---- published by C ----
const std::string T_CTRL_DATA    = BASE_CTRL + "control/data";     // "on" / "off", retained
const std::string T_CTRL_DISPLAY = BASE_CTRL + "control/display";  // "next" / "prev"
const std::string T_CTRL_AUDIO   = BASE_CTRL + "control/audio";    // "send"
const std::string T_CTRL_STATUS  = BASE_CTRL + "status";
