// Ignition.cpp - COMPLETELY CLEAN - Replace your entire file with this
#include "Ignition.h"
#include "Globals.h"
#include "Utility.h"

// Simple 3-state system using existing enums
static IgnitionState currentState = IGNITION_OFF;
static unsigned long stateStartTime = 0;
static unsigned long lastAugerAction = 0;
static bool augerCurrentlyOn = false;

// Simple relay control - NO sensor access
void setRelay(int pin, bool state) {
  digitalWrite(pin, state ? HIGH : LOW);
}

void ignition_init() {
  // Setup relay pins
  pinMode(RELAY_IGNITER_PIN, OUTPUT);
  pinMode(RELAY_AUGER_PIN, OUTPUT);
  pinMode(RELAY_HOPPER_FAN_PIN, OUTPUT);
  pinMode(RELAY_BLOWER_FAN_PIN, OUTPUT);
  
  // All off initially
  setRelay(RELAY_IGNITER_PIN, false);
  setRelay(RELAY_AUGER_PIN, false);
  setRelay(RELAY_HOPPER_FAN_PIN, false);
  setRelay(RELAY_BLOWER_FAN_PIN, false);
  
  currentState = IGNITION_OFF;
  augerCurrentlyOn = false;
}

void ignition_start(double currentTemp) {
  if (currentState == IGNITION_OFF) {
    currentState = IGNITION_LIGHTING; // Start in lighting phase
    stateStartTime = millis();
    lastAugerAction = millis();
    
    // Always turn on fans when starting
    setRelay(RELAY_HOPPER_FAN_PIN, true);
    setRelay(RELAY_BLOWER_FAN_PIN, true);
    
    grillRunning = true;
  }
}

void ignition_stop() {
  currentState = IGNITION_OFF;
  grillRunning = false;
  
  // Turn everything off
  setRelay(RELAY_IGNITER_PIN, false);
  setRelay(RELAY_AUGER_PIN, false);
  setRelay(RELAY_HOPPER_FAN_PIN, false);
  setRelay(RELAY_BLOWER_FAN_PIN, false);
  
  augerCurrentlyOn = false;
}

