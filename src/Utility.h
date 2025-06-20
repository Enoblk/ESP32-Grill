// Utility.h - Minimal header for simple temperature reading
#ifndef UTILITY_H
#define UTILITY_H

#include <Arduino.h>

// Debug control flags
extern bool debugGrillSensor;
extern bool debugAmbientSensor; 
extern bool debugMeatProbes;
extern bool debugRelays;
extern bool debugSystem;

// Temperature functions
double readTemperature();              
double readGrillTemperature();         
double readAmbientTemperature();       
String getStatus(double temp);
bool isValidTemperature(double temp);



void testSpecificProbe();
void testAmbientSensor();


// Debug control functions
void setGrillDebug(bool enabled);
void setAmbientDebug(bool enabled);
void setMeatProbesDebug(bool enabled);
void setRelayDebug(bool enabled);
void setSystemDebug(bool enabled);
void setAllDebug(bool enabled);

// Status functions
bool getGrillDebug();
bool getAmbientDebug();
bool getMeatProbesDebug();
bool getRelayDebug();
bool getSystemDebug();

// Simple calibration system
void setupTemperatureCalibration();
void printCalibrationStatus();
void handleCalibrationCommands(String command);

// Diagnostic functions
void runTemperatureDiagnostics();
void testGrillSensor();

// Compatibility functions
void resetCalibration();
void saveCalibrationData();
void loadCalibrationData();
void printTemperatureDiagnostics();
void debugTemperatureLoop();
void testAmbientNTC();
void testSpecificProbe();
void testAmbientSensor();
void setTemperatureDebugMode(bool enabled);
bool isDebugModeEnabled();

#endif