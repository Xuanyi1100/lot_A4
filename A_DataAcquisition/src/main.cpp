/*
 * FILE          : main.cpp
 * PROJECT       : SENG3030 Assignment 4 - Device A (Data Acquisition)
 * PROGRAMMERS   : Alexia Tu, Carlos Gomez Guerrero, Deyi Zou, Juan Jose Bejarano
 * FIRST VERSION : 2026-11-04
 * DESCRIPTION   :
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
 *   The loop never blocks: each task runs on its own millis() timer and
 *   delay() is not used anywhere.
 *
 *   mic/clip payload: raw PCM, signed 16-bit little-endian, mono, 44100 Hz,
 *   48000 samples = 96000 bytes (about 1.1 s), oldest sample first.
 */

#include <M5Unified.h>
#include <M5UnitENV.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "topics.h"
#include "secrets.h"
#include "ca_cert.h"

// ---------------- hardware ----------------
#define EXT_SDA 32   // Grove port I2C pins for the ENV IV unit
#define EXT_SCL 33

// ---------------- timing (ms) ----------------
const uint32_t ENV_PERIOD     = 1000;
const uint32_t IMU_PERIOD     = 200;
const uint32_t BATTERY_PERIOD = 10000;
const uint32_t SCREEN_PERIOD  = 500;
const uint32_t WIFI_RETRY     = 10000;
const uint32_t MQTT_RETRY     = 5000;
const uint16_t KEEPALIVE_SEC = 15;     // seconds; Last Will fires ~1.5x this after a drop

// ---------------- audio (same sizes as Lab 4) ----------------
const size_t   RECORD_NUMBER     = 200;                            // chunks in the ring buffer
const size_t   RECORD_LENGTH     = 240;                            // samples per chunk
const size_t   RECORD_SIZE       = RECORD_NUMBER * RECORD_LENGTH;  // 48000 samples
const uint32_t RECORD_SAMPLERATE = 44100;
const size_t   AUDIO_WRITE_CHUNK = 2048;                           // bytes per network write

// ---------------- objects and state ----------------
WiFiClientSecure net;
PubSubClient     mqtt(net);

SHT3X   sht3x;
QMP6988 qmp;
bool    sht_ok = false;
bool    qmp_ok = false;

int16_t* rec_data = nullptr;   // ring buffer, 96000 bytes
size_t   rec_idx  = 0;         // next chunk to fill = oldest chunk

bool data_enabled    = true;   // set by C via control/data (retained)
bool audio_requested = false;  // set by C via control/audio

uint32_t t_env = 0, t_imu = 0, t_bat = 0, t_screen = 0, t_wifi = 0, t_mqtt = 0;
uint32_t pub_ok = 0, pub_fail = 0;
String   last_event = "starting";

// ---------------- prototypes ----------------
void setupEnv();
void setupMic();
void wifiTask(uint32_t now);
void mqttTask(uint32_t now);
bool connectMqtt();
void onMessage(char* topic, byte* payload, unsigned int length);
void publishEnv();
void publishImu();
void publishBattery();
void recordAudio();
void sendAudio();
bool writeAudio(const uint8_t* p, size_t n);
void publishText(const std::string& topic, const char* text, bool retained);
void publishFloat(const std::string& topic, float value);
void drawScreen();


void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);

    M5.Display.setRotation(1);
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextSize(1.5f);

    setupMic();   // take the 96 KB buffer first, before WiFi and TLS use the heap
    setupEnv();

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    t_wifi = millis();

    net.setCACert(CA_CERT);
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt.setKeepAlive(KEEPALIVE_SEC);
    mqtt.setSocketTimeout(5);
    mqtt.setBufferSize(512);
    mqtt.setCallback(onMessage);

    Serial.printf("Setup done. Free heap: %lu bytes\n", (unsigned long)ESP.getFreeHeap());
}


void loop()
{
    M5.update();
    uint32_t now = millis();

    wifiTask(now);
    mqttTask(now);
    recordAudio();

    if (mqtt.connected() && data_enabled) {
        if (now - t_env >= ENV_PERIOD)     { t_env = now; publishEnv(); }
        if (now - t_imu >= IMU_PERIOD)     { t_imu = now; publishImu(); }
        if (now - t_bat >= BATTERY_PERIOD) { t_bat = now; publishBattery(); }
        if (audio_requested)               { sendAudio(); }
    } else {
        audio_requested = false;   // data is off or offline: ignore audio requests
    }

    if (now - t_screen >= SCREEN_PERIOD) { t_screen = now; drawScreen(); }
}


