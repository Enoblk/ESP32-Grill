// Globals.cpp - Fixed version
#include "Globals.h"

// Global variables - only declare variables here, not pins (those are #defines)
bool grillRunning = false;
double setpoint = 225.0;  // Default temperature
AsyncWebServer server(80);
Preferences preferences;

// Remove the conflicting pin variable declarations
// The pins are already defined as #defines in Globals.h

// Settings functions
void save_setpoint() {
  preferences.begin("grill", false);
  preferences.putFloat("setpoint", setpoint);
  preferences.end();
  Serial.printf("Setpoint saved: %.1f°F\n", setpoint);
}

void load_setpoint() {
  preferences.begin("grill", true);
  setpoint = preferences.getFloat("setpoint", 225.0); // Default to 225°F
  preferences.end();
  Serial.printf("Setpoint loaded: %.1f°F\n", setpoint);
}