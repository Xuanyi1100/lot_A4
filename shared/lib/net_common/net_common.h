/*
 * FILE        : net_common.h
 * PROJECT     : SENG3030 Assignment 4
 * PROGRAMMERS : Alexia Tu, Carlos Gomez Guerrero, Deyi Zou, Juan Jose Bejarano
 * DESCRIPTION : WiFi + MQTT code shared by devices A, B and C.
 *               Handles: WiFi connect/retry, TLS with the CA cert, MQTT connect with
 *               Last Will, "online" status, timed retries (no blocking loops), and
 *               publish counters. Each device only supplies its login, status topic
 *               and two callbacks.
 *
 *               The API is plain C++ because the shared implementation uses
 *               PubSubClient and WiFiClientSecure.
 *
 *               WiFi credentials, broker, port and CA cert come from the shared
 *               secrets.h / topics.h / ca_cert.h.
 */

#ifndef NET_COMMON_H
#define NET_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


#define NET_WIFI_RETRY_MS 10000
#define NET_MQTT_RETRY_MS 5000
#define NET_KEEPALIVE_SEC 15
#define NET_DEFAULT_BUFFER 512


/* Called when a subscribed MQTT message arrives. */
typedef void (*NetMessageCallback)(
    char* topic,
    uint8_t* payload,
    unsigned int length
);

/* Called right after MQTT connects and "online" is published. */
typedef void (*NetConnectedCallback)(void);


typedef struct {
    const char* clientPrefix;
    const char* user;
    const char* pass;
    const char* statusTopic;

    size_t bufferSize;

    NetMessageCallback onMessage;
    NetConnectedCallback onConnected;

} NetConfig;


// ================= setup =================

bool netBegin(const NetConfig* config);


// ================= periodic work =================

void netUpdate(uint32_t nowMs);
void netWifiTask(uint32_t nowMs);
void netMqttTask(uint32_t nowMs);


// ================= state =================

bool netWifiOk(void);
bool netMqttOk(void);


// ================= publishing =================

bool netPublishText(const char* topic, const char* text, bool retained);
bool netPublishFloat(const char* topic, float value);
bool netSubscribe(const char* topic, uint8_t qos);


// ================= streamed publishing =================

bool netBeginPublish(const char* topic, size_t totalLength, bool retained);
size_t netWrite(const uint8_t* data, size_t length);
bool netEndPublish(void);


// ================= counters and last event =================

void netCountPublish(bool ok);
uint32_t netPubOk(void);
uint32_t netPubFail(void);
void netSetEvent(const char* text);
const char* netLastEvent(void);


#endif // NET_COMMON_H
