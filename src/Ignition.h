// =========================
// Ignition.h
// Handles the ignition phase for ESP32 Grill Controller
// =========================
#ifndef IGNITION_H
#define IGNITION_H

#include <Arduino.h>

void ignition_start(double temp_start);      // Initiate ignition phase
void ignition_loop();                        // Call from loop; manages relays and exit
bool ignition_active();                      // True if ignition is in progress

#endif // IGNITION_H
