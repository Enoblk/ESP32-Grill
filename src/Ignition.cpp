// Ignition.cpp - Complete PiFire auger control that replaces ALL pellet control
#include "Ignition.h"
#include "Globals.h"
#include "Utility.h"
#include "RelayControl.h"

// Ignition state tracking
static IgnitionState currentState = IGNITION_OFF;
static unsigned long stateStartTime = 0;
static double ignitionTargetTemp = 0;
static double startingTemp = 0;
static double peakTemp = 0;
static bool ignitionRequested = false;

// Ignition timing parameters (adjustable as requested)
#define PREHEAT_TIME (2 * 60 * 1000)      // 2 minutes preheat
#define INITIAL_FEED_TIME (30 * 1000)     // 30 seconds initial feed
#define LIGHTING_TIME (10 * 60 * 1000)    // 10 minutes lighting phase (was 8)
#define FLAME_DETECT_TIME (5 * 60 * 1000) // 5 minutes flame detection (was 3)
#define STABILIZE_TIME (2 * 60 * 1000)    // 2 minutes stabilization
#define TOTAL_IGNITION_TIME (20 * 60 * 1000) // 20 minutes max total time (was 15)

// Temperature thresholds (adjusted as requested)
#define IGNITION_SUCCESS_TEMP_RISE 50.0    // 50°F rise indicates success
#define IGNITION_MIN_TEMP_RISE 15.0        // 15°F minimum rise for progress (was 25°F)
#define IGNITION_TIMEOUT_TEMP 200.0        // Timeout if we don't reach this temp

// PiFire-style auger control structure
struct PiFireAugerControl {
  unsigned long lastCycleTime = 0;
  bool augerCurrentlyOn = false;
  unsigned long currentCycleStartTime = 0;
  
  // PiFire-style cycle parameters that respond to temperature error
  unsigned long baseAugerOnTime = 15000;     // Base 15 seconds ON
  unsigned long baseAugerOffTime = 60000;    // Base 60 seconds OFF
  unsigned long primeAmount = 30000;         // 30 seconds for priming
  
  // Current calculated times (adjusted based on temperature error)
  unsigned long currentOnTime = 15000;
  unsigned long currentOffTime = 60000;
};

static PiFireAugerControl piFireAuger;

// Calculate auger timing based on temperature error (PiFire style with temp response)
void pifire_calculate_timing() {
  if (!grillRunning) return;
  
  double currentTemp = readTemperature();
  double targetTemp = setpoint;
  double tempError = targetTemp - currentTemp;
  
  // PiFire-style temperature response - adjust timing based on how far off we are
  if (tempError > 50.0) {
    // Way too cold - more pellets
    piFireAuger.currentOnTime = 20000;   // 20 seconds
    piFireAuger.currentOffTime = 45000;  // 45 seconds
  } else if (tempError > 25.0) {
    // Cold - more pellets
    piFireAuger.currentOnTime = 18000;   // 18 seconds
    piFireAuger.currentOffTime = 50000;  // 50 seconds
  } else if (tempError > 10.0) {
    // Slightly cold - slightly more pellets
    piFireAuger.currentOnTime = 16000;   // 16 seconds
    piFireAuger.currentOffTime = 55000;  // 55 seconds
  } else if (tempError > -5.0) {
    // Near target - normal feeding
    piFireAuger.currentOnTime = 15000;   // 15 seconds (normal)
    piFireAuger.currentOffTime = 60000;  // 60 seconds (normal)
  } else if (tempError > -15.0) {
    // Slightly hot - less pellets
    piFireAuger.currentOnTime = 12000;   // 12 seconds
    piFireAuger.currentOffTime = 75000;  // 75 seconds
  } else if (tempError > -25.0) {
    // Hot - much less pellets
    piFireAuger.currentOnTime = 8000;    // 8 seconds
    piFireAuger.currentOffTime = 90000;  // 90 seconds
  } else {
    // Way too hot - minimal pellets
    piFireAuger.currentOnTime = 5000;    // 5 seconds
    piFireAuger.currentOffTime = 120000; // 2 minutes
  }
  
  // Debug output every 30 seconds
  static unsigned long lastTempDebug = 0;
  if (millis() - lastTempDebug >= 30000) {
    Serial.printf("PiFire Temp Control: Current=%.1f°F, Target=%.1f°F, Error=%.1f°F\n",
                  currentTemp, targetTemp, tempError);
    Serial.printf("PiFire Timing: ON=%lu sec, OFF=%lu sec\n",
                  piFireAuger.currentOnTime / 1000, piFireAuger.currentOffTime / 1000);
    lastTempDebug = millis();
  }
}

