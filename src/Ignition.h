// Ignition.h - Simplified ignition system with safety features
#ifndef IGNITION_H
#define IGNITION_H

#include <Arduino.h>

// Ignition states
enum IgnitionState {
  IGNITION_OFF,
  IGNITION_PREHEAT,    // Fans only, no igniter
  IGNITION_LIGHTING,   // Igniter on, watching for temp rise
  IGNITION_STABILIZE,  // Igniter off, stabilizing flame
  IGNITION_COMPLETE,   // Success
  IGNITION_FAILED      // Failed
};

// Core ignition functions
void ignition_init();
void ignition_start(double currentTemp);
void ignition_stop();
void ignition_loop();

// Status functions
IgnitionState ignition_get_state();
String ignition_get_status_string();
bool ignition_is_active();
bool ignition_is_complete();
bool ignition_has_failed();
unsigned long ignition_get_runtime();

// Configuration
void ignition_set_target_temp(double temp);
double ignition_get_target_temp();

// Internal functions
void ignition_complete();
void ignition_fail();
void ignition_emergency_stop();

#endif // IGNITION_H