// PelletControl.h - Advanced PID-based Pellet Feed Control
#ifndef PELLETCONTROL_H
#define PELLETCONTROL_H

#include <Arduino.h>

// PID Controller structure
struct PIDController {
  float kp, ki, kd;           // PID coefficients
  float integral;             // Integral accumulator
  float previous_error;       // Previous error for derivative
  float output;               // Current PID output
  unsigned long last_time;    // Last calculation time
  float output_min, output_max; // Output limits
};

// Pellet feed control functions
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

// Status and diagnostics
String pellet_get_status();
void pellet_print_diagnostics();

#endif
