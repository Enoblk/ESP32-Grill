#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>

#include "BoardConfig.h"

#define RREF 430.0
#define RNOMINAL 100.0
#define MIN_SETPOINT 150.0
#define MAX_SETPOINT 500.0
#define EMERGENCY_TEMP 650.0

extern bool grillRunning;
extern double setpoint;
extern WebServer server;
extern Preferences preferences;

void save_setpoint();
void load_setpoint();
void clamp_setpoint();

#endif