// FIXED: PiFire auger control - no recursion
void pifire_auger_cycle() {
  unsigned long now = millis();
  
  // Don't do anything if grill isn't running
  if (!grillRunning) {
    if (piFireAuger.augerCurrentlyOn) {
      RelayRequest stopReq = {RELAY_NOCHANGE, RELAY_OFF, RELAY_NOCHANGE, RELAY_NOCHANGE};
      relay_request_auto(&stopReq);
      piFireAuger.augerCurrentlyOn = false;
    }
    return;
  }
  
  IgnitionState currentState = ignition_get_state();
  
  // Special handling for prime during initial feed
  if (currentState == IGNITION_INITIAL_FEED) {
    if (!piFireAuger.augerCurrentlyOn && (now - piFireAuger.lastCycleTime) > 5000) {
      RelayRequest primeReq = {RELAY_NOCHANGE, RELAY_ON, RELAY_NOCHANGE, RELAY_NOCHANGE};
      relay_request_auto(&primeReq);
      piFireAuger.augerCurrentlyOn = true;
      piFireAuger.currentCycleStartTime = now;
      Serial.println("PiFire Auger: Prime cycle started");
    } else if (piFireAuger.augerCurrentlyOn && (now - piFireAuger.currentCycleStartTime) > piFireAuger.primeAmount) {
      RelayRequest stopReq = {RELAY_NOCHANGE, RELAY_OFF, RELAY_NOCHANGE, RELAY_NOCHANGE};
      relay_request_auto(&stopReq);
      piFireAuger.augerCurrentlyOn = false;
      piFireAuger.lastCycleTime = now;
      Serial.println("PiFire Auger: Prime cycle complete");
    }
    return;
  }
  
  // For ALL other states - run temperature-responsive cycling
  // Calculate timing based on temperature error
  pifire_calculate_timing();
  
  if (!piFireAuger.augerCurrentlyOn) {
    // Check if it's time to turn auger ON
    if ((now - piFireAuger.lastCycleTime) >= piFireAuger.currentOffTime) {
      RelayRequest onReq = {RELAY_NOCHANGE, RELAY_ON, RELAY_NOCHANGE, RELAY_NOCHANGE};
      relay_request_auto(&onReq);
      piFireAuger.augerCurrentlyOn = true;
      piFireAuger.currentCycleStartTime = now;
      Serial.printf("PiFire Auger: ON cycle (%lu sec)\n", piFireAuger.currentOnTime / 1000);
    }
  } else {
    // Check if it's time to turn auger OFF
    if ((now - piFireAuger.currentCycleStartTime) >= piFireAuger.currentOnTime) {
      RelayRequest offReq = {RELAY_NOCHANGE, RELAY_OFF, RELAY_NOCHANGE, RELAY_NOCHANGE};
      relay_request_auto(&offReq);
      piFireAuger.augerCurrentlyOn = false;
      piFireAuger.lastCycleTime = now;
      Serial.printf("PiFire Auger: OFF cycle (%lu sec)\n", piFireAuger.currentOffTime / 1000);
    }
  }
}

void ignition_init() {
  Serial.println("Initializing ignition system with COMPLETE PiFire auger control...");
  
  currentState = IGNITION_OFF;
  stateStartTime = 0;
  ignitionTargetTemp = 0;
  startingTemp = 0;
  peakTemp = 0;
  ignitionRequested = false;
  
  // Initialize PiFire auger control
  piFireAuger.lastCycleTime = 0;
  piFireAuger.augerCurrentlyOn = false;
  piFireAuger.currentCycleStartTime = 0;
  piFireAuger.currentOnTime = piFireAuger.baseAugerOnTime;
  piFireAuger.currentOffTime = piFireAuger.baseAugerOffTime;
  
  Serial.println("PiFire auger control will handle ALL pellet feeding and temperature response");
}

