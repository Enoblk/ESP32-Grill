// =========================
// Ignition.cpp
// Handles the ignition phase for ESP32 Grill Controller
// =========================

#include "Ignition.h"
#include "Globals.h"
#include "Utility.h"
#include <Arduino.h>

static bool ignition_in_progress = false;
static unsigned long ignition_start_time = 0;
static double ignition_temp_start = 0;
const unsigned long IGNITION_TIME_MS = 5UL * 60UL * 1000UL; // 5 minutes
const double IGNITION_TEMP_DELTA = 10.0; // 10°F rise

void ignition_start(double temp_start) {
    ignition_in_progress = true;
    ignition_start_time = millis();
    ignition_temp_start = temp_start;
    //Serial.println("Ignition start triggered!");

}

void ignition_loop() {
    
    if (!ignition_in_progress) return;
    // Always keep ALL required relays ON during ignition
    digitalWrite(RELAY_IGNITER_PIN, HIGH);      // Igniter ON
    digitalWrite(RELAY_AUGER_PIN, HIGH);        // Auger ON
    digitalWrite(RELAY_HOPPER_FAN_PIN, HIGH);   // Hopper Fan ON
    digitalWrite(RELAY_BLOWER_FAN_PIN, HIGH);   // Blower ON
    //Serial.println("IGNITION PHASE: relays ON!");


    double temp_now = readTemperature();
    unsigned long elapsed = millis() - ignition_start_time;
    // Only turn OFF igniter AFTER ignition completes
    if ((elapsed >= IGNITION_TIME_MS) && (temp_now >= ignition_temp_start + IGNITION_TEMP_DELTA)) {
        digitalWrite(RELAY_IGNITER_PIN, LOW);   // Turn igniter OFF
        ignition_in_progress = false;

    }
}

bool ignition_active() {
    return ignition_in_progress;
}