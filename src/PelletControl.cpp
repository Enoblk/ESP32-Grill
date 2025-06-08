// =========================
// PelletControl.cpp (modular, using RelayControl)
// =========================

#include "PelletControl.h"
#include "Globals.h"
#include "Utility.h"
#include "Ignition.h"
#include "RelayControl.h"   // <-- NEW include
#include <Arduino.h>

// Only manage auger and fans. Never touch igniter relay in this file!

void pellet_feed_loop() {
    static float pidIntegral = 0;
    static float pidLastError = 0;

    double temp = readTemperature();
    unsigned long now = millis();

    // Relay request structure
    RelayRequest req;

    // Hopper and blower fans management
    if (grillRunning && !ignition_active()) {
        req.hopperFan = RELAY_ON;
        req.blowerFan = RELAY_ON;
    } else if (!grillRunning && temp >= 100.0) {
        req.hopperFan = RELAY_ON;    // Cooldown fan ON
        req.blowerFan = RELAY_OFF;   // Optionally leave blower OFF
    } else {
        req.hopperFan = RELAY_OFF;
        req.blowerFan = RELAY_OFF;
    }

    // Pellet feed PID logic (runs ONLY when grillRunning & not in ignition)
    if (grillRunning && !ignition_active()) {
        // Simple proportional control
        float Kp = 2.0, Ki = 0.07, Kd = 0.0;
        float error = setpoint - temp;
        pidIntegral += error;
        float pid = Kp * error + Ki * pidIntegral + Kd * (error - pidLastError);
        pidLastError = error;
        pid = constrain(pid, 0, 100); // Clamp PID
        // Duty cycle period (10 seconds)
        unsigned long dutyPeriod = 10000;
        unsigned long onTime = (unsigned long)((pid / 100.0) * dutyPeriod);
        unsigned long phase = now % dutyPeriod;
        if (phase < onTime) {
            req.auger = RELAY_ON;
        } else {
            req.auger = RELAY_OFF;
        }
    } else {
        req.auger = RELAY_OFF;
    }

    // Never touch igniter relay in this file!
    req.igniter = RELAY_NOCHANGE;

    // Make the request to the relay manager
    relay_request_auto(&req);
}
