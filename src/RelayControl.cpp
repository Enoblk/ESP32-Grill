// RelayControl.cpp - Complete file with enhanced debugging and manual override timeout
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
  Serial.println("Initializing enhanced relay control with debugging...");
  
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
  
  // Verify all pins are actually LOW
  Serial.printf("🔍 Pin verification: IGN=%s, AUG=%s, HOP=%s, BLO=%s\n",
                digitalRead(RELAY_IGNITER_PIN) ? "HIGH" : "LOW",
                digitalRead(RELAY_AUGER_PIN) ? "HIGH" : "LOW",
                digitalRead(RELAY_HOPPER_FAN_PIN) ? "HIGH" : "LOW",
                digitalRead(RELAY_BLOWER_FAN_PIN) ? "HIGH" : "LOW");
}

void relay_update() {
  // Check for manual override timeout
  if (manualOverrideActive && millis() > manualOverrideTimeout) {
    Serial.println("⏰ Manual override timeout - returning to auto control");
    manualOverrideActive = false;
    manualOverrideTimeout = 0;
  }
}

void relay_commit() {
  // Nothing needed in simple version
}

void relay_request_auto(RelayRequest* request) {
  if (manualOverrideActive) {
    Serial.println("🚫 Auto request ignored - manual override active");
    unsigned long remaining = (manualOverrideTimeout > millis()) ? 
                              (manualOverrideTimeout - millis()) / 1000 : 0;
    Serial.printf("   Manual override expires in %lu seconds\n", remaining);
    return;
  }
  
  Serial.println("🤖 Processing AUTO relay request");
  
  // Apply the auto request directly with enhanced debugging
  if (request->igniter != RELAY_NOCHANGE) {
    bool newState = (request->igniter == RELAY_ON);
    if (newState != igniterState) {
      Serial.printf("🔥 AUTO: Changing Igniter %s -> %s\n", 
                    igniterState ? "ON" : "OFF", newState ? "ON" : "OFF");
      digitalWrite(RELAY_IGNITER_PIN, newState ? HIGH : LOW);
      igniterState = newState;
      
      // Verify the change
      delay(5);
      bool actualState = digitalRead(RELAY_IGNITER_PIN);
      if (actualState == (newState ? HIGH : LOW)) {
        Serial.printf("   ✅ Igniter successfully set to %s\n", newState ? "ON" : "OFF");
      } else {
        Serial.printf("   ❌ ERROR: Igniter state mismatch! Expected %s, got %s\n",
                      newState ? "HIGH" : "LOW", actualState ? "HIGH" : "LOW");
      }
    }
  }
  
  if (request->auger != RELAY_NOCHANGE) {
    bool newState = (request->auger == RELAY_ON);
    if (newState != augerState) {
      Serial.printf("🌾 AUTO: Changing Auger %s -> %s (CRITICAL OPERATION)\n", 
                    augerState ? "ON" : "OFF", newState ? "ON" : "OFF");
      
      // Extra debugging for auger operations
      Serial.printf("   📊 Pre-change: Free heap=%d, Stack=%d\n", 
                    ESP.getFreeHeap(), uxTaskGetStackHighWaterMark(NULL));
      
      digitalWrite(RELAY_AUGER_PIN, newState ? HIGH : LOW);
      augerState = newState;
      
      // Add delay to prevent rapid switching and allow verification
      delay(10);
      
      // Verify the state was actually set
      bool actualState = digitalRead(RELAY_AUGER_PIN);
      if (actualState == (newState ? HIGH : LOW)) {
        Serial.printf("   ✅ Auger successfully set to %s\n", newState ? "ON" : "OFF");
      } else {
        Serial.printf("   ❌ CRITICAL ERROR: Auger state mismatch! Expected %s, got %s\n",
                      newState ? "HIGH" : "LOW", actualState ? "HIGH" : "LOW");
      }
      
      Serial.printf("   📊 Post-change: Free heap=%d, Stack=%d\n", 
                    ESP.getFreeHeap(), uxTaskGetStackHighWaterMark(NULL));
    }
  }
  
  if (request->hopperFan != RELAY_NOCHANGE) {
    bool newState = (request->hopperFan == RELAY_ON);
    if (newState != hopperState) {
      Serial.printf("💨 AUTO: Changing Hopper Fan %s -> %s\n", 
                    hopperState ? "ON" : "OFF", newState ? "ON" : "OFF");
      digitalWrite(RELAY_HOPPER_FAN_PIN, newState ? HIGH : LOW);
      hopperState = newState;
      
      // Verify the change
      delay(5);
      bool actualState = digitalRead(RELAY_HOPPER_FAN_PIN);
      if (actualState == (newState ? HIGH : LOW)) {
        Serial.printf("   ✅ Hopper Fan successfully set to %s\n", newState ? "ON" : "OFF");
      } else {
        Serial.printf("   ❌ ERROR: Hopper Fan state mismatch!\n");
      }
    }
  }
  
  if (request->blowerFan != RELAY_NOCHANGE) {
    bool newState = (request->blowerFan == RELAY_ON);
    if (newState != blowerState) {
      Serial.printf("🌪️ AUTO: Changing Blower Fan %s -> %s\n", 
                    blowerState ? "ON" : "OFF", newState ? "ON" : "OFF");
      digitalWrite(RELAY_BLOWER_FAN_PIN, newState ? HIGH : LOW);
      blowerState = newState;
      
      // Verify the change
      delay(5);
      bool actualState = digitalRead(RELAY_BLOWER_FAN_PIN);
      if (actualState == (newState ? HIGH : LOW)) {
        Serial.printf("   ✅ Blower Fan successfully set to %s\n", newState ? "ON" : "OFF");
      } else {
        Serial.printf("   ❌ ERROR: Blower Fan state mismatch!\n");
      }
    }
  }
}

