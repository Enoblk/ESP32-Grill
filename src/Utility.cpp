// Utility.cpp - Complete Fixed Version with Enhanced Diagnostics
#include "Utility.h" 
#include "Globals.h"
#include "Ignition.h"
#include "TemperatureSensor.h"
#include <math.h>

// Temperature reading with different sensor types
static double lastValidGrillTemp = 70.0;
static double lastValidAmbientTemp = 70.0;
static bool debugMode = true;  // Set to false to reduce serial output

// COMPLETE TEMPERATURE DIAGNOSTICS FUNCTION
void runTemperatureDiagnostics() {
  Serial.println("\n=====================================");
  Serial.println("TEMPERATURE SENSOR FULL DIAGNOSTICS");
  Serial.println("=====================================");
  
  // 1. GPIO35 PT100 Diagnostics
  Serial.println("\n--- GPIO35 PT100 GRILL TEMPERATURE ---");
  int grillADC = analogRead(GRILL_TEMP_PIN);
  double grillVoltage = (grillADC / 4095.0) * 5.0;  // FIXED: Now 5V reference
  
  Serial.printf("Raw ADC Reading: %d (0-4095 range)\n", grillADC);
  Serial.printf("Calculated Voltage: %.3fV (should be 0.1-4.9V)\n", grillVoltage);
  
  // Check for common PT100 issues
  if (grillADC < 100) {
    Serial.println("⚠️  ISSUE: ADC too low - Check PT100 connection/short circuit");
  } else if (grillADC > 3900) {
    Serial.println("⚠️  ISSUE: ADC too high - Check PT100 connection/open circuit");
  } else {
    Serial.println("✅ ADC reading in normal range");
  }
  
  // Calculate PT100 resistance for 5V system
  const float SERIES_R = 150.0;
  if (grillVoltage > 0.1 && grillVoltage < 4.9) {
    double rtdResistance = SERIES_R * grillVoltage / (5.0 - grillVoltage);  // FIXED: 5V reference
    Serial.printf("Calculated PT100 Resistance: %.1fΩ (should be ~100-200Ω at room temp)\n", rtdResistance);
  }
  
  double grillTemp = readGrillTemperature();
  Serial.printf("Final Temperature: %.1f°F\n", grillTemp);
  
  // 2. GPIO36 100K NTC Ambient Temperature Diagnostics (Ender 3 thermistor)
  Serial.println("\n--- GPIO36 100K NTC AMBIENT TEMPERATURE (Ender 3) ---");
  int ambientADC = analogRead(AMBIENT_TEMP_PIN);
  double ambientVoltage = (ambientADC / 4095.0) * 5.0;  // FIXED: Now 5V reference
  
  Serial.printf("Raw ADC Reading: %d (0-4095 range)\n", ambientADC);
  Serial.printf("Calculated Voltage: %.3fV (should be 1.0-4.0V)\n", ambientVoltage);
  
  // Check for common NTC issues
  if (ambientADC < 200) {
    Serial.println("⚠️  ISSUE: ADC too low - Check NTC connection/short circuit");
  } else if (ambientADC > 3800) {
    Serial.println("⚠️  ISSUE: ADC too high - Check NTC connection/open circuit");
  } else {
    Serial.println("✅ ADC reading in normal range");
  }
  
  // Calculate NTC resistance for 5V system with 100K thermistor
  const float SERIES_R_NTC = 10000.0;
  if (ambientVoltage > 0.01 && ambientVoltage < 4.99) {
    double ntcResistance = SERIES_R_NTC * ambientVoltage / (5.0 - ambientVoltage);  // FIXED: 5V reference
    Serial.printf("Calculated 100K NTC Resistance: %.0fΩ (should be ~50-100kΩ at room temp)\n", ntcResistance);
  } else {
    Serial.println("⚠️  Cannot calculate NTC resistance - voltage out of range");
  }
  
  double ambientTemp = readAmbientTemperature();
  Serial.printf("Final Temperature: %.1f°F\n", ambientTemp);
  
  if (ambientTemp == 70.0 || ambientTemp == -999.0) {
    Serial.println("⚠️  ISSUE: Reading fallback value - sensor not working");
  }
  
  Serial.println("\n=====================================");
  Serial.println("DIAGNOSTICS COMPLETE");
  Serial.println("=====================================\n");
}

