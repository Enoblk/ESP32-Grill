// Utility.h - Enhanced with flexible two-point calibration system
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

// ENHANCED CALIBRATION SYSTEM
// Core calibration functions
void setupTemperatureCalibration();
void saveCalibrationData();
void loadCalibrationData();
void printCalibrationStatus();
void resetCalibration();
void handleCalibrationCommands(String command);

// Flexible calibration point management
void setCalibrationPoint1(int adc, float temp);     // Set/modify point 1
void setCalibrationPoint2(int adc, float temp);     // Set/modify point 2
void setCalibrationPoint1Current(float temp);       // Use current ADC for point 1
void setCalibrationPoint2Current(float temp);       // Use current ADC for point 2
void deleteCalibrationPoint2();                     // Remove point 2 (back to single-point)

// Calibration data access
struct CalibrationData {
  int adc1;
  float temp1;
  int adc2;
  float temp2;
  float slope;
  float offset;
  bool calibrated;
  bool point1Set;
  bool point2Set;
};

CalibrationData getCalibrationData();               // Get current calibration info
bool isCalibrationValid();                         // Check if calibration is reasonable
String getCalibrationEquation();                   // Get linear equation as string
float predictTemperature(int adc);                 // Predict temperature for given ADC

// Calibration utilities
void validateCalibration();                         // Check calibration for issues
void swapCalibrationPoints();                       // Swap point 1 and point 2 if needed
String getCalibrationStatusJSON();                  // Get status as JSON for web interface

// Temperature diagnostics
void printTemperatureDiagnostics();
void runTemperatureDiagnostics();      
void debugTemperatureLoop();           
void testAmbientNTC();                 

// Settings functions  
void save_setpoint();
void load_setpoint();

// Legacy compatibility
void setTemperatureDebugMode(bool enabled);
bool isDebugModeEnabled();

#endif