void relay_request_manual(RelayRequest* request) {
  manualOverrideActive = true;
  manualOverrideTimeout = millis() + MANUAL_OVERRIDE_DURATION;
  Serial.printf("🕹️ MANUAL CONTROL ACTIVATED (timeout in %lu minutes)\n", MANUAL_OVERRIDE_DURATION / 60000);
  
  // Apply manual request immediately with full debugging
  if (request->igniter != RELAY_NOCHANGE) {
    bool newState = (request->igniter == RELAY_ON);
    Serial.printf("🔥 MANUAL: Setting Igniter to %s\n", newState ? "ON" : "OFF");
    digitalWrite(RELAY_IGNITER_PIN, newState ? HIGH : LOW);
    igniterState = newState;
    Serial.printf("   Pin %d = %s\n", RELAY_IGNITER_PIN, newState ? "HIGH" : "LOW");
  }
  
  if (request->auger != RELAY_NOCHANGE) {
    bool newState = (request->auger == RELAY_ON);
    Serial.printf("🌾 MANUAL: Setting Auger to %s (CRITICAL)\n", newState ? "ON" : "OFF");
    Serial.printf("   📊 Before: Free heap=%d, Stack=%d\n", 
                  ESP.getFreeHeap(), uxTaskGetStackHighWaterMark(NULL));
    
    digitalWrite(RELAY_AUGER_PIN, newState ? HIGH : LOW);
    augerState = newState;
    
    Serial.printf("   Pin %d = %s\n", RELAY_AUGER_PIN, newState ? "HIGH" : "LOW");
    Serial.printf("   📊 After: Free heap=%d, Stack=%d\n", 
                  ESP.getFreeHeap(), uxTaskGetStackHighWaterMark(NULL));
  }
  
  if (request->hopperFan != RELAY_NOCHANGE) {
    bool newState = (request->hopperFan == RELAY_ON);
    Serial.printf("💨 MANUAL: Setting Hopper Fan to %s\n", newState ? "ON" : "OFF");
    digitalWrite(RELAY_HOPPER_FAN_PIN, newState ? HIGH : LOW);
    hopperState = newState;
    Serial.printf("   Pin %d = %s\n", RELAY_HOPPER_FAN_PIN, newState ? "HIGH" : "LOW");
  }
  
  if (request->blowerFan != RELAY_NOCHANGE) {
    bool newState = (request->blowerFan == RELAY_ON);
    Serial.printf("🌪️ MANUAL: Setting Blower Fan to %s\n", newState ? "ON" : "OFF");
    digitalWrite(RELAY_BLOWER_FAN_PIN, newState ? HIGH : LOW);
    blowerState = newState;
    Serial.printf("   Pin %d = %s\n", RELAY_BLOWER_FAN_PIN, newState ? "HIGH" : "LOW");
  }
  
  // Print current pin states for verification
  Serial.printf("🔍 Pin readback verification:\n");
  Serial.printf("   IGN (pin %d): %s\n", RELAY_IGNITER_PIN, digitalRead(RELAY_IGNITER_PIN) ? "HIGH" : "LOW");
  Serial.printf("   AUG (pin %d): %s\n", RELAY_AUGER_PIN, digitalRead(RELAY_AUGER_PIN) ? "HIGH" : "LOW");
  Serial.printf("   HOP (pin %d): %s\n", RELAY_HOPPER_FAN_PIN, digitalRead(RELAY_HOPPER_FAN_PIN) ? "HIGH" : "LOW");
  Serial.printf("   BLO (pin %d): %s\n", RELAY_BLOWER_FAN_PIN, digitalRead(RELAY_BLOWER_FAN_PIN) ? "HIGH" : "LOW");
}

