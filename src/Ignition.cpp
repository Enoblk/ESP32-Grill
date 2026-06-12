#include "Ignition.h"
#include "Globals.h"
#include "Utility.h"

static IgnitionState currentState = IGNITION_OFF;
static unsigned long stateStartTime = 0;
static unsigned long lastAugerAction = 0;
static bool augerCurrentlyOn = false;
static bool primeActive = false;
static unsigned long primeEndTime = 0;

static void setRelay(int pin, bool state) {
  digitalWrite(pin, state ? HIGH : LOW);
}

static void allRelaysOff() {
  setRelay(RELAY_IGNITER_PIN, false);
  setRelay(RELAY_AUGER_PIN, false);
  setRelay(RELAY_HOPPER_FAN_PIN, false);
  setRelay(RELAY_BLOWER_FAN_PIN, false);
  augerCurrentlyOn = false;
}

static void servicePrime(unsigned long now) {
  if (!primeActive) return;
  if ((long)(now - primeEndTime) >= 0) {
    primeActive = false;
    setRelay(RELAY_AUGER_PIN, false);
    setRelay(RELAY_HOPPER_FAN_PIN, false);
    Serial.println("Auger prime complete");
  }
}

void ignition_init() {
  pinMode(RELAY_IGNITER_PIN, OUTPUT);
  pinMode(RELAY_AUGER_PIN, OUTPUT);
  pinMode(RELAY_HOPPER_FAN_PIN, OUTPUT);
  pinMode(RELAY_BLOWER_FAN_PIN, OUTPUT);
  allRelaysOff();
  currentState = IGNITION_OFF;
  stateStartTime = millis();
}

void ignition_start(double currentTemp) {
  if (currentState != IGNITION_OFF || primeActive) return;
  if (!isValidTemperature(currentTemp)) {
    Serial.println("Start refused: grill temperature sensor is not healthy");
    currentState = IGNITION_FAILED;
    return;
  }

  currentState = currentTemp >= 130.0 ? IGNITION_STABILIZE : IGNITION_LIGHTING;
  stateStartTime = millis();
  lastAugerAction = millis();
  grillRunning = true;
  setRelay(RELAY_HOPPER_FAN_PIN, true);
  setRelay(RELAY_BLOWER_FAN_PIN, true);
  Serial.println("Grill start accepted");
}

void ignition_stop() {
  currentState = IGNITION_OFF;
  grillRunning = false;
  primeActive = false;
  allRelaysOff();
  stateStartTime = millis();
}

void ignition_emergency_stop() {
  Serial.println("Emergency stop: relays off");
  ignition_stop();
  currentState = IGNITION_FAILED;
}

void ignition_loop() {
  unsigned long now = millis();
  servicePrime(now);

  if (!grillRunning || currentState == IGNITION_OFF || currentState == IGNITION_FAILED) return;

  double currentTemp = readGrillTemperature();
  if (!isValidTemperature(currentTemp) || !grillTemperatureHealthy()) {
    Serial.println("Emergency stop: grill temperature sensor lost");
    ignition_emergency_stop();
    return;
  }

  if (currentTemp >= EMERGENCY_TEMP) {
    Serial.printf("Emergency stop: %.1f F exceeds %.1f F\n", currentTemp, EMERGENCY_TEMP);
    ignition_emergency_stop();
    return;
  }

  double tempError = setpoint - currentTemp;
  unsigned long augerOnTime = 8000;
  unsigned long augerOffTime = 75000;

  if (tempError > 100.0) {
    augerOnTime = 30000; augerOffTime = 20000;
  } else if (tempError > 75.0) {
    augerOnTime = 25000; augerOffTime = 25000;
  } else if (tempError > 50.0) {
    augerOnTime = 20000; augerOffTime = 35000;
  } else if (tempError > 30.0) {
    augerOnTime = 16000; augerOffTime = 45000;
  } else if (tempError > 15.0) {
    augerOnTime = 12000; augerOffTime = 55000;
  } else if (tempError > 5.0) {
    augerOnTime = 10000; augerOffTime = 65000;
  } else if (tempError > -5.0) {
    augerOnTime = 8000; augerOffTime = 75000;
  } else if (tempError > -15.0) {
    augerOnTime = 6000; augerOffTime = 85000;
  } else if (tempError > -25.0) {
    augerOnTime = 4000; augerOffTime = 100000;
  } else {
    augerOnTime = 0; augerOffTime = 120000;
  }

  if (currentState == IGNITION_LIGHTING && currentTemp < 130.0) {
    setRelay(RELAY_IGNITER_PIN, true);
    augerOnTime = 45000;
    augerOffTime = 15000;
    if (now - stateStartTime > 15UL * 60UL * 1000UL) {
      Serial.println("Ignition failed: startup timeout");
      ignition_emergency_stop();
      return;
    }
  } else {
    setRelay(RELAY_IGNITER_PIN, false);
    if (currentState == IGNITION_LIGHTING) currentState = IGNITION_STABILIZE;
  }

  if (!augerCurrentlyOn && now - lastAugerAction >= augerOffTime) {
    if (augerOnTime > 0) {
      setRelay(RELAY_AUGER_PIN, true);
      augerCurrentlyOn = true;
    }
    lastAugerAction = now;
  } else if (augerCurrentlyOn && now - lastAugerAction >= augerOnTime) {
    setRelay(RELAY_AUGER_PIN, false);
    augerCurrentlyOn = false;
    lastAugerAction = now;
  }

  if (currentTemp < 130.0) {
    currentState = IGNITION_LIGHTING;
  } else if (abs(tempError) <= 10.0) {
    currentState = IGNITION_COMPLETE;
  } else {
    currentState = IGNITION_STABILIZE;
  }

  static unsigned long lastDebug = 0;
  if (now - lastDebug >= 30000) {
    Serial.printf("Temp %.1f F target %.1f F error %.1f F auger %s (%lus/%lus)\n",
                  currentTemp, setpoint, tempError, augerCurrentlyOn ? "ON" : "OFF",
                  augerOnTime / 1000, augerOffTime / 1000);
    lastDebug = now;
  }
}

bool auger_prime_start(unsigned long durationMs) {
  if (grillRunning || primeActive) return false;
  primeActive = true;
  primeEndTime = millis() + durationMs;
  setRelay(RELAY_AUGER_PIN, true);
  setRelay(RELAY_HOPPER_FAN_PIN, true);
  Serial.printf("Auger prime started for %lu ms\n", durationMs);
  return true;
}

bool auger_prime_active() {
  return primeActive;
}

unsigned long auger_prime_remaining_ms() {
  if (!primeActive) return 0;
  long remaining = (long)(primeEndTime - millis());
  return remaining > 0 ? (unsigned long)remaining : 0;
}

IgnitionState ignition_get_state() { return currentState; }

String ignition_get_status_string() {
  if (primeActive) return "PRIMING";
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

bool ignition_is_active() { return grillRunning; }
bool ignition_is_complete() { return currentState == IGNITION_COMPLETE; }
bool ignition_has_failed() { return currentState == IGNITION_FAILED; }
unsigned long ignition_get_runtime() { return millis() - stateStartTime; }

void ignition_set_target_temp(double temp) {
  setpoint = temp;
  clamp_setpoint();
}

double ignition_get_target_temp() { return setpoint; }
void ignition_complete() { currentState = IGNITION_COMPLETE; }
void ignition_fail() { ignition_emergency_stop(); }
