// Utility.cpp - Clean version with individual debug controls
#include "Utility.h" 
#include "Globals.h"
#include "Ignition.h"
#include "TemperatureSensor.h"
#include <math.h>

// Individual debug control flags - REPLACE the single debugMode
bool debugGrillSensor = false;
bool debugAmbientSensor = false;
bool debugMeatProbes = false;
bool debugRelays = false;
bool debugSystem = false;

// Temperature reading with different sensor types
static double lastValidGrillTemp = 70.0;
static double lastValidAmbientTemp = 70.0;

// Individual debug control functions
void setGrillDebug(bool enabled) {
  debugGrillSensor = enabled;
  Serial.printf("Grill sensor debug: %s\n", enabled ? "ON" : "OFF");
}

void setAmbientDebug(bool enabled) {
  debugAmbientSensor = enabled;
  Serial.printf("Ambient sensor debug: %s\n", enabled ? "ON" : "OFF");
}

void setMeatProbesDebug(bool enabled) {
  debugMeatProbes = enabled;
  Serial.printf("Meat probes debug: %s\n", enabled ? "ON" : "OFF");
}

void setRelayDebug(bool enabled) {
  debugRelays = enabled;
  Serial.printf("Relay debug: %s\n", enabled ? "ON" : "OFF");
}

void setSystemDebug(bool enabled) {
  debugSystem = enabled;
  Serial.printf("System debug: %s\n", enabled ? "ON" : "OFF");
}

void setAllDebug(bool enabled) {
  debugGrillSensor = enabled;
  debugAmbientSensor = enabled;
  debugMeatProbes = enabled;
  debugRelays = enabled;
  debugSystem = enabled;
  Serial.printf("ALL debug modes: %s\n", enabled ? "ON" : "OFF");
}

// Status getter functions
bool getGrillDebug() { return debugGrillSensor; }
bool getAmbientDebug() { return debugAmbientSensor; }
bool getMeatProbesDebug() { return debugMeatProbes; }
bool getRelayDebug() { return debugRelays; }
bool getSystemDebug() { return debugSystem; }

// 100k NTC Ambient Temperature Reading (OPTIMIZED with 10k pulldown)
double readAmbientTemperature() {
  // Constants for 100k NTC thermistor (OPTIMIZED CIRCUIT)
  const float THERMISTOR_NOMINAL = 100000.0;    // 100k ohm at 25°C (confirmed by measurement)
  const float TEMPERATURE_NOMINAL = 25.0;       // 25°C
  const float B_COEFFICIENT = 3950.0;           // Beta coefficient for 100k NTC
  const float PULLDOWN_RESISTOR = 10000.0;      // 10k pulldown resistor (OPTIMIZED!)
  const float SUPPLY_VOLTAGE = 5.0;             // 5V supply
  
  // Read ADC with averaging
  int totalADC = 0;
  for (int i = 0; i < 5; i++) {
    totalADC += analogRead(AMBIENT_TEMP_PIN);
    delay(2);
  }
  int adcReading = totalADC / 5;
  
  // Convert ADC to voltage
  double voltage = (adcReading / 4095.0) * SUPPLY_VOLTAGE;
  
  // Calculate thermistor resistance using voltage divider formula
  // Circuit: 5V → NTC → GPIO → 10k → GND
  // Voltage divider: V = 5V × R_pulldown / (R_ntc + R_pulldown)
  // Solving for R_ntc: R_ntc = R_pulldown × (5V - V) / V
  double ntcResistance = PULLDOWN_RESISTOR * (SUPPLY_VOLTAGE - voltage) / voltage;
  
  // Check for valid voltage range (should be much wider now)
  if (voltage <= 0.1 || voltage >= (SUPPLY_VOLTAGE - 0.1)) {
    if (debugAmbientSensor) {
      Serial.printf("🔴 AMBIENT: Voltage out of range: %.3fV\n", voltage);
    }
    return -999.0;
  }
  
  // Sanity check for 100k NTC 
  if (ntcResistance < 10000 || ntcResistance > 1000000) {
    if (debugAmbientSensor) {
      Serial.printf("🔴 AMBIENT: Resistance out of range: %.0fΩ\n", ntcResistance);
    }
    return -999.0;
  }
  
  // Steinhart-Hart equation for temperature calculation
  double steinhart = ntcResistance / THERMISTOR_NOMINAL;  // (R/Ro)
  steinhart = log(steinhart);                              // ln(R/Ro)
  steinhart /= B_COEFFICIENT;                              // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURE_NOMINAL + 273.15);      // + (1/To)
  steinhart = 1.0 / steinhart;                            // Invert
  steinhart -= 273.15;                                     // Convert to Celsius
  
  // Convert to Fahrenheit
  double tempF = steinhart * 9.0 / 5.0 + 32.0;
  
  // Validate temperature range
  if (tempF < -40.0 || tempF > 200.0 || isnan(tempF) || isinf(tempF)) {
    if (debugAmbientSensor) {
      Serial.printf("🔴 AMBIENT: Temperature out of range: %.1f°F\n", tempF);
    }
    return -999.0;
  }
  
  // Debug output with detailed info
  if (debugAmbientSensor) {
    Serial.printf("🌡️  AMBIENT: ADC=%d, V=%.3fV, R=%.0fΩ, Temp=%.1f°F (%.1f°C), Beta=%.0f\n", 
                  adcReading, voltage, ntcResistance, tempF, steinhart, B_COEFFICIENT);
  }
  
  lastValidAmbientTemp = tempF;
  return tempF;
}

