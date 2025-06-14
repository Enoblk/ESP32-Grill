// RelayControl.h - Advanced Relay Management System
#ifndef RELAYCONTROL_H
#define RELAYCONTROL_H

#include <Arduino.h>

// Relay states
enum RelayState {
  RELAY_OFF = 0,
  RELAY_ON = 1,
  RELAY_NOCHANGE = 2  // Don't change current state
};

// Relay request structure for coordinated control
struct RelayRequest {
  RelayState igniter;
  RelayState auger;
  RelayState hopperFan;
  RelayState blowerFan;
};

// Safety interlocks and timing
struct RelayTiming {
  unsigned long igniterOnTime;
  unsigned long augerOnTime;
  unsigned long lastAugerCycle;
  unsigned long hopperFanOnTime;
  unsigned long blowerFanOnTime;
};

// Relay control functions
void relay_init();
void relay_update();
void relay_commit();
void relay_clear_manual();

// Request relay changes
void relay_request_auto(RelayRequest* request);
void relay_request_manual(RelayRequest* request);

// Safety and status
bool relay_is_safe_state();
void relay_emergency_stop();
void relay_print_status();

// Internal functions
void relay_apply_state(RelayRequest* request);
void relay_clear_emergency();

// Individual relay controls for manual override
void relay_set_igniter(bool state);
void relay_set_auger(bool state);
void relay_set_hopper_fan(bool state);
void relay_set_blower_fan(bool state);

#endif