/*
 * FILE          : main.cpp
 * PROJECT       : SENG3030 Assignment 4 - Device C (Control)
 * PROGRAMMERS   : Alexia Tu, Carlos Gomez Guerrero, Deyi Zou, Juan Jose Bejarano
 * FIRST VERSION : 2026-11-05
 * DESCRIPTION   :
 *   Controls device A and device B through MQTT. Publishes under
 *   SENG3030/Wednesday/<USER_CTRL>/ :
 *
 *     Button              Topic             Payload        Retained   Effect
 *     BtnA click          control/display   "next"         no         B shows the next page
 *     BtnB click          control/display   "prev"         no         B shows the previous page
 *     BtnA hold           control/data      "on" / "off"   yes        A starts / stops publishing
 *     BtnB hold           control/audio     "send"         no         A publishes its audio buffer
 *
 *   control/data is retained so A gets the last setting after a reboot.
 *   When C connects, it publishes its own setting (ON after power-up).
 *   "status" is "online" while connected; Last Will publishes "offline".
 *   The loop never blocks and delay() is not used anywhere.
 */

#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "topics.h"
#include "secrets.h"
#include "ca_cert.h"

// ---------------- timing ----------------
const uint32_t SCREEN_PERIOD = 250;     // ms
const uint32_t WIFI_RETRY    = 10000;   // ms
const uint32_t MQTT_RETRY    = 5000;    // ms
const uint16_t KEEPALIVE_SEC = 15;
const uint32_t HOLD_MS       = 800;     // press longer than this = "hold"

// ---------------- objects and state ----------------
WiFiClientSecure net;
PubSubClient     mqtt(net);

bool     data_on    = true;   // what C wants A to do
String   last_cmd   = "-";
bool     last_ok    = true;
uint32_t t_screen = 0, t_wifi = 0, t_mqtt = 0;

// ---------------- prototypes ----------------
void wifiTask(uint32_t now);
void mqttTask(uint32_t now);
bool connectMqtt();
void handleButtons();
bool sendCommand(const std::string& topic, const char* payload, bool retained, const char* label);
void beep(bool ok);
void drawScreen();


void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);

    M5.BtnA.setHoldThresh(HOLD_MS);
    M5.BtnB.setHoldThresh(HOLD_MS);
    M5.Speaker.setVolume(80);   // short beeps as button feedback

    M5.Display.setRotation(1);
    M5.Display.fillScreen(BLACK);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    t_wifi = millis();

    net.setCACert(CA_CERT);
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt.setKeepAlive(KEEPALIVE_SEC);
    mqtt.setSocketTimeout(5);
}


void loop()
{
    M5.update();
    uint32_t now = millis();

    wifiTask(now);
    mqttTask(now);
    handleButtons();

    if (now - t_screen >= SCREEN_PERIOD) {
        t_screen = now;
        drawScreen();
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
    mqtt.loop();   // keep-alive (C subscribes to nothing)
}

bool connectMqtt()
{
    String client_id = "seng3030-C-" + WiFi.macAddress();

    bool ok = mqtt.connect(client_id.c_str(), USER_CTRL.c_str(), MQTT_PASS_CTRL,
                           T_CTRL_STATUS.c_str(), 1, true, "offline");
    if (!ok) {
        Serial.printf("MQTT connect failed, rc=%d\n", mqtt.state());
        return false;
    }

    mqtt.publish(T_CTRL_STATUS.c_str(), "online", true);
    // Make the retained setting match what this screen shows.
    mqtt.publish(T_CTRL_DATA.c_str(), data_on ? "on" : "off", true);
    Serial.println("MQTT connected");
    return true;
}


// ================= buttons =================

void handleButtons()
{
    // wasClicked() fires on release of a short press; wasHold() fires once
    // when a press passes HOLD_MS. A hold never also counts as a click.
    if (M5.BtnA.wasClicked()) {
        sendCommand(T_CTRL_DISPLAY, "next", false, "B: next page");
    }
    if (M5.BtnB.wasClicked()) {
        sendCommand(T_CTRL_DISPLAY, "prev", false, "B: prev page");
    }
    if (M5.BtnA.wasHold()) {
        bool want = !data_on;
        if (sendCommand(T_CTRL_DATA, want ? "on" : "off", true, want ? "A: data ON" : "A: data OFF")) {
            data_on = want;   // only change the screen if the command left this device
        }
    }
    if (M5.BtnB.wasHold()) {
        sendCommand(T_CTRL_AUDIO, "send", false, "A: send audio");
    }
}

bool sendCommand(const std::string& topic, const char* payload, bool retained, const char* label)
{
    bool ok = mqtt.connected() && mqtt.publish(topic.c_str(), payload, retained);
    last_cmd = label;
    last_ok  = ok;
    beep(ok);
    Serial.printf("%s %s = %s\n", ok ? "TX" : "NOT SENT", topic.c_str(), payload);
    return ok;
}

void beep(bool ok)
{
    // tone() is non-blocking: the speaker plays in the background.
    if (ok) M5.Speaker.tone(3000, 40);
    else    M5.Speaker.tone(800, 200);
}


// ================= screen =================

void drawScreen()
{
    auto& d = M5.Display;

    // header
    d.setTextSize(1.5f);
    d.setCursor(0, 0);
    d.setTextColor(CYAN, BLACK);
    d.print("C: CONTROL     ");
    d.setCursor(150, 0);
    if (WiFi.status() != WL_CONNECTED) { d.setTextColor(ORANGE, BLACK); d.print("WiFi ... "); }
    else if (!mqtt.connected())        { d.setTextColor(ORANGE, BLACK); d.print("MQTT ... "); }
    else                               { d.setTextColor(GREEN, BLACK);  d.print("online   "); }
    d.drawFastHLine(0, 16, 240, DARKGREY);

    // A's data switch
    d.setTextSize(2.5f);
    d.setCursor(4, 24);
    d.setTextColor(WHITE, BLACK);
    d.print("A data ");
    d.setTextColor(data_on ? GREEN : RED, BLACK);
    d.print(data_on ? "ON " : "OFF");

    // last command
    d.setTextSize(1.5f);
    d.setCursor(4, 54);
    d.setTextColor(LIGHTGREY, BLACK);
    d.print("Last: ");
    d.setTextColor(last_ok ? WHITE : RED, BLACK);
    d.printf("%-14s", last_ok ? last_cmd.c_str() : (last_cmd + " FAIL").c_str());

    // button help
    d.drawFastHLine(0, 72, 240, DARKGREY);
    d.setTextColor(DARKGREY, BLACK);
    d.setCursor(4, 80);  d.print("A click : B next page");
    d.setCursor(4, 94);  d.print("B click : B prev page");
    d.setCursor(4, 108); d.print("A hold  : A data on/off");
    d.setCursor(4, 122); d.print("B hold  : A send audio");
}