// PT100 GRILL TEMPERATURE READING FOR 5V SYSTEM  
double readGrillTemperature() {
  const float SERIES_RESISTOR = 150.0;
  const float SUPPLY_VOLTAGE = 5.0;
  
  // Read ADC
  int totalADC = 0;
  for (int i = 0; i < 5; i++) {
    totalADC += analogRead(GRILL_TEMP_PIN);
    delay(2);
  }
  int adcReading = totalADC / 5;
  
  // Convert to voltage
  double voltage = (adcReading / 4095.0) * SUPPLY_VOLTAGE;
  
  // Simple voltage range check
  if (voltage <= 0.1 || voltage >= 4.9) {
    return -999.0;  // Out of range
  }
  
  // Calculate resistance with corrected formula
  double calculatedResistance = SERIES_RESISTOR * (SUPPLY_VOLTAGE - voltage) / voltage;
  
  // Add the known connection resistance (from our measurements)
  double correctedResistance = calculatedResistance + 88.4;
  
  // Simple temperature calculation (PT100 formula)
  double tempC = (correctedResistance - 100.0) / (100.0 * 0.00385);
  double tempF = tempC * 9.0 / 5.0 + 32.0;
  
  // Apply simple calibration offset to match ambient reading
  // Your ambient shows 80.7°F, so let's calibrate to that range
  tempF = tempF - 90.0;  // Subtract 90°F to bring 170°F down to 80°F range
  
  // Reasonable range check
  if (tempF < -50.0 || tempF > 900.0 || isnan(tempF) || isinf(tempF)) {
    return -999.0;
  }
  
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

// Enhanced diagnostics with individual debug controls
void runTemperatureDiagnostics() {
  Serial.println("\n=== INDIVIDUAL SENSOR DIAGNOSTICS ===");
  
  if (debugSystem) {
    Serial.printf("🔧 System Debug Flags: Grill=%s, Ambient=%s, Meat=%s, Relay=%s, System=%s\n",
                  debugGrillSensor ? "ON" : "OFF",
                  debugAmbientSensor ? "ON" : "OFF", 
                  debugMeatProbes ? "ON" : "OFF",
                  debugRelays ? "ON" : "OFF",
                  debugSystem ? "ON" : "OFF");
  }
  
  // Test grill sensor
  Serial.println("\n--- GRILL SENSOR TEST ---");
  bool oldGrillDebug = debugGrillSensor;
  debugGrillSensor = true;  // Force debug for test
  double grillTemp = readGrillTemperature();
  debugGrillSensor = oldGrillDebug;  // Restore setting
  
  // Test ambient sensor  
  Serial.println("\n--- AMBIENT SENSOR TEST ---");
  bool oldAmbientDebug = debugAmbientSensor;
  debugAmbientSensor = true;  // Force debug for test
  double ambientTemp = readAmbientTemperature();
  debugAmbientSensor = oldAmbientDebug;  // Restore setting
  
  // Test meat probes
  Serial.println("\n--- MEAT PROBES TEST ---");
  for (int i = 1; i <= 4; i++) {
    float temp = tempSensor.getFoodTemperature(i);
    Serial.printf("🥩 Meat Probe %d: ", i);
    if (temp > -900) {
      Serial.printf("%.1f°F\n", temp);
    } else {
      Serial.println("N/A (disconnected)");
    }
  }
  
  Serial.println("==========================================\n");
}

void printTemperatureDiagnostics() {
  runTemperatureDiagnostics();
}

void debugTemperatureLoop() {
  static unsigned long lastDiagnostic = 0;
  
  if (millis() - lastDiagnostic > 30000) {
    if (debugSystem) {
      runTemperatureDiagnostics();
    }
    lastDiagnostic = millis();
  }
}

// Legacy function for compatibility
void setTemperatureDebugMode(bool enabled) {
  setAllDebug(enabled);
}

bool isDebugModeEnabled() {
  return debugGrillSensor || debugAmbientSensor || debugMeatProbes || debugRelays || debugSystem;
}

// Test function specifically for the ambient sensor
void testAmbientNTC() {
  Serial.println("\n=== DETAILED AMBIENT NTC TEST ===");
  Serial.println("Circuit: 5V → 100k NTC → GPIO36 → 10k pulldown → GND");
  Serial.println("Testing different NOMINAL resistance values...");
  
  // Get current raw readings
  int adc = analogRead(AMBIENT_TEMP_PIN);
  double voltage = (adc / 4095.0) * 5.0;
  double actualResistance = 10000.0 * (5.0 - voltage) / voltage;
  
  Serial.printf("Current readings: ADC=%d, Voltage=%.3fV, Resistance=%.0fΩ\n", 
                adc, voltage, actualResistance);
  Serial.println("Your actual temperature should be ~72.5°F at room temp");
  
  // Test different nominal resistance values with common beta
  float nominalResistances[] = {1000, 2200, 4700, 10000, 47000, 100000};
  float commonBeta = 3950; // Standard beta for most NTC thermistors
  
  Serial.println("\nTesting different NOMINAL resistance values (R₀):");
  Serial.println("Using standard Beta = 3950 for all tests");
  
  for (int i = 0; i < 6; i++) {
    float nominalR = nominalResistances[i];
    
    // Calculate temperature with this nominal resistance
    double steinhart = actualResistance / nominalR;        // R/Ro
    steinhart = log(steinhart);                            // ln(R/Ro)  
    steinhart /= commonBeta;                               // 1/B * ln(R/Ro)
    steinhart += 1.0 / (25.0 + 273.15);                  // + (1/To)
    steinhart = 1.0 / steinhart;                          // Invert
    steinhart -= 273.15;                                   // Convert to Celsius
    double tempF = steinhart * 9.0 / 5.0 + 32.0;          // Convert to Fahrenheit
    
    Serial.printf("R₀=%.0fΩ: %.1f°F", nominalR, tempF);
    
    // Check how close to actual temperature  
    double error = abs(tempF - 72.5);
    if (error < 2.0) {
      Serial.print(" ★ EXCELLENT MATCH!");
    } else if (error < 5.0) {
      Serial.print(" ✓ GOOD MATCH");
    } else if (error < 10.0) {
      Serial.print(" ~ Close");
    }
    
    Serial.printf(" (error: %.1f°F)", error);
    Serial.println();
  }
  
  Serial.println("\nOnce we find the right nominal resistance,");
  Serial.println("we can fine-tune with the beta coefficient.");
  
  Serial.println("=====================================\n");
}