// =========================
// PelletControl.cpp (modular, no direct ignition handling)
// =========================

#include "PelletControl.h"
#include "Globals.h"
#include "Utility.h"
#include "Ignition.h"
#include <Arduino.h>

// Only manage auger and fans. Never touch igniter relay in this file!

void pellet_feed_loop() {
    static float pidIntegral = 0;
    static float pidLastError = 0;
    static unsigned long augerOnUntil = 0;
    static bool augerCurrentlyOn = false;

    double temp = readTemperature();
    unsigned long now = millis();

    // Hopper and blower fans management
if (grillRunning && !ignition_active()) {
    digitalWrite(RELAY_HOPPER_FAN_PIN, HIGH);
    digitalWrite(RELAY_BLOWER_FAN_PIN, HIGH);
} else if (!grillRunning && temp >= 100.0) {
    digitalWrite(RELAY_HOPPER_FAN_PIN, HIGH);  // Cooldown fan ON
    digitalWrite(RELAY_BLOWER_FAN_PIN, LOW);   // Optionally leave blower OFF
} else {
    digitalWrite(RELAY_HOPPER_FAN_PIN, LOW);
    digitalWrite(RELAY_BLOWER_FAN_PIN, LOW);
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
            digitalWrite(RELAY_AUGER_PIN, HIGH);
            augerCurrentlyOn = true;
        } else {
            digitalWrite(RELAY_AUGER_PIN, LOW);
            augerCurrentlyOn = false;
        }
    } else {
        digitalWrite(RELAY_AUGER_PIN, LOW);
        augerCurrentlyOn = false;
    }
}
