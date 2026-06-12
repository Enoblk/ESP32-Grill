#ifndef IGNITION_H
#define IGNITION_H

#include <Arduino.h>

enum IgnitionState {
  IGNITION_OFF,
  IGNITION_PREHEAT,
  IGNITION_LIGHTING,
  IGNITION_STABILIZE,
  IGNITION_COMPLETE,
  IGNITION_FAILED
};

void ignition_init();
void ignition_start(double currentTemp);
void ignition_stop();
void ignition_loop();
void ignition_complete();
void ignition_fail();
void ignition_emergency_stop();

IgnitionState ignition_get_state();
String ignition_get_status_string();
bool ignition_is_active();
bool ignition_is_complete();
bool ignition_has_failed();
unsigned long ignition_get_runtime();

void ignition_set_target_temp(double temp);
double ignition_get_target_temp();

bool auger_prime_start(unsigned long durationMs = 30000);
bool auger_prime_active();
unsigned long auger_prime_remaining_ms();

#endif
