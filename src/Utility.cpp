// Utility.cpp - Complete rewrite with simple ambient temperature calculation
#include "Utility.h" 
#include "Globals.h"
#include "Ignition.h"
#include "TemperatureSensor.h"
#include <math.h>

// Temperature reading with different sensor types
static double lastValidGrillTemp = 70.0;
static double lastValidAmbientTemp = 70.0;
static bool debugMode = true;

double readAmbientTemperature() {
  int adcReading = analogRead(AMBIENT_TEMP_PIN);
  
  if (debugMode) {
    Serial.printf("*** UTILITY.CPP *** Ambient ADC: %d\n", adcReading);
  }
  
  // Simple linear mapping like your previous version
  // Map ADC range to temperature range
  double tempF = map(adcReading, 0, 4095, -40, 140);  // Adjust range as needed
  
  if (debugMode) {
    Serial.printf("Ambient calculated temp: %.1f°F\n", tempF);
  }
  
  return tempF;
}

// PT100 GRILL TEMPERATURE READING FOR 5V SYSTEM  
double readGrillTemperature() {
  const float RTD_NOMINAL = 100.0;
  const float RTD_ALPHA = 0.00385;
  const float SERIES_RESISTOR = 150.0;
  const float SUPPLY_VOLTAGE = 5.0;
  
  int totalADC = 0;
  for (int i = 0; i < 5; i++) {
    totalADC += analogRead(GRILL_TEMP_PIN);
    delay(2);
  }
  int adcReading = totalADC / 5;
  
  if (debugMode) {
    Serial.printf("Grill ADC: %d\n", adcReading);
  }
  
  double voltage = (adcReading / 4095.0) * SUPPLY_VOLTAGE;
  
  if (debugMode) {
    Serial.printf("Grill Voltage: %.3fV\n", voltage);
  }
  
  if (voltage <= 0.1 || voltage >= (SUPPLY_VOLTAGE - 0.1)) {
    if (debugMode) {
      Serial.printf("Grill voltage out of range: %.3fV\n", voltage);
    }
    return -999.0;
  }
  
  double rtdResistance = SERIES_RESISTOR * voltage / (SUPPLY_VOLTAGE - voltage);
  
  if (debugMode) {
    Serial.printf("PT100 Resistance: %.1fΩ\n", rtdResistance);
  }
  
  if (rtdResistance < 50 || rtdResistance > 500) {
    if (debugMode) {
      Serial.printf("PT100 resistance out of range: %.1fΩ\n", rtdResistance);
    }
    return -999.0;
  }
  
  double tempC = (rtdResistance - RTD_NOMINAL) / (RTD_NOMINAL * RTD_ALPHA);
  double tempF = tempC * 9.0 / 5.0 + 32.0;
  
  if (debugMode) {
    Serial.printf("Grill calculated temp: %.1f°F\n", tempF);
  }
  
  if (tempF < -50.0 || tempF > 900.0 || isnan(tempF) || isinf(tempF)) {
    if (debugMode) {
      Serial.printf("Grill temp out of range: %.1f°F\n", tempF);
    }
    return -999.0;
  }
  
  lastValidGrillTemp = tempF;
  return tempF;
}

// Main temperature function - returns grill temperature
double readTemperature() {
  return readGrillTemperature();
}

// Get system status string
String getStatus(double temp) {
  if (!grillRunning) {
    return "IDLE";
  }
  
  if (temp == -999.0) {
    return "SENSOR ERROR";
  }
  
  bool igniterOn = digitalRead(RELAY_IGNITER_PIN) == HIGH;
  if (igniterOn && temp < (setpoint - 50)) {
    return "IGNITING";
  }
  
  double error = abs(temp - setpoint);
  if (error < 10.0) {
    return "AT TEMP";
  } else if (temp < setpoint) {
    return "HEATING";
  } else {
    return "COOLING";
  }
}

// Minimal diagnostics
void runTemperatureDiagnostics() {
  Serial.println("\n=== SIMPLE TEMPERATURE DIAGNOSTICS ===");
  
  double grillTemp = readGrillTemperature();
  double ambientTemp = readAmbientTemperature();
  
  Serial.printf("Grill Temperature: %.1f°F\n", grillTemp);
  Serial.printf("Ambient Temperature: %.1f°F\n", ambientTemp);
  
  Serial.println("========================================\n");
}

void printTemperatureDiagnostics() {
  runTemperatureDiagnostics();
}

void debugTemperatureLoop() {
  static unsigned long lastDiagnostic = 0;
  
  if (millis() - lastDiagnostic > 30000) {
    runTemperatureDiagnostics();
    lastDiagnostic = millis();
  }
}

void setTemperatureDebugMode(bool enabled) {
  debugMode = enabled;
  Serial.printf("Temperature debug mode: %s\n", enabled ? "ON" : "OFF");
}