/*
 * FILE          : main.cpp
 * PROJECT       : SENG3030 Assignment 4 - Device B (Data Display)
 * PROGRAMMERS   : Alexia Tu, Carlos Gomez Guerrero, Deyi Zou, Juan Jose Bejarano
 * FIRST VERSION : 2026-11-05
 * DESCRIPTION   :
 *   Subscribes to everything device A publishes (SENG3030/Wednesday/<USER_ACQ>/#)
 *   and shows it on the LCD, one sensor per page:
 *       1 SHT30   2 QMP6988   3 Accel   4 Gyro   5 Power   6 Mic
 *   - Front button (BtnA) = next page, side button (BtnB) = previous page.
 *   - Device C can also change the page: "next" / "prev" on <USER_CTRL>/control/display.
 *   - Every new audio clip from A (mic/clip) is played on the speaker.
 *   - A page turns red ("STALE") when its data is older than 5 s (25 s for the
 *     battery, which A sends only every 10 s), e.g. when
 *     C has turned A's data off or A has dropped off.
 *   - Publishes its own "status" (online, with Last Will "offline").
 *
 *   Audio without a second 48 KB buffer:
 *   the MQTT receive buffer is made large enough for one clip. While the clip
 *   plays (~1.1 s), loop() does not call mqtt.loop(), so the clip stays in that
 *   buffer. New messages wait in the network stack and arrive right after.
 *   The loop never blocks and delay() is not used anywhere.
 */

#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <math.h>

#include "topics.h"
#include "secrets.h"
#include "ca_cert.h"

// ---------------- timing (ms) ----------------
const uint32_t SCREEN_PERIOD  = 250;
const uint32_t STALE_AFTER    = 5000;    // data sent every 0.2-1 s
const uint32_t STALE_BATTERY  = 25000;   // battery is sent every 10 s: allow one lost message
const uint32_t WIFI_RETRY     = 10000;
const uint32_t MQTT_RETRY     = 5000;
const uint16_t KEEPALIVE_SEC  = 15;

// ---------------- audio (must match device A) ----------------
const size_t   CLIP_BYTES      = 24000 * sizeof(int16_t);   // 48000 bytes
const uint32_t CLIP_SAMPLERATE = 22050;
const size_t   MQTT_BUFFER     = CLIP_BYTES + 512;          // clip + MQTT header + topic

// ---------------- pages ----------------
enum Page { PAGE_SHT, PAGE_QMP, PAGE_ACCEL, PAGE_GYRO, PAGE_POWER, PAGE_MIC, PAGE_COUNT };
const char* const PAGE_NAMES[PAGE_COUNT] = { "SHT30", "QMP6988", "Accel", "Gyro", "Power", "Mic" };

// ---------------- received data ----------------
struct Reading {
    float    value = NAN;
    uint32_t at    = 0;      // millis() when received, 0 = never
};

Reading sht_temp, sht_humi;
Reading qmp_temp, qmp_pres, qmp_alt;
Reading ax, ay, az, gx, gy, gz;
Reading battery_mv;

bool     a_status_known = false;
bool     a_online       = false;

uint32_t clip_count     = 0;
uint32_t clip_at        = 0;
bool     clip_pending   = false;   // set in callback, played in loop()
bool     clip_playing   = false;
uint8_t* clip_ptr       = nullptr; // payload inside the MQTT buffer
size_t   clip_len       = 0;
bool     audio_ok       = false;   // big MQTT buffer allocated

// ---------------- objects and state ----------------
WiFiClientSecure net;
PubSubClient     mqtt(net);

int      page        = PAGE_SHT;
bool     page_changed = true;      // full redraw needed
uint32_t t_screen = 0, t_wifi = 0, t_mqtt = 0;

// ---------------- prototypes ----------------
void wifiTask(uint32_t now);
void mqttTask(uint32_t now);
bool connectMqtt();
void onMessage(char* topic, byte* payload, unsigned int length);
void handleClip(char* topic, byte* payload, unsigned int length);
void audioTask();
void changePage(int step);
void setReading(Reading& r, const char* text);
uint32_t pageUpdatedAt();
void drawScreen(uint32_t now);
void drawValue(int y, const char* label, const Reading& r, const char* fmt, float scale, const char* unit);


void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);

    // Allocate the big MQTT buffer first, while the heap is still in one piece.
    audio_ok = mqtt.setBufferSize(MQTT_BUFFER);
    if (!audio_ok) mqtt.setBufferSize(1024);   // still show sensor data without audio
    Serial.printf("MQTT buffer %s. Free heap: %lu bytes\n",
                  audio_ok ? "48 KB ok" : "FAILED (audio off)", (unsigned long)ESP.getFreeHeap());

    // B only plays sound, so the microphone is not used.
    M5.Mic.end();
    M5.Speaker.begin();
    M5.Speaker.setVolume(255);

    M5.Display.setRotation(1);
    M5.Display.fillScreen(BLACK);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    t_wifi = millis();

    net.setCACert(CA_CERT);
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt.setKeepAlive(KEEPALIVE_SEC);
    mqtt.setSocketTimeout(5);
    mqtt.setCallback(onMessage);
}


