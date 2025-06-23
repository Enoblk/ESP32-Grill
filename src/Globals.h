// Globals.h - Updated with new function declarations for debugging and pellet control
#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

// Forward declare the MAX31865 sensor
class MAX31865Sensor;
extern MAX31865Sensor grillSensor;

// ===== I2C PIN DEFINITIONS =====
#define SDA_PIN 21
#define SCL_PIN 22

// ===== PIN DEFINITIONS =====
// Relay control pins
#define RELAY_IGNITER_PIN     27  // GPIO27 - Igniter relay
#define RELAY_AUGER_PIN       26  // GPIO26 - Auger motor relay  
#define RELAY_HOPPER_FAN_PIN  25  // GPIO25 - Hopper fan relay
#define RELAY_BLOWER_FAN_PIN  14  // GPIO14 - Blower fan relay

// ===== MAX31865 PIN DEFINITIONS =====
#define MAX31865_CS_PIN   5     // GPIO5 - Chip Select
// SPI uses default pins: SCK=18, MISO=19, MOSI=23

// Other sensor pins
#define AMBIENT_TEMP_PIN 36       // GPIO36 - Ambient temperature

// ADS1115 channels for meat probes
#define MEAT_PROBE_1_CHANNEL 0
#define MEAT_PROBE_2_CHANNEL 1
#define MEAT_PROBE_3_CHANNEL 2
#define MEAT_PROBE_4_CHANNEL 3

// Button pins
#define BUTTON_UP_PIN     32  // GPIO32 - UP button
#define BUTTON_DOWN_PIN   33  // GPIO33 - DOWN button  
#define BUTTON_SELECT_PIN 39  // GPIO39 - Select button
// Built-in LED
#define LED_BUILTIN 2

// ===== MAX31865 CONFIGURATION =====
#define RREF 430.0        // Reference resistor on MAX31865 board
#define RNOMINAL 100.0    // Your 100Ω test resistor

// ===== SYSTEM STATE VARIABLES =====
extern bool grillRunning;
extern double setpoint;
extern AsyncWebServer server;
extern Preferences preferences;

// ===== TEMPERATURE LIMITS =====
#define MIN_SETPOINT 150.0
#define MAX_SETPOINT 500.0
#define EMERGENCY_TEMP 650.0

// ===== TIMING CONSTANTS =====
#define MAIN_LOOP_INTERVAL 100
#define TEMP_UPDATE_INTERVAL 1000
#define STATUS_PRINT_INTERVAL 10000

// ===== VOLTAGE REFERENCE CONSTANTS =====
#define ADC_REFERENCE_VOLTAGE 5.0

// ===== FUNCTION DECLARATIONS =====
void save_setpoint();
void load_setpoint();

// New debugging and pellet control function declarations
extern bool relay_get_manual_override_status();
extern unsigned long relay_get_manual_override_remaining();
extern void relay_force_clear_manual();

// Pellet control parameter functions
extern unsigned long pellet_get_initial_feed_duration();
extern unsigned long pellet_get_lighting_feed_duration();
extern unsigned long pellet_get_normal_feed_duration();
extern unsigned long pellet_get_lighting_feed_interval();

extern void pellet_set_initial_feed_duration(unsigned long duration);
extern void pellet_set_lighting_feed_duration(unsigned long duration);
extern void pellet_set_normal_feed_duration(unsigned long duration);
extern void pellet_set_lighting_feed_interval(unsigned long interval);

extern void savePelletParameters();
extern void loadPelletParameters();

#endif // GLOBALS_H