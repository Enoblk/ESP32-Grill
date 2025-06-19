// Utility.cpp - Updated for MAX31865 RTD sensor
#include "Utility.h" 
#include "Globals.h"
#include "Ignition.h"
#include "TemperatureSensor.h"
#include "MAX31865Sensor.h"  // Add MAX31865 support
#include <math.h>

// Individual debug control flags
bool debugGrillSensor = false;
bool debugAmbientSensor = false;
bool debugMeatProbes = false;
bool debugRelays = false;
bool debugSystem = false;

// Temperature reading with different sensor types
static double lastValidGrillTemp = 70.0;
static double lastValidAmbientTemp = 70.0;

// UNIFIED TEMPERATURE VALIDATION FUNCTION
bool isValidTemperature(double temp) {
  if (isnan(temp) || isinf(temp)) return false;
  if (temp <= -900.0 || temp >= 999.0) return false;
  return true;
}

// Individual debug control functions
void setGrillDebug(bool enabled) {
  debugGrillSensor = enabled;
  grillSensor.setDebug(enabled);  // Also set MAX31865 debug
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
  grillSensor.setDebug(enabled);  // Also set MAX31865 debug
  Serial.printf("ALL debug modes: %s\n", enabled ? "ON" : "OFF");
}

// Status getter functions
bool getGrillDebug() { return debugGrillSensor; }
bool getAmbientDebug() { return debugAmbientSensor; }
bool getMeatProbesDebug() { return debugMeatProbes; }
bool getRelayDebug() { return debugRelays; }
bool getSystemDebug() { return debugSystem; }

// UPDATED GRILL TEMPERATURE READING - Now uses MAX31865
double readGrillTemperature() {
  // Cache readings to prevent multiple calls per loop
  static unsigned long lastReading = 0;
  static double cachedTemp = 70.0;
  
  // Only read every 250ms to prevent overwhelming the sensor
  if (millis() - lastReading < 250) {
    return cachedTemp;
  }
  
  // Read from MAX31865
  double temp = grillSensor.readTemperatureF();
  
  // Validate reading
  if (!isValidTemperature(temp)) {
    if (debugGrillSensor) {
      Serial.printf("🔴 GRILL MAX31865: Invalid reading %.1f°F\n", temp);
      
      // Show fault information if available
      if (grillSensor.hasFault()) {
        Serial.printf("🔴 GRILL MAX31865: Fault - %s\n", grillSensor.getFaultString().c_str());
      }
    }
    
    // Return last valid reading if recent (within 30 seconds)
    if (millis() - lastReading < 30000 && lastValidGrillTemp > 0) {
      cachedTemp = lastValidGrillTemp;
      return lastValidGrillTemp;
    }
    
    return -999.0;  // Sensor error
  }
  
  // Good reading - update cache
  lastValidGrillTemp = temp;
  cachedTemp = temp;
  lastReading = millis();
  
  if (debugGrillSensor) {
    Serial.printf("🔥 GRILL MAX31865: %.1f°F (resistance: %.1fΩ)\n", 
                  temp, grillSensor.readRTD());
  }
  
  return temp;
}

// 100k NTC Ambient Temperature Reading (unchanged - still uses ADC)
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
    if (debugAmbientSensor) {
      Serial.printf("🔴 AMBIENT: Voltage out of range: %.3fV\n", voltage);
    }
    return -999.0;
  }
  
  if (ntcResistance < 10000 || ntcResistance > 1000000) {
    if (debugAmbientSensor) {
      Serial.printf("🔴 AMBIENT: Resistance out of range: %.0fΩ\n", ntcResistance);
    }
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
    if (debugAmbientSensor) {
      Serial.printf("🔴 AMBIENT: Temperature out of range: %.1f°F\n", tempF);
    }
    return -999.0;
  }
  
  if (debugAmbientSensor) {
    Serial.printf("🌡️  AMBIENT: ADC=%d, V=%.3fV, R=%.0fΩ, Temp=%.1f°F\n", 
                  adcReading, voltage, ntcResistance, tempF);
  }
  
  lastValidAmbientTemp = tempF;
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

// =============================================================================
// MAX31865 CALIBRATION SYSTEM (Replaces old voltage divider calibration)
// =============================================================================