void ignition_start(double currentTemp) {
  if (currentState != IGNITION_OFF && currentState != IGNITION_FAILED) {
    Serial.println("Ignition already in progress");
    return;
  }
  
  Serial.printf("Starting ignition sequence at %.1f°F with COMPLETE PiFire auger control\n", currentTemp);
  
  // Initialize ignition parameters
  startingTemp = currentTemp;
  peakTemp = currentTemp;
  ignitionTargetTemp = setpoint;
  ignitionRequested = true;
  
  // Reset PiFire auger state
  piFireAuger.lastCycleTime = millis();
  piFireAuger.augerCurrentlyOn = false;
  piFireAuger.currentCycleStartTime = 0;
  
  // Start with preheat phase
  currentState = IGNITION_PREHEAT;
  stateStartTime = millis();
  
  // Turn on fans only during preheat - no igniter yet
  RelayRequest preheatReq = {RELAY_OFF, RELAY_OFF, RELAY_ON, RELAY_ON};
  relay_request_auto(&preheatReq);
  
  Serial.println("Ignition: PREHEAT phase started");
}

void ignition_stop() {
  if (currentState == IGNITION_OFF) return;
  
  Serial.println("Stopping ignition sequence - PiFire auger will continue if grill running");
  
  // Turn off igniter, but PiFire auger control continues if grill is still running
  RelayRequest stopReq = {RELAY_OFF, RELAY_NOCHANGE, RELAY_ON, RELAY_ON};
  relay_request_auto(&stopReq);
  
  currentState = IGNITION_OFF;
  stateStartTime = 0;
  ignitionRequested = false;
  
  // Don't reset auger state - let it continue for temperature control
  Serial.println("PiFire auger control continues for temperature maintenance");
}

void ignition_fail() {
  Serial.println("Ignition: FAILED");
  
  currentState = IGNITION_FAILED;
  ignitionRequested = false;
  
  // Turn off igniter and auger, keep fans on for safety
  RelayRequest failReq = {RELAY_OFF, RELAY_OFF, RELAY_ON, RELAY_ON};
  relay_request_auto(&failReq);
  
  // Reset PiFire auger state
  piFireAuger.augerCurrentlyOn = false;
  piFireAuger.lastCycleTime = 0;
  
  // Stop the grill
  grillRunning = false;
}

void ignition_loop() {
  // ALWAYS run PiFire auger control if grill is running
  if (grillRunning) {
    pifire_auger_cycle();
  }
  
  // Only do ignition state management if ignition is active
  if (!ignitionRequested || currentState == IGNITION_OFF) {
    return;
  }
  
  unsigned long now = millis();
  unsigned long stateTime = now - stateStartTime;
  double currentTemp = readTemperature();
  
  // Update peak temperature
  if (currentTemp > peakTemp) {
    peakTemp = currentTemp;
  }
  
  // Safety timeout - total ignition time
  if (stateTime > TOTAL_IGNITION_TIME) {
    Serial.println("Ignition: TIMEOUT - Taking too long");
    ignition_fail();
    return;
  }
  
  // STATE MACHINE
  switch (currentState) {
    case IGNITION_PREHEAT:
      if (stateTime >= PREHEAT_TIME) {
        currentState = IGNITION_INITIAL_FEED;
        stateStartTime = now;
        Serial.println("Ignition: INITIAL_FEED phase started");
      }
      break;
      
    case IGNITION_INITIAL_FEED:
      if (stateTime >= INITIAL_FEED_TIME) {
        currentState = IGNITION_LIGHTING;
        stateStartTime = now;
        
        // Turn on igniter and fans
        RelayRequest lightReq = {RELAY_ON, RELAY_NOCHANGE, RELAY_ON, RELAY_ON};
        relay_request_auto(&lightReq);
        
        Serial.println("Ignition: LIGHTING phase started");
      }
      break;
      
    case IGNITION_LIGHTING:
      if (currentTemp > startingTemp + IGNITION_MIN_TEMP_RISE) {
        currentState = IGNITION_FLAME_DETECT;
        stateStartTime = now;
        Serial.printf("Ignition: FLAME_DETECT phase - temp rise detected (%.1f°F)\n", 
                      currentTemp - startingTemp);
      } else if (stateTime >= LIGHTING_TIME) {
        Serial.printf("Ignition: No temperature rise after %.1f minutes\n", 
                      LIGHTING_TIME / 60000.0);
        ignition_fail();
        return;
      }
      break;
      
    case IGNITION_FLAME_DETECT:
      if (currentTemp > startingTemp + IGNITION_SUCCESS_TEMP_RISE) {
        currentState = IGNITION_STABILIZE;
        stateStartTime = now;
        
        // Turn off igniter
        RelayRequest stabilizeReq = {RELAY_OFF, RELAY_NOCHANGE, RELAY_ON, RELAY_ON};
        relay_request_auto(&stabilizeReq);
        
        Serial.printf("Ignition: STABILIZE phase - good temp rise (%.1f°F)\n", 
                      currentTemp - startingTemp);
      } else if (stateTime >= FLAME_DETECT_TIME) {
        if (currentTemp < startingTemp + IGNITION_MIN_TEMP_RISE) {
          Serial.println("Ignition: Temperature rise stalled");
          ignition_fail();
          return;
        } else {
          currentState = IGNITION_STABILIZE;
          stateStartTime = now;
          
          RelayRequest stabilizeReq = {RELAY_OFF, RELAY_NOCHANGE, RELAY_ON, RELAY_ON};
          relay_request_auto(&stabilizeReq);
          
          Serial.println("Ignition: STABILIZE phase - marginal temp rise");
        }
      }
      break;
      
    case IGNITION_STABILIZE:
      if (stateTime >= STABILIZE_TIME) {
        if (currentTemp > startingTemp + IGNITION_MIN_TEMP_RISE && 
            currentTemp >= IGNITION_TIMEOUT_TEMP) {
          currentState = IGNITION_COMPLETE;
          ignitionRequested = false;
          
          Serial.printf("Ignition: COMPLETE - PiFire auger continues temperature control\n");
        } else {
          Serial.printf("Ignition: Failed to reach target temp (%.1f°F)\n", currentTemp);
          ignition_fail();
          return;
        }
      }
      break;
      
    default:
      break;
  }
  
  // Debug output every 30 seconds during ignition
  static unsigned long lastIgnitionDebug = 0;
  if (now - lastIgnitionDebug >= 30000) {
    Serial.printf("Ignition: %s, Temp: %.1f°F, Time: %lu sec\n",
                  ignition_get_status_string().c_str(),
                  currentTemp, stateTime / 1000);
    lastIgnitionDebug = now;
  }
}

