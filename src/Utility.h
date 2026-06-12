#ifndef UTILITY_H
#define UTILITY_H

#include <Arduino.h>

extern bool debugGrillSensor;
extern bool debugAmbientSensor;
extern bool debugMeatProbes;
extern bool debugRelays;
extern bool debugSystem;

double readTemperature();
double readGrillTemperature();
double readAmbientTemperature();
String getStatus(double temp);
bool isValidTemperature(double temp);
bool grillTemperatureHealthy();
unsigned long grillTemperatureAgeMs();

void setGrillDebug(bool enabled);
void setAmbientDebug(bool enabled);
void setMeatProbesDebug(bool enabled);
void setRelayDebug(bool enabled);
void setSystemDebug(bool enabled);
void setAllDebug(bool enabled);

bool getGrillDebug();
bool getAmbientDebug();
bool getMeatProbesDebug();
bool getRelayDebug();
bool getSystemDebug();

void setupTemperatureCalibration();
void printCalibrationStatus();
void handleCalibrationCommands(String command);
void runTemperatureDiagnostics();
void testGrillSensor();
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