// ================= setup helpers =================

void setupEnv()
{
    qmp_ok = qmp.begin(&Wire, QMP6988_SLAVE_ADDRESS_L, EXT_SDA, EXT_SCL, 400000U);
    sht_ok = sht3x.begin(&Wire, SHT3X_I2C_ADDR, EXT_SDA, EXT_SCL, 400000U);
    Serial.printf("QMP6988: %s   SHT30: %s\n", qmp_ok ? "ok" : "NOT FOUND", sht_ok ? "ok" : "NOT FOUND");
}

void setupMic()
{
    rec_data = (int16_t*)heap_caps_malloc(RECORD_SIZE * sizeof(int16_t), MALLOC_CAP_8BIT);
    if (rec_data == nullptr) {
        Serial.println("Mic: not enough memory for the audio buffer");
        return;
    }
    memset(rec_data, 0, RECORD_SIZE * sizeof(int16_t));

    // The microphone and the speaker cannot run at the same time (Lab 4).
    M5.Speaker.end();
    M5.Mic.begin();
}


// ================= network =================

void wifiTask(uint32_t now)
{
    if (WiFi.status() == WL_CONNECTED) return;
    if (now - t_wifi >= WIFI_RETRY) {
        t_wifi = now;
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        last_event = "WiFi retry";
    }
}

void mqttTask(uint32_t now)
{
    if (WiFi.status() != WL_CONNECTED) return;

    if (!mqtt.connected()) {
        if (now - t_mqtt >= MQTT_RETRY) {   // retry on a timer, never in a while() loop
            t_mqtt = now;
            connectMqtt();
        }
        return;
    }
    mqtt.loop();   // keep-alive and incoming messages
}

bool connectMqtt()
{
    String client_id = "seng3030-A-" + WiFi.macAddress();

    // Last Will: if we drop off, the broker publishes "offline" (retained) for us.
    bool ok = mqtt.connect(client_id.c_str(), USER_ACQ.c_str(), MQTT_PASS_ACQ,
                           T_ACQ_STATUS.c_str(), 1, true, "offline");
    if (!ok) {
        last_event = "MQTT fail rc=" + String(mqtt.state());
        Serial.println(last_event);
        return false;
    }

    publishText(T_ACQ_STATUS, "online", true);
    mqtt.subscribe(T_CTRL_DATA.c_str(), 1);
    mqtt.subscribe(T_CTRL_AUDIO.c_str(), 1);

    // publish every value right away instead of waiting a full period
    uint32_t now = millis();
    t_env = now - ENV_PERIOD;
    t_imu = now - IMU_PERIOD;
    t_bat = now - BATTERY_PERIOD;

    last_event = "MQTT connected";
    Serial.printf("MQTT connected. Free heap: %lu bytes\n", (unsigned long)ESP.getFreeHeap());
    return true;
}

// Called from mqtt.loop() when a subscribed message arrives.
void onMessage(char* topic, byte* payload, unsigned int length)
{
    String msg;
    msg.reserve(length);
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
    msg.trim();

    if (T_CTRL_DATA == topic) {
        if (msg == "on")  data_enabled = true;
        if (msg == "off") data_enabled = false;
        last_event = "data " + msg;
    }
    else if (T_CTRL_AUDIO == topic && msg == "send") {
        audio_requested = true;   // sent from loop(), not from inside this callback
    }
    Serial.printf("RX %s = %s\n", topic, msg.c_str());
}


// ================= sensors =================

void publishEnv()
{
    if (sht_ok && sht3x.update()) {
        publishFloat(T_SHT_TEMP, sht3x.cTemp);
        publishFloat(T_SHT_HUMI, sht3x.humidity);
    }
    if (qmp_ok && qmp.update()) {
        publishFloat(T_QMP_TEMP, qmp.cTemp);
        publishFloat(T_QMP_PRES, qmp.pressure);
        publishFloat(T_QMP_ALT,  qmp.altitude);
    }
}

