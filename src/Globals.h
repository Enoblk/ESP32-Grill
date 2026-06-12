#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>

#define RELAY_IGNITER_PIN     27
#define RELAY_AUGER_PIN       26
#define RELAY_HOPPER_FAN_PIN  25
#define RELAY_BLOWER_FAN_PIN  14
#define MAX31865_CS_PIN       5
#define SDA_PIN               21
#define SCL_PIN               22
#define AMBIENT_TEMP_PIN      36

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
