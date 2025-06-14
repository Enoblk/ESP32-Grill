// Ignition.cpp - Clean Implementation with Fixed Syntax
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

// Ignition timing parameters (optimized for Daniel Boone)
#define PREHEAT_TIME (2 * 60 * 1000)      // 2 minutes preheat
#define INITIAL_FEED_TIME (30 * 1000)     // 30 seconds initial feed
#define LIGHTING_TIME (8 * 60 * 1000)     // 8 minutes lighting phase
#define FLAME_DETECT_TIME (3 * 60 * 1000) // 3 minutes flame detection
#define STABILIZE_TIME (2 * 60 * 1000)    // 2 minutes stabilization
#define TOTAL_IGNITION_TIME (15 * 60 * 1000) // 15 minutes max total time

// Temperature thresholds
#define IGNITION_SUCCESS_TEMP_RISE 50.0    // 50°F rise indicates success
#define IGNITION_MIN_TEMP_RISE 25.0        // 25°F minimum rise for progress
#define IGNITION_TIMEOUT_TEMP 200.0        // Timeout if we don't reach this temp

void ignition_init() {
  Serial.println("Initializing ignition system...");
  
  currentState = IGNITION_OFF;
  stateStartTime = 0;
  ignitionTargetTemp = 0;
  startingTemp = 0;
  peakTemp = 0;
  ignitionRequested = false;
  
  Serial.println("Ignition system initialized");
}

void ignition_start(double currentTemp) {
  if (currentState != IGNITION_OFF && currentState != IGNITION_FAILED) {
    Serial.println("Ignition already in progress");
    return;
  }
  
  Serial.printf("Starting ignition sequence at %.1f°F\n", currentTemp);
  
  // Initialize ignition parameters
  startingTemp = currentTemp;
  peakTemp = currentTemp;
  ignitionTargetTemp = setpoint;
  ignitionRequested = true;
  
  // Start with preheat phase
  currentState = IGNITION_PREHEAT;
  stateStartTime = millis();
  
  // Turn on igniter and fans
  RelayRequest ignitionReq = {RELAY_ON, RELAY_OFF, RELAY_ON, RELAY_ON};
  relay_request_auto(&ignitionReq);
  
  Serial.println("Ignition: PREHEAT phase started");
}

void ignition_stop() {
  if (currentState == IGNITION_OFF) return;
  
  Serial.println("Stopping ignition sequence");
  
  // Turn off igniter, keep fans running
  RelayRequest stopReq = {RELAY_OFF, RELAY_OFF, RELAY_ON, RELAY_ON};
  relay_request_auto(&stopReq);
  
  currentState = IGNITION_OFF;
  stateStartTime = 0;
  ignitionRequested = false;
}

void ignition_fail() {
  Serial.println("Ignition: FAILED");
  
  currentState = IGNITION_FAILED;
  ignitionRequested = false;
  
  // Turn off igniter and auger, keep fans on for safety
  RelayRequest failReq = {RELAY_OFF, RELAY_OFF, RELAY_ON, RELAY_ON};
  relay_request_auto(&failReq);
  
  // Stop the grill
  grillRunning = false;
}