// FIXED AMBIENT TEMPERATURE READING - Ender 3 100K NTC Thermistor
double readAmbientTemperature() {
  // Empirical constants based on your actual readings
  const float THERMISTOR_NOMINAL = 18400.0;   // Your room temp resistance
  const float TEMPERATURE_NOMINAL = 22.0;     // 22°C (72°F)  
  const float B_COEFFICIENT = 3950.0;         // Standard NTC beta (removed negative)
  const float SERIES_RESISTOR = 10000.0;      // 10k pullup resistor
  const float SUPPLY_VOLTAGE = 5.0;           // 5V supply voltage
  
  // Read ADC multiple times for stability
  int totalADC = 0;
  for (int i = 0; i < 5; i++) {
    totalADC += analogRead(AMBIENT_TEMP_PIN);
    delay(2);
  }
  int adcReading = totalADC / 5;
  
  if (debugMode) {
    Serial.printf("Ambient ADC: %d\n", adcReading);
  }
  
  double voltage = (adcReading / 4095.0) * SUPPLY_VOLTAGE;
  
  if (debugMode) {
    Serial.printf("Ambient Voltage: %.3fV\n", voltage);
  }
  
  // Voltage validation for 5V system
  if (voltage <= 0.05 || voltage >= (SUPPLY_VOLTAGE - 0.05)) {
    if (debugMode) {
      Serial.printf("Ambient voltage out of range: %.3fV\n", voltage);
    }
    return -999.0;
  }
  
  // Calculate thermistor resistance using INVERTED formula (thermistor is on top of divider)
  // Circuit: 5V → thermistor → GPIO36 → 10kΩ pulldown → GND
  double thermistorResistance = SERIES_RESISTOR * (SUPPLY_VOLTAGE - voltage) / voltage;
  
  if (debugMode) {
    Serial.printf("Ambient NTC Resistance: %.0fΩ\n", thermistorResistance);
  }
  
  // Sanity check for 100K NTC (should be 10k-500k range)
  if (thermistorResistance < 5000 || thermistorResistance > 1000000) {
    if (debugMode) {
      Serial.printf("Ambient resistance out of range: %.0fΩ\n", thermistorResistance);
    }
    return -999.0;
  }
  
  // Steinhart-Hart equation for 100K NTC thermistor
  double steinhart;
  steinhart = thermistorResistance / THERMISTOR_NOMINAL;
  steinhart = log(steinhart);
  steinhart /= B_COEFFICIENT;
  steinhart += 1.0 / (TEMPERATURE_NOMINAL + 273.15);
  steinhart = 1.0 / steinhart;
  steinhart -= 273.15;  // Convert to Celsius
  
  double tempF = steinhart * 9.0 / 5.0 + 32.0;
  
  if (debugMode) {
    Serial.printf("Ambient calculated temp: %.1f°F\n", tempF);
  }
  
  // Temperature validation
  if (tempF < -40.0 || tempF > 150.0 || isnan(tempF) || isinf(tempF)) {
    if (debugMode) {
      Serial.printf("Ambient temp out of range: %.1f°F\n", tempF);
    }
    return -999.0;
  }
  
  lastValidAmbientTemp = tempF;
  return tempF;
}

