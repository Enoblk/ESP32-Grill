// Utility.cpp - Minimal version that just reads 100Ω resistor temperature
#include "Utility.h" 
#include "Globals.h"
#include "MAX31865Sensor.h"

// Simple debug flags
bool debugGrillSensor = false;
bool debugAmbientSensor = false;
bool debugMeatProbes = false;
bool debugRelays = false;
bool debugSystem = false;

// Temperature validation
bool isValidTemperature(double temp) {
  if (isnan(temp) || isinf(temp)) return false;
  if (temp <= -900.0 || temp >= 999.0) return false;
  return true;
}

// Debug control functions
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

// SIMPLE GRILL TEMPERATURE READING from 100Ω resistor
double readGrillTemperature() {
  static unsigned long lastReading = 0;
  static double cachedTemp = 70.0;
  
  // Only read every 500ms
  if (millis() - lastReading < 500) {
    return cachedTemp;
  }
  
  // Read from MAX31865
  double temp = grillSensor.readTemperatureF();
  
  if (debugGrillSensor) {
    float resistance = grillSensor.readRTD();
    Serial.printf("🔥 GRILL: %.1f°F (R: %.1fΩ)\n", temp, resistance);
  }
  
  if (isValidTemperature(temp)) {
    cachedTemp = temp;
    lastReading = millis();
    return temp;
  }
  
  // Return cached value if current reading is bad
  return cachedTemp;
}

// Ambient temperature (unchanged)
double readAmbientTemperature() {
  const float THERMISTOR_NOMINAL = 100000.0;    
  const float TEMPERATURE_NOMINAL = 25.0;       
  const float B_COEFFICIENT = 3950.0;           
  const float PULLDOWN_RESISTOR = 10000.0;      
  const float SUPPLY_VOLTAGE = 5.0;             
  
  int totalADC = 0;
  for (int i = 0; i < 5; i++) {
    totalADC += analogRead(AMBIENT_TEMP_PIN);
    delay(2);
  }
  int adcReading = totalADC / 5;
  
  double voltage = (adcReading / 4095.0) * SUPPLY_VOLTAGE;
  double ntcResistance = PULLDOWN_RESISTOR * (SUPPLY_VOLTAGE - voltage) / voltage;
  
  if (voltage <= 0.1 || voltage >= (SUPPLY_VOLTAGE - 0.1)) {
    return -999.0;
  }
  
  if (ntcResistance < 10000 || ntcResistance > 1000000) {
    return -999.0;
  }
  
  double steinhart = ntcResistance / THERMISTOR_NOMINAL;  
  steinhart = log(steinhart);                              
  steinhart /= B_COEFFICIENT;                              
  steinhart += 1.0 / (TEMPERATURE_NOMINAL + 273.15);      
  steinhart = 1.0 / steinhart;                            
  steinhart -= 273.15;                                     
  
  double tempF = steinhart * 9.0 / 5.0 + 32.0;
  
  if (!isValidTemperature(tempF) || tempF < -40.0 || tempF > 200.0) {
    return -999.0;
  }
  
  return tempF;
}

// Main temperature function
double readTemperature() {
  return readGrillTemperature();
}

// STATUS FUNCTION
String getStatus(double temp) {
  if (!grillRunning) {
    return "IDLE";
  }
  
  if (!isValidTemperature(temp)) {
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

// Simple calibration system
void setupTemperatureCalibration() {
  Serial.println("Simple temperature system ready (100Ω resistor via MAX31865)");
  
  if (grillSensor.isInitialized()) {
    Serial.println("✅ MAX31865 ready for temperature reading");
  } else {
    Serial.println("⚠️  MAX31865 not initialized");
  }
}

void handleCalibrationCommands(String command) {
  if (command == "max_help" || command == "cal_help") {
    Serial.println("\n=== SIMPLE TEMPERATURE COMMANDS ===");
    Serial.println("test_temp    - Test temperature reading");
    Serial.println("debug_on/off - Toggle debug output");
    Serial.println("====================================\n");
    
  } else if (command == "test_temp") {
    Serial.println("Testing temperature reading...");
    float temp = grillSensor.readTemperatureF();
    float resistance = grillSensor.readRTD();
    Serial.printf("Temperature: %.1f°F\n", temp);
    Serial.printf("Resistance: %.1fΩ\n", resistance);
    
  } else if (command == "debug_on") {
    setGrillDebug(true);
  } else if (command == "debug_off") {
    setGrillDebug(false);
  }
}

void printCalibrationStatus() {
  Serial.println("\n=== SIMPLE TEMPERATURE STATUS ===");
  
  if (grillSensor.isInitialized()) {
    Serial.println("✅ MAX31865 initialized");
    float temp = grillSensor.readTemperatureF();
    float resistance = grillSensor.readRTD();
    Serial.printf("Current: %.1f°F (%.1fΩ)\n", temp, resistance);
  } else {
    Serial.println("❌ MAX31865 not working");
  }
  
  Serial.println("==================================\n");
}

// Simple diagnostic functions
void runTemperatureDiagnostics() {
  Serial.println("\n=== TEMPERATURE DIAGNOSTICS ===");
  
  double grillTemp = readGrillTemperature();
  Serial.printf("🔥 Grill: %.1f°F - %s\n", 
                grillTemp, isValidTemperature(grillTemp) ? "VALID" : "INVALID");
  
  double ambientTemp = readAmbientTemperature();
  Serial.printf("🌡️ Ambient: %.1f°F - %s\n", 
                ambientTemp, isValidTemperature(ambientTemp) ? "VALID" : "INVALID");
  
  Serial.println("===============================\n");
}

void testGrillSensor() {
  Serial.println("\n=== TESTING 100Ω RESISTOR ===");
  
  if (!grillSensor.isInitialized()) {
    Serial.println("❌ MAX31865 not initialized");
    return;
  }
  
  for (int i = 0; i < 5; i++) {
    float temp = grillSensor.readTemperatureF();
    float resistance = grillSensor.readRTD();
    Serial.printf("Reading %d: %.1f°F (%.1fΩ)\n", i+1, temp, resistance);
    delay(500);
  }
  
  Serial.println("==============================\n");
}

// Compatibility functions
void resetCalibration() {
  Serial.println("No calibration needed for simple system");
}

void saveCalibrationData() {}
void loadCalibrationData() {}
void printTemperatureDiagnostics() { runTemperatureDiagnostics(); }
void debugTemperatureLoop() {}
void testAmbientNTC() {}
void testSpecificProbe() {}
void testAmbientSensor() {}
void setTemperatureDebugMode(bool enabled) { setAllDebug(enabled); }
bool isDebugModeEnabled() { return debugGrillSensor; }