void ignition_loop() {
  if (!ignitionRequested || currentState == IGNITION_OFF) return;
  
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
  
  // State machine
  switch (currentState) {
    case IGNITION_PREHEAT:
      if (stateTime >= PREHEAT_TIME) {
        // Move to initial feed phase
        currentState = IGNITION_INITIAL_FEED;
        stateStartTime = now;
        
        // Start feeding pellets
        RelayRequest feedReq = {RELAY_ON, RELAY_ON, RELAY_ON, RELAY_ON};
        relay_request_auto(&feedReq);
        
        Serial.println("Ignition: INITIAL_FEED phase started");
      }
      break;
      
    case IGNITION_INITIAL_FEED:
      if (stateTime >= INITIAL_FEED_TIME) {
        // Move to lighting phase
        currentState = IGNITION_LIGHTING;
        stateStartTime = now;
        
        // Continue with igniter on, moderate pellet feeding
        RelayRequest lightReq = {RELAY_ON, RELAY_OFF, RELAY_ON, RELAY_ON};
        relay_request_auto(&lightReq);
        
        Serial.println("Ignition: LIGHTING phase started");
      }
      break;
      
    case IGNITION_LIGHTING:
      // Check for temperature rise indicating ignition
      if (currentTemp > startingTemp + IGNITION_MIN_TEMP_RISE) {
        // We're seeing temperature rise, move to flame detection
        currentState = IGNITION_FLAME_DETECT;
        stateStartTime = now;
        
        Serial.printf("Ignition: FLAME_DETECT phase - temp rise detected (%.1f°F)\n", 
                      currentTemp - startingTemp);
      } else if (stateTime >= LIGHTING_TIME) {
        // No temperature rise after lighting time
        Serial.printf("Ignition: No temperature rise after %.1f minutes\n", 
                      LIGHTING_TIME / 60000.0);
        ignition_fail();
        return;
      }
      
      // Periodic pellet feeding during lighting
      if ((stateTime % 90000) < 15000) { // Feed for 15s every 90s
        RelayRequest feedReq = {RELAY_ON, RELAY_ON, RELAY_ON, RELAY_ON};
        relay_request_auto(&feedReq);
      } else {
        RelayRequest noFeedReq = {RELAY_ON, RELAY_OFF, RELAY_ON, RELAY_ON};
        relay_request_auto(&noFeedReq);
      }
      break;
      
    case IGNITION_FLAME_DETECT:
      // Check for significant temperature rise
      if (currentTemp > startingTemp + IGNITION_SUCCESS_TEMP_RISE) {
        // Good temperature rise, move to stabilization
        currentState = IGNITION_STABILIZE;
        stateStartTime = now;
        
        // Turn off igniter, continue with normal operation
        RelayRequest stabilizeReq = {RELAY_OFF, RELAY_OFF, RELAY_ON, RELAY_ON};
        relay_request_auto(&stabilizeReq);
        
        Serial.printf("Ignition: STABILIZE phase - good temp rise (%.1f°F)\n", 
                      currentTemp - startingTemp);
      } else if (stateTime >= FLAME_DETECT_TIME) {
        // Temperature rise stalled
        if (currentTemp < startingTemp + IGNITION_MIN_TEMP_RISE) {
          Serial.println("Ignition: Temperature rise stalled");
          ignition_fail();
          return;
        } else {
          // Some rise, but not enough - continue to stabilize anyway
          currentState = IGNITION_STABILIZE;
          stateStartTime = now;
          
          RelayRequest stabilizeReq = {RELAY_OFF, RELAY_OFF, RELAY_ON, RELAY_ON};
          relay_request_auto(&stabilizeReq);
          
          Serial.println("Ignition: STABILIZE phase - marginal temp rise");
        }
      }
      break;
      
    case IGNITION_STABILIZE:
      // Check for temperature stability and continued rise
      if (stateTime >= STABILIZE_TIME) {
        if (currentTemp > startingTemp + IGNITION_MIN_TEMP_RISE && 
            currentTemp >= IGNITION_TIMEOUT_TEMP) {
          // Successful ignition
          currentState = IGNITION_COMPLETE;
          ignitionRequested = false;
          
          Serial.printf("Ignition: COMPLETE - Final temp %.1f°F (rise: %.1f°F)\n", 
                        currentTemp, currentTemp - startingTemp);
        } else {
          Serial.printf("Ignition: Failed to reach target temp (%.1f°F)\n", currentTemp);
          ignition_fail();
          return;
        }
      }
      break;
      
    case IGNITION_COMPLETE:
      // Ignition complete, normal operation takes over
      ignitionRequested = false;
      break;
      
    case IGNITION_FAILED:
      // Stay in failed state until manual reset
      break;
      
    default:
      currentState = IGNITION_OFF;
      break;
  }
  
  // Debug output every 30 seconds during ignition
  static unsigned long lastIgnitionDebug = 0;
  if (now - lastIgnitionDebug >= 30000) {
    Serial.printf("Ignition: %s, Temp: %.1f°F (start: %.1f°F, rise: %.1f°F), Time: %lu sec\n",
                  ignition_get_status_string().c_str(),
                  currentTemp, startingTemp, currentTemp - startingTemp,
                  stateTime / 1000);
    lastIgnitionDebug = now;
  }
}

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