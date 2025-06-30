// Settings.cpp - Fixed version
#include "Settings.h"
#include "Globals.h"

void load_settings() {
  // Use the global preferences object from Globals.h
  preferences.begin("grill", true);
  
  // Load setpoint
  setpoint = preferences.getFloat("setpoint", 225.0);
  
  // Load other settings as needed
  // Add more settings here later
  
  preferences.end();
  
  Serial.printf("Settings loaded - Setpoint: %.1f°F\n", setpoint);
}

void save_settings() {
  preferences.begin("grill", false);
  
  // Save setpoint
  preferences.putFloat("setpoint", setpoint);
  
  // Save other settings as needed
  // Add more settings here later
  
  preferences.end();
  
  Serial.println("Settings saved");
}