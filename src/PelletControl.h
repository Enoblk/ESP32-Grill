// PelletControl.h - Simplified pellet control with safety features
#ifndef PELLETCONTROL_H
#define PELLETCONTROL_H

#include <Arduino.h>
#include <Preferences.h>

// Timing constants (in milliseconds) - moved to header for global access
#define DEFAULT_ON_TIME    15000    // 15 seconds ON
#define DEFAULT_OFF_TIME   60000    // 60 seconds OFF
#define MIN_ON_TIME        5000     // 5 seconds minimum
#define MAX_ON_TIME        30000    // 30 seconds maximum
#define MIN_OFF_TIME       30000    // 30 seconds minimum
#define MAX_OFF_TIME       180000   // 3 minutes maximum

// Special modes
#define STARTUP_ON_TIME    20000    // 20 seconds during startup
#define STARTUP_OFF_TIME   45000    // 45 seconds during startup
#define PRIME_TIME         30000    // 30 seconds for manual prime

// Auger state structure
struct AugerState {
  bool isRunning;               // Currently feeding pellets
  unsigned long cycleStartTime; // When current cycle started
  unsigned long lastCycleEndTime; // When last cycle ended
  unsigned long currentOnTime;  // Current feed duration
  unsigned long currentOffTime; // Current wait duration
  unsigned long cycleCount;     // Total cycles since start
};

// Core pellet functions
void pellet_init();
void pellet_feed_loop();

// Auger control
void pellet_start_auger();
void pellet_stop_auger();
void pellet_manual_prime(unsigned long primeTime = PRIME_TIME);

// Mode control
void pellet_set_startup_mode();
void pellet_set_cooking_mode();

// Status and diagnostics
String pellet_get_status();
void pellet_print_status();
void pellet_print_diagnostics();
AugerState pellet_get_auger_state();

// Safety functions
bool pellet_safety_checks();
void pellet_emergency_stop();

// Internal functions
void pellet_adjust_timing();
void resetPelletTimers();
void savePelletSettings();
void loadPelletSettings();

// Compatibility functions (for existing code)
void setPIDParameters(float kp, float ki, float kd);
void getPIDParameters(float* kp, float* ki, float* kd);

// Functions for web interface compatibility
unsigned long pellet_get_initial_feed_duration();
unsigned long pellet_get_lighting_feed_duration();
unsigned long pellet_get_normal_feed_duration();
unsigned long pellet_get_lighting_feed_interval();

void pellet_set_initial_feed_duration(unsigned long duration);
void pellet_set_lighting_feed_duration(unsigned long duration);
void pellet_set_normal_feed_duration(unsigned long duration);
void pellet_set_lighting_feed_interval(unsigned long interval);

#endif // PELLETCONTROL_H