void ignition_loop() {
  if (!grillRunning || currentState == IGNITION_OFF) {
    return;
  }
  
  unsigned long now = millis();
  double currentTemp = readGrillTemperature();
  
  // Calculate temperature error (how far off target we are)
  double tempError = setpoint - currentTemp;
  
  // PiFire-style auger timing based on temperature error
  unsigned long augerOnTime = 8000;   // Default 8 seconds
  unsigned long augerOffTime = 75000; // Default 75 seconds
  
  // Enhanced error curve with more points for better control
  if (tempError > 100.0) {
    // WAY TOO COLD - emergency heating
    augerOnTime = 30000;   // 30 seconds ON
    augerOffTime = 20000;  // 20 seconds OFF
  } else if (tempError > 75.0) {
    // VERY COLD - aggressive heating  
    augerOnTime = 25000;   // 25 seconds ON
    augerOffTime = 25000;  // 25 seconds OFF
  } else if (tempError > 50.0) {
    // COLD - heavy heating
    augerOnTime = 20000;   // 20 seconds ON
    augerOffTime = 35000;  // 35 seconds OFF
  } else if (tempError > 30.0) {
    // COOL - moderate heating
    augerOnTime = 16000;   // 16 seconds ON
    augerOffTime = 45000;  // 45 seconds OFF
  } else if (tempError > 15.0) {
    // SLIGHTLY COOL - gentle heating
    augerOnTime = 12000;   // 12 seconds ON
    augerOffTime = 55000;  // 55 seconds OFF
  } else if (tempError > 5.0) {
    // CLOSE TO TARGET - fine tuning
    augerOnTime = 10000;   // 10 seconds ON
    augerOffTime = 65000;  // 65 seconds OFF
  } else if (tempError > -5.0) {
    // AT TARGET - maintenance mode
    augerOnTime = 8000;    // 8 seconds ON
    augerOffTime = 75000;  // 75 seconds OFF
  } else if (tempError > -15.0) {
    // SLIGHTLY HOT - reduce pellets
    augerOnTime = 6000;    // 6 seconds ON
    augerOffTime = 85000;  // 85 seconds OFF
  } else if (tempError > -25.0) {
    // HOT - minimal pellets
    augerOnTime = 4000;    // 4 seconds ON
    augerOffTime = 100000; // 100 seconds OFF
  } else {
    // TOO HOT - stop feeding temporarily
    augerOnTime = 0;       // 0 seconds ON
    augerOffTime = 120000; // 2 minutes OFF
  }
  
  // Handle ignition phase special case
  if (currentTemp < 130.0 && currentState == IGNITION_LIGHTING) {
    // During ignition under 130F, run igniter and use ignition timing
    setRelay(RELAY_IGNITER_PIN, true);
    
    // Override with ignition-specific timing (more aggressive)
    augerOnTime = 45000;   // 45 seconds ON
    augerOffTime = 15000;  // 15 seconds OFF
    
    // Safety timeout - stop igniting after 15 minutes
    if ((now - stateStartTime) > (15 * 60 * 1000)) {
      ignition_stop();
      return;
    }
  } else {
    // Turn off igniter after 130F
    setRelay(RELAY_IGNITER_PIN, false);
    if (currentState == IGNITION_LIGHTING) {
      currentState = IGNITION_STABILIZE; // Move to normal operation
    }
  }
  
  // Execute the auger cycle based on calculated timing
  if (!augerCurrentlyOn) {
    // Check if it's time to turn auger ON
    if ((now - lastAugerAction) >= augerOffTime) {
      if (augerOnTime > 0) {  // Only turn on if we want pellets
        setRelay(RELAY_AUGER_PIN, true);
        augerCurrentlyOn = true;
        lastAugerAction = now;
      } else {
        // Skip this cycle if augerOnTime is 0 (too hot)
        lastAugerAction = now;
      }
    }
  } else {
    // Check if it's time to turn auger OFF
    if ((now - lastAugerAction) >= augerOnTime) {
      setRelay(RELAY_AUGER_PIN, false);
      augerCurrentlyOn = false;
      lastAugerAction = now;
    }
  }
  
  // Update status based on temperature error
  if (currentTemp < 130.0) {
    currentState = IGNITION_LIGHTING;
  } else if (abs(tempError) <= 10.0) {
    currentState = IGNITION_COMPLETE;  // At target
  } else {
    currentState = IGNITION_STABILIZE; // Heating/cooling
  }
  
  // Emergency safety - shut down if too hot
  if (currentTemp > 600.0) {
    ignition_stop();
  }
  
  // Debug output every 30 seconds
  static unsigned long lastDebug = 0;
  if (now - lastDebug >= 30000) {
    Serial.printf("🌡️ Temp: %.1f°F, Target: %.1f°F, Error: %.1f°F\n", currentTemp, setpoint, tempError);
    Serial.printf("🌾 Auger: %s, Timing: %lus ON / %lus OFF\n", 
                  augerCurrentlyOn ? "ON" : "OFF", augerOnTime/1000, augerOffTime/1000);
    lastDebug = now;
  }
}

// Status functions - Only use enum values that compiler recognizes
IgnitionState ignition_get_state() {
  return currentState;
}

String ignition_get_status_string() {
  switch (currentState) {
    case IGNITION_OFF: return "OFF";
    case IGNITION_PREHEAT: return "PREHEAT";
    case IGNITION_LIGHTING: return "IGNITING";
    case IGNITION_STABILIZE: return "HEATING";
    case IGNITION_COMPLETE: return "MAINTAINING";
    case IGNITION_FAILED: return "FAILED";
    default: return "UNKNOWN";
  }
}

bool ignition_is_complete() {
  return currentState == IGNITION_COMPLETE;
}

bool ignition_has_failed() {
  return currentState == IGNITION_FAILED;
}

void ignition_set_target_temp(double temp) {
  setpoint = temp;
}

double ignition_get_target_temp() {
  return setpoint;
}

// Keep the PiFire functions simple
void pifire_auger_cycle() {
  // Do nothing - control is now in ignition_loop()
}

void pifire_manual_auger_prime() {
  // Simple 30-second prime
  if (!grillRunning) {
    setRelay(RELAY_AUGER_PIN, true);
    setRelay(RELAY_HOPPER_FAN_PIN, true);
    delay(30000);  // 30 second prime
    setRelay(RELAY_AUGER_PIN, false);
    setRelay(RELAY_HOPPER_FAN_PIN, false);
  }
}

void pifire_temperature_control() {
  // Do nothing - control is now in ignition_loop()
}

String pifire_get_status() {
  if (!grillRunning) return "IDLE";
  return augerCurrentlyOn ? "FEEDING" : "WAITING";
}