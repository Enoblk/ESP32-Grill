// RelayControl.cpp - Simplified version with auger debugging removed
#include "RelayControl.h"
#include "Globals.h"

// Simple state tracking
static bool igniterState = false;
static bool augerState = false;
static bool hopperState = false;
static bool blowerState = false;
static bool manualOverrideActive = false;
static unsigned long manualOverrideTimeout = 0;
static const unsigned long MANUAL_OVERRIDE_DURATION = 300000; // 5 minutes timeout

void relay_init() {
  Serial.println("Initializing relay control...");
  
  // Initialize all relay pins as outputs
  pinMode(RELAY_IGNITER_PIN, OUTPUT);
  pinMode(RELAY_AUGER_PIN, OUTPUT);
  pinMode(RELAY_HOPPER_FAN_PIN, OUTPUT);
  pinMode(RELAY_BLOWER_FAN_PIN, OUTPUT);
  
  // Start with all relays off
  digitalWrite(RELAY_IGNITER_PIN, LOW);
  digitalWrite(RELAY_AUGER_PIN, LOW);
  digitalWrite(RELAY_HOPPER_FAN_PIN, LOW);
  digitalWrite(RELAY_BLOWER_FAN_PIN, LOW);
  
  igniterState = false;
  augerState = false;
  hopperState = false;
  blowerState = false;
  manualOverrideActive = false;
  manualOverrideTimeout = 0;
  
  Serial.printf("✅ Relay pins initialized: IGN=%d, AUG=%d, HOP=%d, BLO=%d\n", 
                RELAY_IGNITER_PIN, RELAY_AUGER_PIN, RELAY_HOPPER_FAN_PIN, RELAY_BLOWER_FAN_PIN);
}

void relay_update() {
  // Check for manual override timeout
  if (manualOverrideActive && millis() > manualOverrideTimeout) {
    Serial.println("Manual override timeout - returning to auto control");
    manualOverrideActive = false;
    manualOverrideTimeout = 0;
  }
}

void relay_commit() {
  // Nothing needed in simple version
}

void relay_request_auto(RelayRequest* request) {
  if (manualOverrideActive) {
    Serial.println("Auto request ignored - manual override active");
    return;
  }
  
  // Apply the auto request directly
  if (request->igniter != RELAY_NOCHANGE) {
    bool newState = (request->igniter == RELAY_ON);
    if (newState != igniterState) {
      digitalWrite(RELAY_IGNITER_PIN, newState ? HIGH : LOW);
      igniterState = newState;
    }
  }
  
  if (request->auger != RELAY_NOCHANGE) {
    bool newState = (request->auger == RELAY_ON);
    if (newState != augerState) {
      digitalWrite(RELAY_AUGER_PIN, newState ? HIGH : LOW);
      augerState = newState;
    }
  }
  
  if (request->hopperFan != RELAY_NOCHANGE) {
    bool newState = (request->hopperFan == RELAY_ON);
    if (newState != hopperState) {
      digitalWrite(RELAY_HOPPER_FAN_PIN, newState ? HIGH : LOW);
      hopperState = newState;
    }
  }
  
  if (request->blowerFan != RELAY_NOCHANGE) {
    bool newState = (request->blowerFan == RELAY_ON);
    if (newState != blowerState) {
      digitalWrite(RELAY_BLOWER_FAN_PIN, newState ? HIGH : LOW);
      blowerState = newState;
    }
  }
}

void relay_request_manual(RelayRequest* request) {
  manualOverrideActive = true;
  manualOverrideTimeout = millis() + MANUAL_OVERRIDE_DURATION;
  Serial.println("Manual control activated");
  
  // Apply manual request immediately
  if (request->igniter != RELAY_NOCHANGE) {
    bool newState = (request->igniter == RELAY_ON);
    digitalWrite(RELAY_IGNITER_PIN, newState ? HIGH : LOW);
    igniterState = newState;
  }
  
  if (request->auger != RELAY_NOCHANGE) {
    bool newState = (request->auger == RELAY_ON);
    digitalWrite(RELAY_AUGER_PIN, newState ? HIGH : LOW);
    augerState = newState;
  }
  
  if (request->hopperFan != RELAY_NOCHANGE) {
    bool newState = (request->hopperFan == RELAY_ON);
    digitalWrite(RELAY_HOPPER_FAN_PIN, newState ? HIGH : LOW);
    hopperState = newState;
  }
  
  if (request->blowerFan != RELAY_NOCHANGE) {
    bool newState = (request->blowerFan == RELAY_ON);
    digitalWrite(RELAY_BLOWER_FAN_PIN, newState ? HIGH : LOW);
    blowerState = newState;
  }
}

void relay_clear_manual() {
  manualOverrideActive = false;
  manualOverrideTimeout = 0;
  Serial.println("Manual override cleared");
}

void relay_emergency_stop() {
  Serial.println("EMERGENCY STOP - All relays OFF");
  
  // Turn off all relays immediately
  digitalWrite(RELAY_IGNITER_PIN, LOW);
  digitalWrite(RELAY_AUGER_PIN, LOW);
  digitalWrite(RELAY_HOPPER_FAN_PIN, LOW);
  digitalWrite(RELAY_BLOWER_FAN_PIN, LOW);
  
  // Update state tracking
  igniterState = false;
  augerState = false;
  hopperState = false;
  blowerState = false;
  
  // Clear manual override in emergency
  manualOverrideActive = false;
  manualOverrideTimeout = 0;
}

void relay_clear_emergency() {
  Serial.println("Emergency stop cleared");
}

bool relay_is_safe_state() {
  return true; // Simple version always returns true
}

void relay_print_status() {
  Serial.println("\n=== RELAY STATUS ===");
  Serial.printf("Igniter: %s\n", igniterState ? "ON" : "OFF");
  Serial.printf("Auger: %s\n", augerState ? "ON" : "OFF");
  Serial.printf("Hopper Fan: %s\n", hopperState ? "ON" : "OFF");
  Serial.printf("Blower Fan: %s\n", blowerState ? "ON" : "OFF");
  Serial.printf("Manual Override: %s\n", manualOverrideActive ? "ACTIVE" : "INACTIVE");
  Serial.println("===================\n");
}

void relay_apply_state(RelayRequest* request) {
  if (manualOverrideActive) {
    relay_request_manual(request);
  } else {
    relay_request_auto(request);
  }
}

// Individual relay controls
void relay_set_igniter(bool state) {
  RelayRequest req = {RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE};
  req.igniter = state ? RELAY_ON : RELAY_OFF;
  relay_request_manual(&req);
}

void relay_set_auger(bool state) {
  RelayRequest req = {RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE};
  req.auger = state ? RELAY_ON : RELAY_OFF;
  relay_request_manual(&req);
}

void relay_set_hopper_fan(bool state) {
  RelayRequest req = {RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE};
  req.hopperFan = state ? RELAY_ON : RELAY_OFF;
  relay_request_manual(&req);
}

void relay_set_blower_fan(bool state) {
  RelayRequest req = {RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE};
  req.blowerFan = state ? RELAY_ON : RELAY_OFF;
  relay_request_manual(&req);
}

// Status functions
bool relay_get_manual_override_status() {
  return manualOverrideActive;
}

unsigned long relay_get_manual_override_remaining() {
  if (!manualOverrideActive) return 0;
  return (manualOverrideTimeout > millis()) ? (manualOverrideTimeout - millis()) / 1000 : 0;
}

void relay_force_clear_manual() {
  manualOverrideActive = false;
  manualOverrideTimeout = 0;
  Serial.println("Manual override forcibly cleared");
}