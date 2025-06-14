// Utility.h - Complete Fixed Version with Enhanced Diagnostics
#ifndef UTILITY_H
#define UTILITY_H

#include <Arduino.h>

// Temperature functions
double readTemperature();              // Main temperature (for compatibility)
double readGrillTemperature();         // PT100 grill temperature (GPIO35)
double readAmbientTemperature();       // 10K NTC ambient temperature (GPIO13)
String getStatus(double temp);

// Temperature diagnostics and calibration
void printTemperatureDiagnostics();
void runTemperatureDiagnostics();      // Complete diagnostic function
void debugTemperatureLoop();           // Call from main loop for periodic diagnostics
void setTemperatureDebugMode(bool enabled);  // Turn debug output on/off

// Settings functions  
void save_setpoint();
void load_setpoint();

// Additional function declarations that may be missing
void setTemperatureDebugMode(bool enabled);

#endif