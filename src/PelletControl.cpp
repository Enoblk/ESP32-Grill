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

// PelletControl.cpp - Implementation
#include "PelletControl.h"
#include "Globals.h"
#include "Utility.h"
#include "RelayControl.h"

// PID controller instance
static PIDController pid;
static double targetTemp = 225.0;

// Feed control parameters
static unsigned long lastFeedTime = 0;
static unsigned long feedDuration = 0;
static unsigned long feedInterval = 60000; // Base feed interval (1 minute)
static bool feedCycleActive = false;
static unsigned long feedCycleStartTime = 0;

// Feed timing parameters (optimized for Daniel Boone)
#define MIN_FEED_TIME 1000      // Minimum auger on time (1 second)
#define MAX_FEED_TIME 15000     // Maximum auger on time (15 seconds)
#define MIN_FEED_INTERVAL 30000 // Minimum time between feeds (30 seconds)
#define MAX_FEED_INTERVAL 300000 // Maximum time between feeds (5 minutes)

// Temperature-based feed curves
struct FeedCurve {
  double tempError;  // Temperature error (setpoint - actual)
  unsigned long feedTime;  // Auger on time in milliseconds
  unsigned long interval;  // Time between feeds
};

// Feed curve optimized for Daniel Boone pellet consumption
static const FeedCurve feedCurves[] = {
  {-50.0, 0,     180000},    // Too hot - no feed, wait 3 minutes
  {-25.0, 0,     120000},    // Hot - no feed, wait 2 minutes  
  {-10.0, 1000,  90000},     // Slightly hot - minimal feed
  {-5.0,  2000,  75000},     // Near target (hot side)
  {0.0,   3000,  60000},     // At target - maintenance feed
  {5.0,   4000,  45000},     // Slightly cool
  {10.0,  6000,  35000},     // Cool - more pellets
  {25.0,  10000, 25000},     // Cold - aggressive feeding
  {50.0,  15000, 20000},     // Very cold - maximum feed
  {100.0, 15000, 15000}      // Extremely cold - rapid feed
};

#define FEED_CURVE_SIZE (sizeof(feedCurves) / sizeof(feedCurves[0]))

void pellet_init() {
  Serial.println("Initializing pellet control system...");
  
  // Initialize PID controller with Daniel Boone optimized parameters
  pid.kp = 1.5;      // Proportional gain
  pid.ki = 0.01;     // Integral gain  
  pid.kd = 0.5;      // Derivative gain
  pid.output_min = 0.0;
  pid.output_max = 100.0;
  resetPID(&pid);
  
  // Initialize feed control
  targetTemp = setpoint;
  lastFeedTime = millis();
  feedCycleActive = false;
  
  Serial.printf("PID initialized: Kp=%.2f, Ki=%.3f, Kd=%.2f\n", pid.kp, pid.ki, pid.kd);
  Serial.println("Pellet control system ready");
}

void pellet_feed_loop() {
  if (!grillRunning) return;
  
  unsigned long now = millis();
  double currentTemp = readTemperature();
  
  // Skip if temperature reading is invalid
  if (currentTemp < 32.0 || currentTemp > 700.0) {
    return;
  }
  
  // Update target temperature
  targetTemp = setpoint;
  
  // Calculate temperature error
  double tempError = targetTemp - currentTemp;
  
  // Calculate PID output
  float pidOutput = calculatePID(&pid, targetTemp, currentTemp);
  
  // Handle active feed cycle
  if (feedCycleActive) {
    if (now - feedCycleStartTime >= feedDuration) {
      // End feed cycle
      RelayRequest stopFeed = {RELAY_NOCHANGE, RELAY_OFF, RELAY_NOCHANGE, RELAY_NOCHANGE};
      relay_request_auto(&stopFeed);
      feedCycleActive = false;
      lastFeedTime = now;
      
      Serial.printf("Feed cycle complete: %lu ms\n", feedDuration);
    }
    return; // Wait for current cycle to complete
  }
  
  // Check if it's time for next feed cycle
  if (now - lastFeedTime < MIN_FEED_INTERVAL) {
    return; // Too soon for next feed
  }
  
  // Calculate feed parameters based on temperature error
  pellet_calculate_feed_time(tempError);
  
  // Determine if we should feed based on calculated interval
  if (now - lastFeedTime >= feedInterval) {
    pellet_execute_feed_cycle();
  }
  
  // Debug output every 30 seconds
  static unsigned long lastDebug = 0;
  if (now - lastDebug >= 30000) {
    Serial.printf("Pellet Control: Temp=%.1f°F, Target=%.1f°F, Error=%.1f°F, PID=%.1f, Next feed in %lu sec\n",
                  currentTemp, targetTemp, tempError, pidOutput,
                  (feedInterval - (now - lastFeedTime)) / 1000);
    lastDebug = now;
  }
}

