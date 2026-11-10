/*
 * FILE        : net_common.cpp
 * PROJECT     : SENG3030 Assignment 4
 * PROGRAMMERS : Alexia Tu, Carlos Gomez Guerrero, Deyi Zou, Juan Jose Bejarano
 * DESCRIPTION : Implementation of net_common.h (WiFi + MQTT shared by A, B, C).
 */

#include "net_common.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "topics.h"
#include "secrets.h"
#include "ca_cert.h"


// ---------------- module state (private to this file) ----------------
static WiFiClientSecure sNet;
static PubSubClient sMqtt(sNet);

static NetConnectedCallback sOnConnected = nullptr;

// Copies of the config strings, so callers do not have to keep them alive.
static String sPrefix;
static String sUser;
static String sPass;
static String sStatus;

static String sEvent = "starting";

static uint32_t sTWifi = 0;
static uint32_t sTMqtt = 0;

static uint32_t sPubOk = 0;
static uint32_t sPubFail = 0;

static bool connectMqtt(void);


// ================= setup =================
bool netBegin(const NetConfig* config) {
    sPrefix = config->clientPrefix ? config->clientPrefix : "seng3030-";
    sUser = config->user ? config->user : "";
    sPass = config->pass ? config->pass : "";
    sStatus = config->statusTopic ? config->statusTopic : "";
    sOnConnected = config->onConnected;

    // Big buffer first, while the heap is still in one piece.
    size_t bufferSize = config->bufferSize
        ? config->bufferSize
        : NET_DEFAULT_BUFFER;

    bool bufferOk = sMqtt.setBufferSize(bufferSize);

    if (!bufferOk) {
        sMqtt.setBufferSize(1024);
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    sTWifi = millis();

    sNet.setCACert(CA_CERT);

    sMqtt.setServer(MQTT_BROKER, MQTT_PORT);
    sMqtt.setKeepAlive(NET_KEEPALIVE_SEC);
    sMqtt.setSocketTimeout(5);

    if (config->onMessage) {
        sMqtt.setCallback(config->onMessage);
    }

    return bufferOk;
}


// ================= periodic work =================

void netUpdate(uint32_t nowMs) {
    netWifiTask(nowMs);
    netMqttTask(nowMs);
}

void netWifiTask(uint32_t nowMs) {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    if (nowMs - sTWifi >= NET_WIFI_RETRY_MS) {
        sTWifi = nowMs;

        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        sEvent = "WiFi retry";
    }
}

void netMqttTask(uint32_t nowMs) {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    if (!sMqtt.connected()) {
        if (nowMs - sTMqtt >= NET_MQTT_RETRY_MS) {
            sTMqtt = nowMs;
            connectMqtt();
        }

        return;
    }

    sMqtt.loop();
}

static bool connectMqtt(void) {
    String clientId = sPrefix + WiFi.macAddress();

    // Last Will: broker publishes "offline" if this device disconnects.
    bool ok = sMqtt.connect(
        clientId.c_str(),
        sUser.c_str(),
        sPass.c_str(),
        sStatus.c_str(),
        1,
        true,
        "offline"
    );

    if (!ok) {
        sEvent = "MQTT fail rc=" + String(sMqtt.state());
        Serial.println(sEvent);
        return false;
    }

    netPublishText(sStatus.c_str(), "online", true);

    sEvent = "MQTT connected";

    Serial.printf(
        "MQTT connected. Free heap: %lu bytes\n",
        (unsigned long)ESP.getFreeHeap()
    );

    if (sOnConnected) {
        sOnConnected();
    }

    return true;
}


// ================= state =================
bool netWifiOk(void) {
    return WiFi.status() == WL_CONNECTED;
}

bool netMqttOk(void) {
    return sMqtt.connected();
}

// ================= publishing =================
bool netPublishText(const char* topic, const char* text, bool retained) {
    // True only means the bytes left this device (QoS 0).
    bool ok = sMqtt.connected() && sMqtt.publish(topic, text, retained);

    netCountPublish(ok);

    return ok;
}

bool netPublishFloat(const char* topic, float value) {
    char buf[24];

    snprintf(buf, sizeof(buf), "%.2f", value);

    return netPublishText(topic, buf, true);
}

bool netSubscribe(const char* topic, uint8_t qos) {
    return sMqtt.subscribe(topic, qos);
}

bool netBeginPublish(const char* topic, size_t totalLength, bool retained) {
    return sMqtt.beginPublish(
        topic,
        (unsigned int)totalLength,
        retained
    );
}

size_t netWrite(const uint8_t* data, size_t length) {
    return sMqtt.write(data, length);
}

bool netEndPublish(void) {
    return sMqtt.endPublish() == 1;
}


// ================= counters and last event =================
void netCountPublish(bool ok) {
    if (ok) {
        sPubOk++;
    } else {
        sPubFail++;
    }
}

uint32_t netPubOk(void) {
    return sPubOk;
}

uint32_t netPubFail(void) {
    return sPubFail;
}

void netSetEvent(const char* text) {
    sEvent = text;
}

const char* netLastEvent(void) {
    return sEvent.c_str();
}
