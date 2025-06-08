// =========================
// Ignition.cpp
// Handles the ignition phase for ESP32 Grill Controller
// =========================

#include "Ignition.h"
#include "Globals.h"
#include "Utility.h"
#include "RelayControl.h"
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
}

void ignition_loop() {
    if (!ignition_in_progress) return;

    // Build relay request: ALL relays ON for ignition
    RelayRequest req;
    req.igniter   = RELAY_ON;
    req.auger     = RELAY_ON;
    req.hopperFan = RELAY_ON;
    req.blowerFan = RELAY_ON;

    // Send request to relay manager
    relay_request_auto(&req);

    double temp_now = readTemperature();
    unsigned long elapsed = millis() - ignition_start_time;

    // Only turn OFF igniter after ignition completes
    if ((elapsed >= IGNITION_TIME_MS) && (temp_now >= ignition_temp_start + IGNITION_TEMP_DELTA)) {
        // Turn igniter OFF (fans/auger still managed elsewhere)
        req.igniter   = RELAY_OFF;
        req.auger     = RELAY_NOCHANGE;
        req.hopperFan = RELAY_NOCHANGE;
        req.blowerFan = RELAY_NOCHANGE;
        relay_request_auto(&req);

        ignition_in_progress = false;
    }
}

bool ignition_active() {
    return ignition_in_progress;
}
