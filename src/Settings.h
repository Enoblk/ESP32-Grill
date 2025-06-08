// =========================
// Settings.h
// Wi-Fi/network setup and persistent preferences for ESP32 Grill Controller
// =========================
#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

extern char ssid[32];
extern char password[64];
extern bool useDHCP;

void load_settings();
void setup_wifi_and_mdns();

#endif // SETTINGS_H
