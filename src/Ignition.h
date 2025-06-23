// Ignition.h - Complete file with PiFire-style auger control declarations
#ifndef IGNITION_H
#define IGNITION_H

#include <Arduino.h>

// Ignition states
enum IgnitionState {
  IGNITION_OFF,
  IGNITION_PREHEAT,      // Preheating fans
  IGNITION_INITIAL_FEED, // Initial pellet feed (prime)
  IGNITION_LIGHTING,     // Lighting pellets
  IGNITION_FLAME_DETECT, // Detecting flame/temperature rise
  IGNITION_STABILIZE,    // Stabilizing flame
  IGNITION_COMPLETE,     // Ignition successful
  IGNITION_FAILED        // Ignition failed
};

// Forward declaration
void ignition_fail();

// Ignition control functions
void ignition_init();
void ignition_start(double currentTemp);
void ignition_stop();
void ignition_loop();

// Status functions
IgnitionState ignition_get_state();
String ignition_get_status_string();
bool ignition_is_complete();
bool ignition_has_failed();

// Configuration
void ignition_set_target_temp(double temp);
double ignition_get_target_temp();

// PiFire-style auger control functions
void pifire_auger_cycle();
void pifire_manual_auger_prime();
void pifire_temperature_control();  // Main function that replaces all pellet control
String pifire_get_status();         // Get PiFire auger status

#endif