void pellet_calculate_feed_time(double tempError) {
  // Find appropriate feed curve
  feedDuration = MIN_FEED_TIME;
  feedInterval = MAX_FEED_INTERVAL;
  
  for (int i = 0; i < FEED_CURVE_SIZE - 1; i++) {
    if (tempError >= feedCurves[i].tempError && tempError < feedCurves[i + 1].tempError) {
      // Interpolate between curve points
      double ratio = (tempError - feedCurves[i].tempError) / 
                     (feedCurves[i + 1].tempError - feedCurves[i].tempError);
      
      feedDuration = feedCurves[i].feedTime + 
                     ratio * (feedCurves[i + 1].feedTime - feedCurves[i].feedTime);
      
      feedInterval = feedCurves[i].interval + 
                     ratio * (feedCurves[i + 1].interval - feedCurves[i].interval);
      break;
    }
  }
  
  // Apply limits
  feedDuration = constrain(feedDuration, MIN_FEED_TIME, MAX_FEED_TIME);
  feedInterval = constrain(feedInterval, MIN_FEED_INTERVAL, MAX_FEED_INTERVAL);
}

void pellet_execute_feed_cycle() {
  if (feedDuration > 0) {
    // Start auger and ensure hopper fan is on
    RelayRequest startFeed = {RELAY_NOCHANGE, RELAY_ON, RELAY_ON, RELAY_NOCHANGE};
    relay_request_auto(&startFeed);
    
    feedCycleActive = true;
    feedCycleStartTime = millis();
    
    Serial.printf("Starting feed cycle: %lu ms duration\n", feedDuration);
  } else {
    // No feed needed, just update timing
    lastFeedTime = millis();
  }
}

float calculatePID(PIDController* pid, float setpoint, float measurement) {
  unsigned long now = millis();
  
  // Calculate time delta
  float dt = (now - pid->last_time) / 1000.0; // Convert to seconds
  if (dt <= 0.0) dt = 0.1; // Prevent division by zero
  
  // Calculate error
  float error = setpoint - measurement;
  
  // Proportional term
  float proportional = pid->kp * error;
  
  // Integral term with windup protection
  pid->integral += error * dt;
  
  // Clamp integral to prevent windup
  float integral_max = pid->output_max / pid->ki;
  float integral_min = pid->output_min / pid->ki;
  pid->integral = constrain(pid->integral, integral_min, integral_max);
  
  float integral = pid->ki * pid->integral;
  
  // Derivative term
  float derivative = pid->kd * (error - pid->previous_error) / dt;
  
  // Calculate total output
  pid->output = proportional + integral + derivative;
  
  // Clamp output
  pid->output = constrain(pid->output, pid->output_min, pid->output_max);
  
  // Save for next iteration
  pid->previous_error = error;
  pid->last_time = now;
  
  return pid->output;
}

void resetPID(PIDController* pid) {
  pid->integral = 0.0;
  pid->previous_error = 0.0;
  pid->output = 0.0;
  pid->last_time = millis();
}

void setPIDParameters(float kp, float ki, float kd) {
  pid.kp = kp;
  pid.ki = ki;
  pid.kd = kd;
  
  // Reset integral when parameters change
  pid.integral = 0.0;
  
  Serial.printf("PID parameters updated: Kp=%.2f, Ki=%.3f, Kd=%.2f\n", kp, ki, kd);
}

void getPIDParameters(float* kp, float* ki, float* kd) {
  *kp = pid.kp;
  *ki = pid.ki;
  *kd = pid.kd;
}

void pellet_set_target(double target) {
  targetTemp = target;
  setpoint = target; // Update global setpoint too
}

double pellet_get_target() {
  return targetTemp;
}

String pellet_get_status() {
  if (!grillRunning) return "IDLE";
  
  if (feedCycleActive) {
    unsigned long remaining = feedDuration - (millis() - feedCycleStartTime);
    return "FEEDING (" + String(remaining / 1000) + "s)";
  }
  
  unsigned long nextFeed = feedInterval - (millis() - lastFeedTime);
  if (nextFeed > 60000) {
    return "WAITING (" + String(nextFeed / 60000) + "min)";
  } else {
    return "WAITING (" + String(nextFeed / 1000) + "s)";
  }
}

void pellet_print_diagnostics() {
  Serial.println("\n=== PELLET CONTROL DIAGNOSTICS ===");
  Serial.printf("Target Temperature: %.1f°F\n", targetTemp);
  Serial.printf("Current Temperature: %.1f°F\n", readTemperature());
  Serial.printf("Temperature Error: %.1f°F\n", targetTemp - readTemperature());
  Serial.printf("PID Output: %.1f\n", pid.output);
  Serial.printf("PID Parameters: Kp=%.2f, Ki=%.3f, Kd=%.2f\n", pid.kp, pid.ki, pid.kd);
  Serial.printf("Feed Duration: %lu ms\n", feedDuration);
  Serial.printf("Feed Interval: %lu ms\n", feedInterval);
  Serial.printf("Feed Cycle Active: %s\n", feedCycleActive ? "YES" : "NO");
  Serial.printf("Time Since Last Feed: %lu sec\n", (millis() - lastFeedTime) / 1000);
  Serial.printf("Status: %s\n", pellet_get_status().c_str());
  Serial.println("=====================================\n");
}