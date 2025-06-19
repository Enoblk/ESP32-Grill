// Utility.h - Updated with missing function declaration
#ifndef UTILITY_H
#define UTILITY_H

#include <Arduino.h>

// Individual debug control flags
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

// UNIFIED TEMPERATURE VALIDATION FUNCTION
bool isValidTemperature(double temp);

// Individual debug control functions
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

// MAX31865 CALIBRATION SYSTEM
void setupTemperatureCalibration();
void saveCalibrationData();
void loadCalibrationData();
void printCalibrationStatus();
void resetCalibration();
void handleCalibrationCommands(String command);

// Temperature diagnostics
void printTemperatureDiagnostics();
void runTemperatureDiagnostics();      
void debugTemperatureLoop();           
void testAmbientNTC();
void testGrillSensor();  // ADD THIS MISSING FUNCTION DECLARATION

// Settings functions  
void save_setpoint();
void load_setpoint();

// Legacy compatibility
void setTemperatureDebugMode(bool enabled);
bool isDebugModeEnabled();

#endif