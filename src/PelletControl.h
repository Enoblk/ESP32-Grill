// PelletControl.h - Complete header with adjustable pellet feed parameters
#ifndef PELLETCONTROL_H
#define PELLETCONTROL_H

#include <Arduino.h>
#include <Preferences.h>
#include "Ignition.h"  // Include Ignition.h for IgnitionState

// PID Controller structure
struct PIDController {
  float kp, ki, kd;           // PID coefficients
  float integral;             // Integral accumulator
  float previous_error;       // Previous error for derivative
  float output;               // Current PID output
  unsigned long last_time;    // Last calculation time
  float output_min, output_max; // Output limits
};

// Core pellet feed control functions
void pellet_init();
void pellet_feed_loop();
void pellet_set_target(double target);
double pellet_get_target();

// PID functions
void setPIDParameters(float kp, float ki, float kd);
void getPIDParameters(float* kp, float* ki, float* kd);
float calculatePID(PIDController* pid, float setpoint, float measurement);
void resetPID(PIDController* pid);

// Feed timing and control
void pellet_calculate_feed_time(double temperature_error);
void pellet_execute_feed_cycle();
void pellet_handle_ignition_feeding(unsigned long now);

// Status and diagnostics
String pellet_get_status();
void pellet_print_diagnostics();

// Adjustable pellet feed parameters - NEW FUNCTIONS
unsigned long pellet_get_initial_feed_duration();
unsigned long pellet_get_lighting_feed_duration();
unsigned long pellet_get_normal_feed_duration();
unsigned long pellet_get_lighting_feed_interval();

void pellet_set_initial_feed_duration(unsigned long duration);
void pellet_set_lighting_feed_duration(unsigned long duration);
void pellet_set_normal_feed_duration(unsigned long duration);
void pellet_set_lighting_feed_interval(unsigned long interval);

// Parameter persistence
void savePelletParameters();
void loadPelletParameters();

#endif // PELLETCONTROL_H