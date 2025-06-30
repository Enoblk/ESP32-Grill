// Utility.cpp - Clean version for temperature reading
#include "Utility.h" 
#include "Globals.h"
#include "MAX31865Sensor.h"
#include "Ignition.h"

// Keep debug flags but make them simple
bool debugGrillSensor = false;

// Temperature validation
bool isValidTemperature(double temp) {
  if (isnan(temp) || isinf(temp)) return false;
  if (temp <= -900.0 || temp >= 999.0) return false;
  return true;
}

// Simple debug control
void setGrillDebug(bool enabled) {
  debugGrillSensor = enabled;
}

void setAllDebug(bool enabled) {
  debugGrillSensor = enabled;
}

bool getGrillDebug() { return debugGrillSensor; }

// Simple temperature reading - restore original approach  
double readGrillTemperature() {
  static unsigned long lastReading = 0;
  static double cachedTemp = 70.0;
  
  // Only read every 1 second 
  if (millis() - lastReading < 1000) {
    return cachedTemp;
  }
  
  // Read from MAX31865 - simple approach like original
  double temp = grillSensor.readTemperatureF();
  
  if (isValidTemperature(temp)) {
    cachedTemp = temp;
    lastReading = millis();
    return temp;
  }
  
  // Return cached value if current reading is bad
  return cachedTemp;
}

// Simple temperature function for compatibility
double readTemperature() {
  return readGrillTemperature();
}

// STATUS FUNCTION - Keep simple
String getStatus(double temp) {
  if (!grillRunning) {
    return "IDLE";
  }
  
  if (!isValidTemperature(temp)) {
    return "SENSOR ERROR";
  }
  
  // Use ignition state for status
  return ignition_get_status_string();
}

// Stub functions to keep compatibility but remove complexity
void setupTemperatureCalibration() {
  // Do nothing - keep simple
}

void handleCalibrationCommands(String command) {
  // Do nothing - remove serial command processing
}

void printCalibrationStatus() {
  // Do nothing - remove verbose output
}

void runTemperatureDiagnostics() {
  // Do nothing - remove complex diagnostics
}

void testGrillSensor() {
  // Do nothing - remove test functions
}

// Remove all the complex functions - keep as empty stubs
double readAmbientTemperature() { return -999.0; }
void resetCalibration() {}
void saveCalibrationData() {}
void loadCalibrationData() {}
void printTemperatureDiagnostics() {}
void debugTemperatureLoop() {}
void testAmbientNTC() {}
void testSpecificProbe() {}
void testAmbientSensor() {}
void setTemperatureDebugMode(bool enabled) {}
bool isDebugModeEnabled() { return false; }

// Remove all other debug setters
void setAmbientDebug(bool enabled) {}
void setMeatProbesDebug(bool enabled) {}
void setRelayDebug(bool enabled) {}
void setSystemDebug(bool enabled) {}

// Remove all other debug getters  
bool getAmbientDebug() { return false; }
bool getMeatProbesDebug() { return false; }
bool getRelayDebug() { return false; }
bool getSystemDebug() { return false; }