/*
 * FILE          : sensor_imu.cpp
 * PROJECT       : SENG3030 Assignment 4 - Device A
 * DESCRIPTION   : Reads and publishes IMU data.
 */

#include <M5Unified.h>

#include "sensor_imu.h"
#include "topics.h"
#include "net_common.h"


void publishImu() {
    M5.Imu.update();

    auto imuData = M5.Imu.getImuData();

    char buf[160];

    snprintf(
        buf,
        sizeof(buf),
        "{\"ax\":%.3f,\"ay\":%.3f,\"az\":%.3f,\"gx\":%.2f,\"gy\":%.2f,\"gz\":%.2f}",
        imuData.accel.x,
        imuData.accel.y,
        imuData.accel.z,
        imuData.gyro.x,
        imuData.gyro.y,
        imuData.gyro.z
    );

    netPublishText(T_IMU.c_str(), buf, false);
}