// Manual auger prime function (PiFire style)
void pifire_manual_auger_prime() {
  Serial.println("PiFire Manual Prime: Starting 30 second prime");
  
  // Reset cycle state
  piFireAuger.augerCurrentlyOn = false;
  piFireAuger.lastCycleTime = millis();
  
  // Simple manual prime
  RelayRequest primeReq = {RELAY_NOCHANGE, RELAY_ON, RELAY_NOCHANGE, RELAY_NOCHANGE};
  relay_request_manual(&primeReq);
  
  delay(30000);  // 30 second prime
  
  RelayRequest stopReq = {RELAY_NOCHANGE, RELAY_OFF, RELAY_NOCHANGE, RELAY_NOCHANGE};
  relay_request_manual(&stopReq);
  
  Serial.println("PiFire Manual Prime: Complete");
}

// Global function that can be called from main loop
void pifire_temperature_control() {
  // This is the main function that replaces all pellet control
  pifire_auger_cycle();
}

// Status functions to show PiFire auger status
String pifire_get_status() {
  if (!grillRunning) return "IDLE";
  
  if (piFireAuger.augerCurrentlyOn) {
    unsigned long remaining = piFireAuger.currentOnTime - (millis() - piFireAuger.currentCycleStartTime);
    return "FEEDING (" + String(remaining / 1000) + "s ON)";
  } else {
    unsigned long remaining = piFireAuger.currentOffTime - (millis() - piFireAuger.lastCycleTime);
    return "WAITING (" + String(remaining / 1000) + "s OFF)";
  }
}

// Existing status functions
IgnitionState ignition_get_state() {
  return currentState;
}

String ignition_get_status_string() {
  switch (currentState) {
    case IGNITION_OFF: return "OFF";
    case IGNITION_PREHEAT: return "PREHEAT";
    case IGNITION_INITIAL_FEED: return "INITIAL FEED";
    case IGNITION_LIGHTING: return "LIGHTING";
    case IGNITION_FLAME_DETECT: return "FLAME DETECT";
    case IGNITION_STABILIZE: return "STABILIZE";
    case IGNITION_COMPLETE: return "COMPLETE";
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
  ignitionTargetTemp = temp;
}

double ignition_get_target_temp() {
  return ignitionTargetTemp;
}