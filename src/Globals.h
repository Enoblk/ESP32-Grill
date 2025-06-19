// Globals.h - Updated for MAX31865 RTD sensor
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
// Relay control pins - YOUR ACTUAL HARDWARE
#define RELAY_IGNITER_PIN     27  // GPIO27 - Igniter relay
#define RELAY_AUGER_PIN       26  // GPIO26 - Auger motor relay  
#define RELAY_HOPPER_FAN_PIN  25  // GPIO25 - Hopper fan relay
#define RELAY_BLOWER_FAN_PIN  5  // GPIO14 - Blower fan relay

// ===== HSPI PIN DEFINITIONS FOR MAX31865 =====
#define MAX31865_CS_PIN   4      // GPIO15 - Chip Select
#define MAX31865_CLK_PIN  14      // GPIO14 - HSPI CLK  
#define MAX31865_MOSI_PIN 13      // GPIO13 - HSPI MOSI
#define MAX31865_MISO_PIN 12      // GPIO12 - HSPI MISO

// Ambient temperature sensor pin (keep existing 10K NTC)
#define AMBIENT_TEMP_PIN 36       // GPIO36 - Ambient temperature (10K NTC + 10KΩ pullup) - 5V reference

// ADS1115 channels for meat probes (high precision 1K NTC with 10K pullups, 5V reference)
#define MEAT_PROBE_1_CHANNEL 0  // ADS1115 A0 - Meat probe 1
#define MEAT_PROBE_2_CHANNEL 1  // ADS1115 A1 - Meat probe 2  
#define MEAT_PROBE_3_CHANNEL 2  // ADS1115 A2 - Meat probe 3
#define MEAT_PROBE_4_CHANNEL 3  // ADS1115 A3 - Meat probe 4

// Button pins (UP/DOWN only - SELECT disabled)
#define BUTTON_UP_PIN     32  // GPIO32 - UP button
#define BUTTON_DOWN_PIN   33  // GPIO33 - DOWN button  

// Built-in LED for status
#define LED_BUILTIN 2

// ===== MAX31865 CONFIGURATION =====
// PT100 RTD reference resistor (usually 430Ω for PT100)
#define RREF 430.0
// Nominal resistance of PT100 at 0°C
#define RNOMINAL 100.0

// ===== SYSTEM STATE VARIABLES =====
extern bool grillRunning;
extern double setpoint;
extern AsyncWebServer server;
extern Preferences preferences;

// ===== WIFI CONFIGURATION =====
// WiFi is now managed by WiFiManager - no hardcoded credentials needed
// Default AP mode credentials:
// SSID: GrillController-[ChipID]
// Password: grillpass123
// Connect to AP and go to http://192.168.4.1 to configure WiFi

// ===== TEMPERATURE LIMITS =====
#define MIN_SETPOINT 150.0    // Minimum grill temperature (°F)
#define MAX_SETPOINT 500.0    // Maximum grill temperature (°F)
#define EMERGENCY_TEMP 650.0  // Emergency shutdown temperature (°F)

// ===== TIMING CONSTANTS =====
#define MAIN_LOOP_INTERVAL 100      // Main loop runs every 100ms
#define TEMP_UPDATE_INTERVAL 1000   // Temperature updates every 1 second
#define STATUS_PRINT_INTERVAL 10000 // Status printed every 10 seconds

// ===== VOLTAGE REFERENCE CONSTANTS =====
// For ambient sensor (still using ADC)
#define ADC_REFERENCE_VOLTAGE 5.0   // 5V reference for ambient sensor

// ===== FUNCTION DECLARATIONS =====
void save_setpoint();
void load_setpoint();

#endif