void loop()
{
    M5.update();
    uint32_t now = millis();

    if (M5.BtnA.wasPressed()) changePage(+1);
    if (M5.BtnB.wasPressed()) changePage(-1);

    wifiTask(now);
    audioTask();
    if (!clip_playing) mqttTask(now);   // pause MQTT while the clip in its buffer is playing

    if (page_changed || now - t_screen >= SCREEN_PERIOD) {
        t_screen = now;
        drawScreen(now);
    }
}


// ================= network =================

void wifiTask(uint32_t now)
{
    if (WiFi.status() == WL_CONNECTED) return;
    if (now - t_wifi >= WIFI_RETRY) {
        t_wifi = now;
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
}

void mqttTask(uint32_t now)
{
    if (WiFi.status() != WL_CONNECTED) return;

    if (!mqtt.connected()) {
        if (now - t_mqtt >= MQTT_RETRY) {
            t_mqtt = now;
            connectMqtt();
        }
        return;
    }
    mqtt.loop();
}

bool connectMqtt()
{
    String client_id = "seng3030-B-" + WiFi.macAddress();

    bool ok = mqtt.connect(client_id.c_str(), USER_DISP.c_str(), MQTT_PASS_DISP,
                           T_DISP_STATUS.c_str(), 1, true, "offline");
    if (!ok) {
        Serial.printf("MQTT connect failed, rc=%d\n", mqtt.state());
        return false;
    }

    mqtt.publish(T_DISP_STATUS.c_str(), "online", true);
    mqtt.subscribe((BASE_ACQ + "#").c_str());        // all of A's data
    mqtt.subscribe(T_CTRL_DISPLAY.c_str(), 1);       // page commands from C
    Serial.printf("MQTT connected. Free heap: %lu bytes\n", (unsigned long)ESP.getFreeHeap());
    return true;
}

// Called from mqtt.loop() for every message on a subscribed topic.
void onMessage(char* topic, byte* payload, unsigned int length)
{
    if (T_MIC_CLIP == topic) {
        handleClip(topic, payload, length);
        return;
    }

    // All other messages are short text.
    char msg[200];
    size_t n = length < sizeof(msg) - 1 ? length : sizeof(msg) - 1;
    memcpy(msg, payload, n);
    msg[n] = '\0';

    if (T_CTRL_DISPLAY == topic) {
        if (strcmp(msg, "next") == 0) changePage(+1);
        if (strcmp(msg, "prev") == 0) changePage(-1);
    }
    else if (T_SHT_TEMP == topic) setReading(sht_temp, msg);
    else if (T_SHT_HUMI == topic) setReading(sht_humi, msg);
    else if (T_QMP_TEMP == topic) setReading(qmp_temp, msg);
    else if (T_QMP_PRES == topic) setReading(qmp_pres, msg);
    else if (T_QMP_ALT  == topic) setReading(qmp_alt,  msg);
    else if (T_BATTERY  == topic) setReading(battery_mv, msg);
    else if (T_ACQ_STATUS == topic) {
        a_status_known = true;
        a_online = (strcmp(msg, "online") == 0);
    }
    else if (T_IMU == topic) {
        JsonDocument doc;
        if (deserializeJson(doc, msg) == DeserializationError::Ok) {
            uint32_t now = millis();
            ax.value = doc["ax"] | NAN;  ay.value = doc["ay"] | NAN;  az.value = doc["az"] | NAN;
            gx.value = doc["gx"] | NAN;  gy.value = doc["gy"] | NAN;  gz.value = doc["gz"] | NAN;
            ax.at = ay.at = az.at = gx.at = gy.at = gz.at = now;
        }
    }
}


// ================= audio =================

// Remember where the clip is; it is played from loop(), not from this callback.
void handleClip(char* topic, byte* payload, unsigned int length)
{
    if (!audio_ok || length == 0 || (length % 2) != 0) return;

    // int16 samples must start on an even address. The topic string sits just
    // before the payload in the same buffer and is no longer needed, so move
    // the payload down to a 4-byte boundary at or before the topic.
    uint8_t* aligned = (uint8_t*)((uintptr_t)topic & ~(uintptr_t)3);
    memmove(aligned, payload, length);

    clip_ptr     = aligned;
    clip_len     = length;
    clip_pending = true;
    clip_count++;
    clip_at = millis();
}

void audioTask()
{
    if (clip_pending) {
        clip_pending = false;
        clip_playing = M5.Speaker.playRaw((const int16_t*)clip_ptr, clip_len / 2,
                                          CLIP_SAMPLERATE, false, 1, -1, true);
        if (page == PAGE_MIC) page_changed = true;
    }
    else if (clip_playing && !M5.Speaker.isPlaying()) {
        clip_playing = false;   // buffer may be reused now: MQTT resumes
        if (page == PAGE_MIC) page_changed = true;
    }
}


// ================= pages =================

void changePage(int step)
{
    page = (page + step + PAGE_COUNT) % PAGE_COUNT;
    page_changed = true;
}

void setReading(Reading& r, const char* text)
{
    r.value = atof(text);
    r.at    = millis();
}

// Time of the oldest value shown on the current page (0 = something never arrived).
uint32_t pageUpdatedAt()
{
    switch (page) {
        case PAGE_SHT:   return min(sht_temp.at, sht_humi.at);
        case PAGE_QMP:   return min(qmp_temp.at, min(qmp_pres.at, qmp_alt.at));
        case PAGE_ACCEL: return ax.at;
        case PAGE_GYRO:  return gx.at;
        case PAGE_POWER: return battery_mv.at;
        default:         return clip_at;
    }
}

void drawScreen(uint32_t now)
{
    auto& d = M5.Display;
    if (page_changed) {
        d.fillScreen(BLACK);
        page_changed = false;
    }

    // ---- header: page number, page name, A online/offline ----
    d.setTextSize(1.5f);
    d.setCursor(0, 0);
    d.setTextColor(CYAN, BLACK);
    d.printf("%d/%d %-8s", page + 1, PAGE_COUNT, PAGE_NAMES[page]);
    d.setCursor(130, 0);
    if (!mqtt.connected())   { d.setTextColor(ORANGE, BLACK); d.print("MQTT ...  "); }
    else if (!a_status_known){ d.setTextColor(LIGHTGREY, BLACK); d.print("A: ?      "); }
    else if (a_online)       { d.setTextColor(GREEN, BLACK); d.print("A: online "); }
    else                     { d.setTextColor(RED, BLACK);   d.print("A: offline"); }
    d.drawFastHLine(0, 16, 240, DARKGREY);

    // ---- values ----
    switch (page) {
        case PAGE_SHT:
            drawValue(28, "Temp", sht_temp, "%7.2f", 1.0f, "C");
            drawValue(56, "Humi", sht_humi, "%7.2f", 1.0f, "%rH");
            break;
        case PAGE_QMP:
            drawValue(24, "Temp", qmp_temp, "%7.2f", 1.0f,   "C");
            drawValue(48, "Pres", qmp_pres, "%7.1f", 0.01f,  "hPa");
            drawValue(72, "Alt",  qmp_alt,  "%7.1f", 1.0f,   "m");
            break;
        case PAGE_ACCEL:
            drawValue(24, "X", ax, "%7.3f", 1.0f, "g");
            drawValue(48, "Y", ay, "%7.3f", 1.0f, "g");
            drawValue(72, "Z", az, "%7.3f", 1.0f, "g");
            break;
        case PAGE_GYRO:
            drawValue(24, "X", gx, "%7.1f", 1.0f, "dps");
            drawValue(48, "Y", gy, "%7.1f", 1.0f, "dps");
            drawValue(72, "Z", gz, "%7.1f", 1.0f, "dps");
            break;
        case PAGE_POWER:
            drawValue(36, "Batt", battery_mv, "%7.2f", 0.001f, "V");
            break;
        case PAGE_MIC:
            d.setTextSize(2);
            d.setTextColor(WHITE, BLACK);
            d.setCursor(4, 28);
            d.printf("Clips: %-6lu", (unsigned long)clip_count);
            d.setCursor(4, 56);
            if (!audio_ok)          { d.setTextColor(RED, BLACK);    d.print("No memory   "); }
            else if (clip_playing)  { d.setTextColor(GREEN, BLACK);  d.print("Playing...  "); }
            else                    { d.setTextColor(LIGHTGREY, BLACK); d.print("Waiting     "); }
            break;
    }

    // ---- footer: how old the data on this page is ----
    d.setTextSize(1.5f);
    d.setCursor(0, 120);
    uint32_t at = pageUpdatedAt();
    if (at == 0) {
        d.setTextColor(LIGHTGREY, BLACK);
        d.print(page == PAGE_MIC ? "no clip yet           " : "no data yet           ");
    } else if (page != PAGE_MIC && now - at > (page == PAGE_POWER ? STALE_BATTERY : STALE_AFTER)) {
        d.setTextColor(RED, BLACK);
        d.printf("STALE: %lus old        ", (unsigned long)((now - at) / 1000));
    } else {
        d.setTextColor(DARKGREY, BLACK);
        d.printf("updated %.1fs ago      ", (now - at) / 1000.0f);
    }
}

// One line: label, value (scaled and formatted), unit. "--" if never received.
void drawValue(int y, const char* label, const Reading& r, const char* fmt, float scale, const char* unit)
{
    auto& d = M5.Display;
    d.setTextSize(2);
    d.setCursor(4, y);
    d.setTextColor(LIGHTGREY, BLACK);
    d.printf("%-5s", label);
    d.setTextColor(WHITE, BLACK);
    if (r.at == 0) d.print("     --");
    else           d.printf(fmt, r.value * scale);
    d.printf(" %-4s", unit);
}
