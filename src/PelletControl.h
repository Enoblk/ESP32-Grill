// =========================
// PelletControl.h
// Pellet/Auger PID control & relay logic for ESP32 Grill Controller
// =========================
#ifndef PELLET_CONTROL_H
#define PELLET_CONTROL_H

#include <Arduino.h>

extern bool grillRunning;
extern bool igniting;
extern unsigned long igniteStartTime;
extern double igniteStartTemp;
extern double setpoint;

// NEW: For modular relay control, no digitalWrite here!
void pellet_feed_loop();
void save_setpoint();      // Store setpoint in Preferences
void restore_setpoint();   // Restore setpoint from Preferences

// PID settings access (for Web UI tuning, etc)
extern float Kp, Ki, Kd;
extern unsigned long AUGER_INTERVAL, AUGER_ON_MIN, AUGER_ON_MAX;

#endif // PELLET_CONTROL_H