void relay_clear_manual() {
  manualOverrideActive = false;
  manualOverrideTimeout = 0;
  Serial.println("🔓 Manual override cleared - auto control restored");
}

void relay_emergency_stop() {
  Serial.println("🚨 EMERGENCY STOP - All relays OFF");
  
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
  
  // Verify all are off
  Serial.printf("🔍 Emergency stop verification:\n");
  Serial.printf("   IGN: %s\n", digitalRead(RELAY_IGNITER_PIN) ? "HIGH" : "LOW");
  Serial.printf("   AUG: %s\n", digitalRead(RELAY_AUGER_PIN) ? "HIGH" : "LOW");
  Serial.printf("   HOP: %s\n", digitalRead(RELAY_HOPPER_FAN_PIN) ? "HIGH" : "LOW");
  Serial.printf("   BLO: %s\n", digitalRead(RELAY_BLOWER_FAN_PIN) ? "HIGH" : "LOW");
}

void relay_clear_emergency() {
  Serial.println("✅ Emergency stop cleared");
}

bool relay_is_safe_state() {
  return true; // Simple version always returns true
}

void relay_print_status() {
  Serial.println("\n=== ENHANCED RELAY STATUS ===");
  
  // Current relay states
  Serial.printf("🔥 Igniter: %s (Pin %d, State: %s)\n", 
                igniterState ? "ON" : "OFF", 
                RELAY_IGNITER_PIN,
                digitalRead(RELAY_IGNITER_PIN) ? "HIGH" : "LOW");
  
  Serial.printf("🌾 Auger: %s (Pin %d, State: %s)\n", 
                augerState ? "ON" : "OFF", 
                RELAY_AUGER_PIN,
                digitalRead(RELAY_AUGER_PIN) ? "HIGH" : "LOW");
  
  Serial.printf("💨 Hopper Fan: %s (Pin %d, State: %s)\n", 
                hopperState ? "ON" : "OFF", 
                RELAY_HOPPER_FAN_PIN,
                digitalRead(RELAY_HOPPER_FAN_PIN) ? "HIGH" : "LOW");
  
  Serial.printf("🌪️ Blower Fan: %s (Pin %d, State: %s)\n", 
                blowerState ? "ON" : "OFF", 
                RELAY_BLOWER_FAN_PIN,
                digitalRead(RELAY_BLOWER_FAN_PIN) ? "HIGH" : "LOW");
  
  // Manual override status
  Serial.printf("🕹️ Manual Override: %s", manualOverrideActive ? "ACTIVE" : "INACTIVE");
  if (manualOverrideActive) {
    unsigned long remaining = (manualOverrideTimeout > millis()) ? 
                              (manualOverrideTimeout - millis()) / 1000 : 0;
    Serial.printf(" (expires in %lu seconds)", remaining);
  }
  Serial.println();
  
  // System health during relay operations
  Serial.printf("📊 System Health: Free heap=%d bytes, Stack=%d bytes\n", 
                ESP.getFreeHeap(), uxTaskGetStackHighWaterMark(NULL));
  
  Serial.println("=============================\n");
}

