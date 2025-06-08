// =========================
// Globals.cpp
// Defines all global variables for ESP32 Grill Controller
// =========================
#include "Globals.h"
#include <Wire.h>

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define SDA_PIN        21
#define SCL_PIN        22

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
AsyncWebServer server(80);
Preferences prefs;

void init_relay_pins() {
    pinMode(RELAY_HOPPER_FAN_PIN, OUTPUT);
    pinMode(RELAY_AUGER_PIN, OUTPUT);
    pinMode(RELAY_IGNITER_PIN, OUTPUT);
    pinMode(RELAY_BLOWER_FAN_PIN, OUTPUT);
    digitalWrite(RELAY_HOPPER_FAN_PIN, LOW);
    digitalWrite(RELAY_AUGER_PIN, LOW);
    digitalWrite(RELAY_IGNITER_PIN, LOW);
    digitalWrite(RELAY_BLOWER_FAN_PIN, LOW);
}

void save_setpoint() {
    prefs.begin("grill", false);
    prefs.putFloat("setpt", setpoint);
    prefs.end();
}

void restore_setpoint() {
    prefs.begin("grill", true);
    setpoint = prefs.getFloat("setpt", setpoint);
    prefs.end();
}

double setpoint = 250.0;
bool grillRunning = false;
bool igniting = false;
unsigned long igniteStartTime = 0;
double igniteStartTemp = 0;

int RELAY_HOPPER_FAN_PIN    = 25;
int RELAY_AUGER_PIN         = 26;
int RELAY_IGNITER_PIN       = 27;
int RELAY_BLOWER_FAN_PIN    = 14;

char ssid[32] = "";
char password[64] = "";
bool useDHCP = true;

