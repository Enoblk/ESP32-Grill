#include "Globals.h"

bool grillRunning = false;
double setpoint = 225.0;
WebServer server(80);
Preferences preferences;

void clamp_setpoint() {
  if (setpoint < MIN_SETPOINT) setpoint = MIN_SETPOINT;
  if (setpoint > MAX_SETPOINT) setpoint = MAX_SETPOINT;
}

void save_setpoint() {
  clamp_setpoint();
  if (!preferences.begin("grill", false)) {
    Serial.println("Preferences: failed to open grill namespace for write");
    return;
  }
  preferences.putFloat("setpoint", setpoint);
  preferences.end();
  Serial.printf("Setpoint saved: %.1f F\n", setpoint);
}

void load_setpoint() {
  if (!preferences.begin("grill", true)) {
    Serial.println("Preferences: failed to open grill namespace for read");
    setpoint = 225.0;
    return;
  }
  setpoint = preferences.getFloat("setpoint", 225.0);
  preferences.end();
  clamp_setpoint();
  Serial.printf("Setpoint loaded: %.1f F\n", setpoint);
}