void publishImu()
{
    M5.Imu.update();
    auto d = M5.Imu.getImuData();

    char buf[160];
    snprintf(buf, sizeof(buf),
             "{\"ax\":%.3f,\"ay\":%.3f,\"az\":%.3f,\"gx\":%.2f,\"gy\":%.2f,\"gz\":%.2f}",
             d.accel.x, d.accel.y, d.accel.z, d.gyro.x, d.gyro.y, d.gyro.z);
    publishText(T_IMU, buf, false);   // fast-changing: not retained
}

void publishBattery()
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", (int)M5.Power.getBatteryVoltage());
    publishText(T_BATTERY, buf, true);
}


// ================= audio =================

// Queue the next chunk of the ring buffer for recording (same idea as Lab 4).
void recordAudio()
{
    if (rec_data == nullptr || !M5.Mic.isEnabled()) return;

    if (M5.Mic.record(&rec_data[rec_idx * RECORD_LENGTH], RECORD_LENGTH, RECORD_SAMPLERATE)) {
        rec_idx = (rec_idx + 1) % RECORD_NUMBER;
    }
}

// Publish the whole ring buffer as one message, oldest sample first.
void sendAudio()
{
    audio_requested = false;
    if (rec_data == nullptr) { last_event = "no audio buffer"; return; }

    // Let queued recordings finish, then stop the mic so the buffer stays still.
    while (M5.Mic.isRecording()) { yield(); }
    M5.Mic.end();

    const size_t total  = RECORD_SIZE * sizeof(int16_t);
    const size_t split  = rec_idx * RECORD_LENGTH;               // first sample of oldest chunk
    const uint8_t* bytes = (const uint8_t*)rec_data;

    // beginPublish/write/endPublish streams the payload, so no 96 KB MQTT buffer is needed.
    bool ok = mqtt.beginPublish(T_MIC_CLIP.c_str(), total, false);
    if (ok) {
        ok = writeAudio(bytes + split * 2, (RECORD_SIZE - split) * 2)   // oldest .. end
          && writeAudio(bytes, split * 2);                              // start .. newest
        ok = (mqtt.endPublish() == 1) && ok;
    }

    M5.Mic.begin();
    ok ? pub_ok++ : pub_fail++;
    last_event = ok ? "audio sent" : "audio FAILED";
    Serial.printf("Audio %u bytes: %s\n", (unsigned)total, ok ? "sent" : "FAILED");
}

bool writeAudio(const uint8_t* p, size_t n)
{
    while (n > 0) {
        size_t chunk = n < AUDIO_WRITE_CHUNK ? n : AUDIO_WRITE_CHUNK;
        if (mqtt.write(p, chunk) != chunk) return false;
        p += chunk;
        n -= chunk;
    }
    return true;
}


// ================= helpers =================

void publishText(const std::string& topic, const char* text, bool retained)
{
    // publish() returning true only means the bytes left this device (QoS 0).
    if (mqtt.publish(topic.c_str(), text, retained)) pub_ok++;
    else                                            pub_fail++;
}

void publishFloat(const std::string& topic, float value)
{
    char buf[24];
    snprintf(buf, sizeof(buf), "%.2f", value);
    publishText(topic, buf, true);   // slow-changing values are retained
}

void drawScreen()
{
    auto& d = M5.Display;
    d.setCursor(0, 0);
    d.setTextColor(CYAN, BLACK);
    d.printf("A: DATA ACQUISITION\n");
    d.setTextColor(WHITE, BLACK);
    d.printf("WiFi : %-14s\n", WiFi.status() == WL_CONNECTED ? "connected" : "connecting");
    d.printf("MQTT : %-14s\n", mqtt.connected() ? "connected" : "connecting");
    d.setTextColor(data_enabled ? GREEN : RED, BLACK);
    d.printf("Data : %-14s\n", data_enabled ? "ON" : "OFF");
    d.setTextColor(WHITE, BLACK);
    d.printf("ENV  : %-14s\n", (sht_ok && qmp_ok) ? "ok" : "check cable");
    d.printf("Mic  : %-14s\n", rec_data ? "recording" : "no memory");
    d.printf("Sent : %lu  fail %lu   \n", (unsigned long)pub_ok, (unsigned long)pub_fail);
    d.printf("Last : %-14s\n", last_event.c_str());
}
