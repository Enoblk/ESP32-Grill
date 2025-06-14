// RelayControl.cpp - Simple version that actually works
#include "RelayControl.h"
#include "Globals.h"

// Simple state tracking
static bool igniterState = false;
static bool augerState = false;
static bool hopperState = false;
static bool blowerState = false;
static bool manualOverrideActive = false;

void relay_init() {
  Serial.println("Initializing simple relay control...");
  
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
  
  Serial.printf("Relay pins initialized: IGN=%d, AUG=%d, HOP=%d, BLO=%d\n", 
                RELAY_IGNITER_PIN, RELAY_AUGER_PIN, RELAY_HOPPER_FAN_PIN, RELAY_BLOWER_FAN_PIN);
}

void relay_update() {
  // Simple update - just maintain current states
  // No complex logic for now
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
      Serial.printf("AUTO: Igniter %s\n", newState ? "ON" : "OFF");
    }
  }
  
  if (request->auger != RELAY_NOCHANGE) {
    bool newState = (request->auger == RELAY_ON);
    if (newState != augerState) {
      digitalWrite(RELAY_AUGER_PIN, newState ? HIGH : LOW);
      augerState = newState;
      Serial.printf("AUTO: Auger %s\n", newState ? "ON" : "OFF");
    }
  }
  
  if (request->hopperFan != RELAY_NOCHANGE) {
    bool newState = (request->hopperFan == RELAY_ON);
    if (newState != hopperState) {
      digitalWrite(RELAY_HOPPER_FAN_PIN, newState ? HIGH : LOW);
      hopperState = newState;
      Serial.printf("AUTO: Hopper Fan %s\n", newState ? "ON" : "OFF");
    }
  }
  
  if (request->blowerFan != RELAY_NOCHANGE) {
    bool newState = (request->blowerFan == RELAY_ON);
    if (newState != blowerState) {
      digitalWrite(RELAY_BLOWER_FAN_PIN, newState ? HIGH : LOW);
      blowerState = newState;
      Serial.printf("AUTO: Blower Fan %s\n", newState ? "ON" : "OFF");
    }
  }
}

void relay_request_manual(RelayRequest* request) {
  manualOverrideActive = true;
  Serial.println("MANUAL CONTROL ACTIVATED");
  
  // Apply manual request immediately
  if (request->igniter != RELAY_NOCHANGE) {
    bool newState = (request->igniter == RELAY_ON);
    digitalWrite(RELAY_IGNITER_PIN, newState ? HIGH : LOW);
    igniterState = newState;
    Serial.printf("MANUAL: Igniter %s (Pin %d = %s)\n", 
                  newState ? "ON" : "OFF", RELAY_IGNITER_PIN, newState ? "HIGH" : "LOW");
  }
  
  if (request->auger != RELAY_NOCHANGE) {
    bool newState = (request->auger == RELAY_ON);
    digitalWrite(RELAY_AUGER_PIN, newState ? HIGH : LOW);
    augerState = newState;
    Serial.printf("MANUAL: Auger %s (Pin %d = %s)\n", 
                  newState ? "ON" : "OFF", RELAY_AUGER_PIN, newState ? "HIGH" : "LOW");
  }
  
  if (request->hopperFan != RELAY_NOCHANGE) {
    bool newState = (request->hopperFan == RELAY_ON);
    digitalWrite(RELAY_HOPPER_FAN_PIN, newState ? HIGH : LOW);
    hopperState = newState;
    Serial.printf("MANUAL: Hopper Fan %s (Pin %d = %s)\n", 
                  newState ? "ON" : "OFF", RELAY_HOPPER_FAN_PIN, newState ? "HIGH" : "LOW");
  }
  
  if (request->blowerFan != RELAY_NOCHANGE) {
    bool newState = (request->blowerFan == RELAY_ON);
    digitalWrite(RELAY_BLOWER_FAN_PIN, newState ? HIGH : LOW);
    blowerState = newState;
    Serial.printf("MANUAL: Blower Fan %s (Pin %d = %s)\n", 
                  newState ? "ON" : "OFF", RELAY_BLOWER_FAN_PIN, newState ? "HIGH" : "LOW");
  }
  
  // Print current pin states for verification
  Serial.printf("Pin readback: IGN=%d, AUG=%d, HOP=%d, BLO=%d\n",
                digitalRead(RELAY_IGNITER_PIN), digitalRead(RELAY_AUGER_PIN),
                digitalRead(RELAY_HOPPER_FAN_PIN), digitalRead(RELAY_BLOWER_FAN_PIN));
}

void relay_clear_manual() {
  manualOverrideActive = false;
  Serial.println("Manual override cleared");
}

void relay_emergency_stop() {
  Serial.println("EMERGENCY STOP - All relays OFF");
  digitalWrite(RELAY_IGNITER_PIN, LOW);
  digitalWrite(RELAY_AUGER_PIN, LOW);
  digitalWrite(RELAY_HOPPER_FAN_PIN, LOW);
  digitalWrite(RELAY_BLOWER_FAN_PIN, LOW);
  
  igniterState = false;
  augerState = false;
  hopperState = false;
  blowerState = false;
}

void relay_clear_emergency() {
  Serial.println("Emergency stop cleared");
}

bool relay_is_safe_state() {
  // Always return true for simple version
  return true;
}

void relay_print_status() {
  Serial.println("=== SIMPLE RELAY STATUS ===");
  Serial.printf("Igniter: %s (Pin %d)\n", igniterState ? "ON" : "OFF", RELAY_IGNITER_PIN);
  Serial.printf("Auger: %s (Pin %d)\n", augerState ? "ON" : "OFF", RELAY_AUGER_PIN);
  Serial.printf("Hopper Fan: %s (Pin %d)\n", hopperState ? "ON" : "OFF", RELAY_HOPPER_FAN_PIN);
  Serial.printf("Blower Fan: %s (Pin %d)\n", blowerState ? "ON" : "OFF", RELAY_BLOWER_FAN_PIN);
  Serial.printf("Manual Override: %s\n", manualOverrideActive ? "YES" : "NO");
  Serial.println("============================");
}

void relay_apply_state(RelayRequest* request) {
  // This function is called by other parts - just forward to manual request
  relay_request_manual(request);
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