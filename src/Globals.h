// =========================
// Globals.h
// Shared variables for ESP32 Grill Controller
// =========================
#ifndef GLOBALS_H
#define GLOBALS_H

#include <Preferences.h>
#include <Adafruit_SSD1306.h>
#include <ESPAsyncWebServer.h>

extern Adafruit_SSD1306 display;
extern AsyncWebServer server;
extern Preferences prefs;

extern double setpoint;
extern bool grillRunning;
extern bool igniting;
extern unsigned long igniteStartTime;
extern double igniteStartTemp;

// Pin numbers
extern int RELAY_HOPPER_FAN_PIN;
extern int RELAY_AUGER_PIN;
extern int RELAY_IGNITER_PIN;
extern int RELAY_BLOWER_FAN_PIN;

void init_relay_pins();
void save_setpoint();
void restore_setpoint();

#endif // GLOBALS_H
