// =========================
// Utility.h
// Helper and cross-module functions for ESP32 Grill Controller
// =========================
#ifndef UTILITY_H
#define UTILITY_H

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

double readTemperature();
String getStatus(double temp);
String relayColor(bool on);

#ifdef __cplusplus
}
#endif

#endif // UTILITY_H