// MAX31865 calibration functions
void setupTemperatureCalibration() {
  Serial.println("Initializing MAX31865 RTD calibration system...");
  
  // Initialize the MAX31865 sensor
  if (!grillSensor.begin(MAX31865_CS_PIN, RREF, RNOMINAL, false)) {  // 4-wire PT100
    Serial.println("❌ Failed to initialize MAX31865 sensor");
    return;
  }
  
  Serial.println("✅ MAX31865 RTD sensor initialized successfully");
  Serial.println("Type 'max_help' for MAX31865 commands");
}

void handleCalibrationCommands(String command) {
  if (command == "max_help") {
    Serial.println("\n=== MAX31865 RTD SENSOR COMMANDS ===");
    Serial.println("max_status      - Show MAX31865 status and diagnostics");
    Serial.println("max_test        - Run connection and reading test");
    Serial.println("max_diag        - Show detailed diagnostics");
    Serial.println("max_fault       - Check for faults");
    Serial.println("max_clear       - Clear any faults");
    Serial.println("max_cal_offset <temp> - Calibrate offset to known temperature");
    Serial.println("max_cal_reset   - Reset calibration to defaults");
    Serial.println("max_debug_on/off - Toggle debug output");
    Serial.println("max_raw         - Show raw RTD values");
    Serial.println("max_resistance  - Show current resistance reading");
    Serial.println("=============================\n");
    
  } else if (command == "max_status") {
    grillSensor.printDiagnostics();
    
  } else if (command == "max_test") {
    grillSensor.runDiagnosticTest();
    
  } else if (command == "max_diag") {
    grillSensor.printDiagnostics();
    
  } else if (command == "max_fault") {
    if (grillSensor.hasFault()) {
      Serial.printf("❌ Fault detected: %s\n", grillSensor.getFaultString().c_str());
    } else {
      Serial.println("✅ No faults detected");
    }
    
  } else if (command == "max_clear") {
    grillSensor.clearFault();
    Serial.println("Faults cleared");
    
  } else if (command.startsWith("max_cal_offset ")) {
    float temp = command.substring(15).toFloat();
    if (temp >= -50.0 && temp <= 800.0) {
      grillSensor.calibrateOffset(temp);
      Serial.printf("Calibration offset set for %.1f°F\n", temp);
    } else {
      Serial.println("❌ Invalid temperature range (-50 to 800°F)");
    }
    
  } else if (command == "max_cal_reset") {
    grillSensor.setCalibration(0.0, 1.0);
    Serial.println("MAX31865 calibration reset to defaults");
    
  } else if (command == "max_debug_on") {
    grillSensor.setDebug(true);
    debugGrillSensor = true;
    
  } else if (command == "max_debug_off") {
    grillSensor.setDebug(false);
    debugGrillSensor = false;
    
  } else if (command == "max_raw") {
    uint16_t raw = grillSensor.readRTDRaw();
    Serial.printf("Raw RTD value: 0x%04X (%d)\n", raw, raw);
    
  } else if (command == "max_resistance") {
    float resistance = grillSensor.readRTD();
    Serial.printf("RTD resistance: %.2fΩ\n", resistance);
    
    // Show expected resistance for common temperatures
    Serial.println("Expected resistances:");
    Serial.println("  32°F (0°C):   100.0Ω");
    Serial.println("  70°F (21°C):  108.3Ω");
    Serial.println("  212°F (100°C): 138.5Ω");
    Serial.println("  300°F (149°C): 157.3Ω");
    Serial.println("  400°F (204°C): 175.9Ω");
    
  } else if (command.startsWith("cal_")) {
    // Legacy calibration commands - inform user about MAX31865
    Serial.println("⚠️  Old calibration system no longer available.");
    Serial.println("The MAX31865 provides accurate readings without manual calibration.");
    Serial.println("Use 'max_help' for MAX31865-specific commands.");
    Serial.println("If needed, use 'max_cal_offset <temp>' for fine-tuning.");
  }
}

// Legacy compatibility functions
void resetCalibration() {
  grillSensor.setCalibration(0.0, 1.0);
  Serial.println("MAX31865 calibration reset");
}