// FIXED PT100 GRILL TEMPERATURE READING FOR 5V SYSTEM  
double readGrillTemperature() {
  // PT100 constants
  const float RTD_NOMINAL = 100.0;         // 100 ohm at 0°C (PT100)
  const float RTD_ALPHA = 0.00385;         // Temperature coefficient
  const float SERIES_RESISTOR = 150.0;     // 150Ω series resistor
  const float SUPPLY_VOLTAGE = 5.0;        // FIXED: Now 5V supply voltage
  
  // Read ADC multiple times for stability
  int totalADC = 0;
  for (int i = 0; i < 5; i++) {
    totalADC += analogRead(GRILL_TEMP_PIN);
    delay(2);
  }
  int adcReading = totalADC / 5;
  
  if (debugMode) {
    Serial.printf("Grill ADC: %d\n", adcReading);
  }
  
  double voltage = (adcReading / 4095.0) * SUPPLY_VOLTAGE;  // FIXED: 5V reference
  
  if (debugMode) {
    Serial.printf("Grill Voltage: %.3fV\n", voltage);
  }
  
  // Strict voltage validation for PT100 with 5V
  if (voltage <= 0.1 || voltage >= (SUPPLY_VOLTAGE - 0.1)) {
    if (debugMode) {
      Serial.printf("Grill voltage out of range: %.3fV\n", voltage);
    }
    return -999.0;
  }
  
  // FIXED: Calculate RTD resistance using voltage divider for 5V
  // Circuit: 5V → PT100 → GPIO35 → 150Ω → GND
  double rtdResistance = SERIES_RESISTOR * voltage / (SUPPLY_VOLTAGE - voltage);
  
  if (debugMode) {
    Serial.printf("PT100 Resistance: %.1fΩ\n", rtdResistance);
  }
  
  // Sanity check - PT100 should be ~100-200Ω at room temp
  if (rtdResistance < 50 || rtdResistance > 500) {
    if (debugMode) {
      Serial.printf("PT100 resistance out of range: %.1fΩ\n", rtdResistance);
    }
    return -999.0;
  }
  
  // RTD temperature calculation (simplified Callendar-Van Dusen)
  double tempC = (rtdResistance - RTD_NOMINAL) / (RTD_NOMINAL * RTD_ALPHA);
  double tempF = tempC * 9.0 / 5.0 + 32.0;
  
  if (debugMode) {
    Serial.printf("Grill calculated temp: %.1f°F\n", tempF);
  }
  
  // PT100 valid range
  if (tempF < -50.0 || tempF > 900.0 || isnan(tempF) || isinf(tempF)) {
    if (debugMode) {
      Serial.printf("Grill temp out of range: %.1f°F\n", tempF);
    }
    return -999.0;
  }
  
  lastValidGrillTemp = tempF;
  return tempF;
}

// Main temperature function (for compatibility) - returns grill temperature
double readTemperature() {
  return readGrillTemperature();
}

// Get system status string
String getStatus(double temp) {
  if (!grillRunning) {
    return "IDLE";
  }
  
  // Check if we got an error reading
  if (temp == -999.0) {
    return "SENSOR ERROR";
  }
  
  // Check ignition status by checking if igniter relay is on
  bool igniterOn = digitalRead(RELAY_IGNITER_PIN) == HIGH;
  if (igniterOn && temp < (setpoint - 50)) {
    return "IGNITING";
  }
  
  // Check temperature vs setpoint
  double error = abs(temp - setpoint);
  if (error < 10.0) {
    return "AT TEMP";
  } else if (temp < setpoint) {
    return "HEATING";
  } else {
    return "COOLING";
  }
}

// Enhanced temperature diagnostics
void printTemperatureDiagnostics() {
  Serial.println("\n=== TEMPERATURE SENSOR DIAGNOSTICS ===");
  Serial.println("Configuration:");
  Serial.printf("  Grill Temp: GPIO%d (ESP32 ADC + PT100) - 5V Reference\n", GRILL_TEMP_PIN);
  Serial.printf("  Ambient Temp: GPIO%d (ESP32 ADC + 100K NTC Ender 3) - 5V Reference\n", AMBIENT_TEMP_PIN);
  Serial.println("  Meat Probes: ADS1115 A0-A3 (High precision + 1K NTC) - 5V Reference");
  
  Serial.println("\nCurrent Readings:");
  double grillTemp = readGrillTemperature();
  double ambientTemp = readAmbientTemperature();
  
  Serial.printf("  Grill Temperature (PT100): %.1f°F\n", grillTemp);
  Serial.printf("  Ambient Temperature (100K NTC): %.1f°F\n", ambientTemp);
  
  // ADS1115 meat probe readings (if available)
  Serial.printf("  Meat Probe 1: %.1f°F\n", tempSensor.getFoodTemperature(1));
  Serial.printf("  Meat Probe 2: %.1f°F\n", tempSensor.getFoodTemperature(2));
  Serial.printf("  Meat Probe 3: %.1f°F\n", tempSensor.getFoodTemperature(3));
  Serial.printf("  Meat Probe 4: %.1f°F\n", tempSensor.getFoodTemperature(4));
  
  Serial.println("==========================================\n");
}

// Call this from your main loop to run diagnostics every 30 seconds
void debugTemperatureLoop() {
  static unsigned long lastDiagnostic = 0;
  
  if (millis() - lastDiagnostic > 30000) {  // Every 30 seconds
    runTemperatureDiagnostics();
    lastDiagnostic = millis();
  }
}

// Turn debug mode on/off
void setTemperatureDebugMode(bool enabled) {
  debugMode = enabled;
  Serial.printf("Temperature debug mode: %s\n", enabled ? "ON" : "OFF");
}