void relay_apply_state(RelayRequest* request) {
  // Enhanced apply state with preference for manual control
  if (manualOverrideActive) {
    Serial.println("🕹️ Applying state as MANUAL request (override active)");
    relay_request_manual(request);
  } else {
    Serial.println("🤖 Applying state as AUTO request");
    relay_request_auto(request);
  }
}

// Individual relay controls with enhanced debugging
void relay_set_igniter(bool state) {
  RelayRequest req = {RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE};
  req.igniter = state ? RELAY_ON : RELAY_OFF;
  
  Serial.printf("🔥 DIRECT IGNITER CONTROL: Setting to %s\n", state ? "ON" : "OFF");
  relay_request_manual(&req);
}

void relay_set_auger(bool state) {
  RelayRequest req = {RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE};
  req.auger = state ? RELAY_ON : RELAY_OFF;
  
  Serial.printf("🌾 DIRECT AUGER CONTROL: Setting to %s\n", state ? "ON" : "OFF");
  Serial.printf("   📊 System state before auger control:\n");
  Serial.printf("      Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("      Stack remaining: %d bytes\n", uxTaskGetStackHighWaterMark(NULL));
  Serial.printf("      Uptime: %lu seconds\n", millis() / 1000);
  
  // Force this to be manual to avoid conflicts
  relay_request_manual(&req);
  
  // Extra verification for auger operations
  delay(50);
  bool actualState = digitalRead(RELAY_AUGER_PIN);
  Serial.printf("   ✅ Auger verification: Expected %s, Actual %s\n", 
                state ? "HIGH" : "LOW", actualState ? "HIGH" : "LOW");
  
  if (actualState != (state ? HIGH : LOW)) {
    Serial.println("   ❌ CRITICAL: Auger state verification failed!");
  }
}

void relay_set_hopper_fan(bool state) {
  RelayRequest req = {RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE};
  req.hopperFan = state ? RELAY_ON : RELAY_OFF;
  
  Serial.printf("💨 DIRECT HOPPER FAN CONTROL: Setting to %s\n", state ? "ON" : "OFF");
  relay_request_manual(&req);
}

void relay_set_blower_fan(bool state) {
  RelayRequest req = {RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE};
  req.blowerFan = state ? RELAY_ON : RELAY_OFF;
  
  Serial.printf("🌪️ DIRECT BLOWER FAN CONTROL: Setting to %s\n", state ? "ON" : "OFF");
  relay_request_manual(&req);
}

// Enhanced debugging functions
bool relay_get_manual_override_status() {
  return manualOverrideActive;
}

unsigned long relay_get_manual_override_remaining() {
  if (!manualOverrideActive) return 0;
  return (manualOverrideTimeout > millis()) ? (manualOverrideTimeout - millis()) / 1000 : 0;
}

void relay_force_clear_manual() {
  Serial.println("🔧 FORCE clearing manual override");
  manualOverrideActive = false;
  manualOverrideTimeout = 0;
  Serial.println("   ✅ Manual override forcibly cleared - auto control restored");
}

void relay_extend_manual_override() {
  if (manualOverrideActive) {
    manualOverrideTimeout = millis() + MANUAL_OVERRIDE_DURATION;
    Serial.printf("⏰ Manual override extended for %lu more minutes\n", MANUAL_OVERRIDE_DURATION / 60000);
  }
}

// Safety verification functions
bool relay_verify_all_states() {
  bool allGood = true;
  
  // Check igniter
  bool ignActual = digitalRead(RELAY_IGNITER_PIN);
  if (ignActual != (igniterState ? HIGH : LOW)) {
    Serial.printf("❌ Igniter state mismatch: Expected %s, Got %s\n", 
                  igniterState ? "HIGH" : "LOW", ignActual ? "HIGH" : "LOW");
    allGood = false;
  }
  
  // Check auger
  bool augActual = digitalRead(RELAY_AUGER_PIN);
  if (augActual != (augerState ? HIGH : LOW)) {
    Serial.printf("❌ Auger state mismatch: Expected %s, Got %s\n", 
                  augerState ? "HIGH" : "LOW", augActual ? "HIGH" : "LOW");
    allGood = false;
  }
  
  // Check hopper fan
  bool hopActual = digitalRead(RELAY_HOPPER_FAN_PIN);
  if (hopActual != (hopperState ? HIGH : LOW)) {
    Serial.printf("❌ Hopper Fan state mismatch: Expected %s, Got %s\n", 
                  hopperState ? "HIGH" : "LOW", hopActual ? "HIGH" : "LOW");
    allGood = false;
  }
  
  // Check blower fan
  bool bloActual = digitalRead(RELAY_BLOWER_FAN_PIN);
  if (bloActual != (blowerState ? HIGH : LOW)) {
    Serial.printf("❌ Blower Fan state mismatch: Expected %s, Got %s\n", 
                  blowerState ? "HIGH" : "LOW", bloActual ? "HIGH" : "LOW");
    allGood = false;
  }
  
  if (allGood) {
    Serial.println("✅ All relay states verified correctly");
  }
  
  return allGood;
}

void relay_force_sync_states() {
  Serial.println("🔄 Force syncing relay states...");
  
  // Read actual pin states and update our tracking
  igniterState = digitalRead(RELAY_IGNITER_PIN) == HIGH;
  augerState = digitalRead(RELAY_AUGER_PIN) == HIGH;
  hopperState = digitalRead(RELAY_HOPPER_FAN_PIN) == HIGH;
  blowerState = digitalRead(RELAY_BLOWER_FAN_PIN) == HIGH;
  
  Serial.printf("   📊 Synced states: IGN=%s, AUG=%s, HOP=%s, BLO=%s\n",
                igniterState ? "ON" : "OFF",
                augerState ? "ON" : "OFF",
                hopperState ? "ON" : "OFF",
                blowerState ? "ON" : "OFF");
}

// Diagnostic function for troubleshooting relay issues
void relay_run_diagnostics() {
  Serial.println("\n🔬 === RELAY DIAGNOSTICS ===");
  
  Serial.printf("📊 System Status:\n");
  Serial.printf("   Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("   Stack remaining: %d bytes\n", uxTaskGetStackHighWaterMark(NULL));
  Serial.printf("   Uptime: %lu seconds\n", millis() / 1000);
  Serial.printf("   Manual override: %s\n", manualOverrideActive ? "ACTIVE" : "INACTIVE");
  
  Serial.printf("\n🔌 Pin Configuration:\n");
  Serial.printf("   Igniter Pin: %d\n", RELAY_IGNITER_PIN);
  Serial.printf("   Auger Pin: %d\n", RELAY_AUGER_PIN);
  Serial.printf("   Hopper Fan Pin: %d\n", RELAY_HOPPER_FAN_PIN);
  Serial.printf("   Blower Fan Pin: %d\n", RELAY_BLOWER_FAN_PIN);
  
  Serial.printf("\n📈 Current States:\n");
  Serial.printf("   Igniter: Tracked=%s, Actual=%s\n", 
                igniterState ? "ON" : "OFF",
                digitalRead(RELAY_IGNITER_PIN) ? "HIGH" : "LOW");
  Serial.printf("   Auger: Tracked=%s, Actual=%s\n", 
                augerState ? "ON" : "OFF",
                digitalRead(RELAY_AUGER_PIN) ? "HIGH" : "LOW");
  Serial.printf("   Hopper Fan: Tracked=%s, Actual=%s\n", 
                hopperState ? "ON" : "OFF",
                digitalRead(RELAY_HOPPER_FAN_PIN) ? "HIGH" : "LOW");
  Serial.printf("   Blower Fan: Tracked=%s, Actual=%s\n", 
                blowerState ? "ON" : "OFF",
                digitalRead(RELAY_BLOWER_FAN_PIN) ? "HIGH" : "LOW");
  
  // Verify states
  bool statesOK = relay_verify_all_states();
  Serial.printf("\n✅ State Verification: %s\n", statesOK ? "PASSED" : "FAILED");
  
  if (manualOverrideActive) {
    unsigned long remaining = relay_get_manual_override_remaining();
    Serial.printf("⏰ Manual Override: %lu seconds remaining\n", remaining);
  }
  
  Serial.println("===========================\n");
}