void printCalibrationStatus() {
  Serial.println("\n=== MAX31865 RTD SENSOR STATUS ===");
  
  if (!grillSensor.isInitialized()) {
    Serial.println("❌ MAX31865 not initialized");
    Serial.println("Check SPI wiring and power connections");
    return;
  }
  
  if (grillSensor.isConnected()) {
    Serial.println("✅ MAX31865 connected and operational");
  } else {
    Serial.println("❌ MAX31865 connection issue");
  }
  
  // Current reading
  float temp = grillSensor.readTemperatureF();
  float resistance = grillSensor.readRTD();
  
  Serial.printf("Current Temperature: %.1f°F\n", temp);
  Serial.printf("Current Resistance: %.2fΩ\n", resistance);
  
  // Fault status
  if (grillSensor.hasFault()) {
    Serial.printf("❌ Fault: %s\n", grillSensor.getFaultString().c_str());
  } else {
    Serial.println("✅ No faults");
  }
  
  Serial.printf("Error Count: %lu\n", grillSensor.getErrorCount());
  Serial.printf("Last Error: %s\n", grillSensor.getLastError().c_str());
  
  Serial.println("====================================\n");
}

void saveCalibrationData() {
  Serial.println("MAX31865 calibration is stored in sensor - no manual save needed");
}

void loadCalibrationData() {
  Serial.println("MAX31865 calibration loaded from sensor");
}

// =============================================================================
// DIAGNOSTIC FUNCTIONS (Updated for MAX31865)
// =============================================================================

void runTemperatureDiagnostics() {
  Serial.println("\n=== TEMPERATURE SENSOR DIAGNOSTICS ===");
  
  // Test MAX31865 grill sensor
  Serial.println("\n--- MAX31865 GRILL SENSOR TEST ---");
  bool oldGrillDebug = debugGrillSensor;
  debugGrillSensor = true;
  grillSensor.setDebug(true);
  
  double grillTemp = readGrillTemperature();
  
  debugGrillSensor = oldGrillDebug;
  grillSensor.setDebug(oldGrillDebug);
  
  Serial.printf("🔥 Grill temperature: %.1f°F - %s\n", 
                grillTemp, isValidTemperature(grillTemp) ? "VALID" : "INVALID");
  
  if (grillSensor.hasFault()) {
    Serial.printf("❌ MAX31865 Fault: %s\n", grillSensor.getFaultString().c_str());
  }
  
  // Test ambient sensor  
  Serial.println("\n--- AMBIENT SENSOR TEST ---");
  bool oldAmbientDebug = debugAmbientSensor;
  debugAmbientSensor = true;  
  double ambientTemp = readAmbientTemperature();
  debugAmbientSensor = oldAmbientDebug;  
  
  Serial.printf("🌡️ Ambient temperature: %.1f°F - %s\n", 
                ambientTemp, isValidTemperature(ambientTemp) ? "VALID" : "INVALID");
  
  // Test meat probes
  Serial.println("\n--- MEAT PROBES TEST ---");
  for (int i = 1; i <= 4; i++) {
    float temp = tempSensor.getFoodTemperature(i);
    Serial.printf("🥩 Meat Probe %d: ", i);
    if (isValidTemperature(temp)) {
      Serial.printf("%.1f°F - VALID\n", temp);
    } else {
      Serial.println("N/A (disconnected)");
    }
  }
  
  // Status test
  Serial.println("\n--- STATUS TEST ---");
  String status = getStatus(grillTemp);
  Serial.printf("🔍 Current status: %s\n", status.c_str());
  
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

// Legacy functions for compatibility
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
  
  // Get current raw readings
  int adc = analogRead(AMBIENT_TEMP_PIN);
  double voltage = (adc / 4095.0) * 5.0;
  double actualResistance = 10000.0 * (5.0 - voltage) / voltage;
  
  Serial.printf("Current readings: ADC=%d, Voltage=%.3fV, Resistance=%.0fΩ\n", 
                adc, voltage, actualResistance);
  
  Serial.println("=====================================\n");
}

// Test function for MAX31865
void testGrillSensor() {
  Serial.println("\n=== TESTING MAX31865 GRILL SENSOR ===");
  
  if (!grillSensor.isInitialized()) {
    Serial.println("❌ MAX31865 not initialized");
    return;
  }
  
  grillSensor.runDiagnosticTest();
}