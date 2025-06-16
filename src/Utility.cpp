// Utility.cpp - Enhanced with flexible two-point calibration system
#include "Utility.h" 
#include "Globals.h"
#include "Ignition.h"
#include "TemperatureSensor.h"
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

// Enhanced calibration data structure
struct TempCalibration {
  int adc1;          // ADC reading at point 1
  float temp1;       // Known temperature at point 1
  int adc2;          // ADC reading at point 2  
  float temp2;       // Known temperature at point 2
  float slope;       // Calculated slope (temp per ADC count)
  float offset;      // Calculated offset
  bool calibrated;   // True when both points are set and valid
  bool point1Set;    // True when point 1 is explicitly set
  bool point2Set;    // True when point 2 is explicitly set
  String point1Name; // Description of point 1
  String point2Name; // Description of point 2
};

// Global calibration data - Initialize with more flexible defaults
TempCalibration grillCalibration = {
  
  .adc1 = 2881,              // Not set initially
  .temp1 = 272.0,           // Not set initially
  .adc2 = 3297,           // Default from your current reading
  .temp2 = 76.0,          // Current ambient temperature
  
  .slope = 0.0,           // Will be calculated
  .offset = 0.0,          // Will be calculated  
  .calibrated = false,    // Will be true after both points are valid
  .point1Set = true,      // Default point 1 is set
  .point2Set = false,     // Point 2 not set initially
  .point1Name = "Room Temperature",
  .point2Name = "Not Set"
};

// UNIFIED TEMPERATURE VALIDATION FUNCTION
bool isValidTemperature(double temp) {
  if (isnan(temp) || isinf(temp)) return false;
  if (temp <= -900.0 || temp >= 999.0) return false;
  return true;
}

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

// ENHANCED CALIBRATED GRILL TEMPERATURE READING
double readGrillTemperature() {
  // Read ADC with averaging
  int totalADC = 0;
  for (int i = 0; i < 5; i++) {
    totalADC += analogRead(GRILL_TEMP_PIN);
    delay(2);
  }
  int currentADC = totalADC / 5;
  
  double tempF;
  
  if (grillCalibration.calibrated && grillCalibration.point1Set && grillCalibration.point2Set) {
    // Use two-point calibration
    tempF = grillCalibration.offset + (currentADC * grillCalibration.slope);
  } else if (grillCalibration.point1Set) {
    // Use single-point calibration (baseline only)
    tempF = grillCalibration.temp1 - (currentADC - grillCalibration.adc1) * 0.05;
  } else {
    // No calibration - use simple default
    tempF = 80.0 + (currentADC - 3200) * 0.05;  // Rough estimate
  }
  
  // Range check
  if (!isValidTemperature(tempF) || tempF < -50.0 || tempF > 900.0) {
    if (debugGrillSensor) {
      Serial.printf("🔴 GRILL: Temperature out of range: %.1f°F\n", tempF);
    }
    return -999.0;
  }
  
  // Debug output
  if (debugGrillSensor) {
    String calType = "none";
    if (grillCalibration.calibrated) calType = "two-point";
    else if (grillCalibration.point1Set) calType = "single-point";
    
    Serial.printf("🔥 GRILL: ADC=%d, Temp=%.1f°F (%s calibration)\n", 
                  currentADC, tempF, calType.c_str());
  }
  
  lastValidGrillTemp = tempF;
  return tempF;
}

// 100k NTC Ambient Temperature Reading (unchanged)
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
// ENHANCED CALIBRATION SYSTEM
// =============================================================================

// Calculate calibration slope and offset
void calculateCalibration() {
  if (!grillCalibration.point1Set || !grillCalibration.point2Set) {
    grillCalibration.calibrated = false;
    return;
  }
  
  // Ensure points are different
  if (grillCalibration.adc1 == grillCalibration.adc2) {
    Serial.println("❌ Error: Calibration points have same ADC value");
    grillCalibration.calibrated = false;
    return;
  }
  
  // Calculate linear calibration: temp = offset + (adc * slope)
  grillCalibration.slope = (grillCalibration.temp2 - grillCalibration.temp1) / 
                          (grillCalibration.adc2 - grillCalibration.adc1);
  grillCalibration.offset = grillCalibration.temp1 - (grillCalibration.adc1 * grillCalibration.slope);
  grillCalibration.calibrated = true;
  
  Serial.printf("✅ Calibration calculated: slope=%.6f, offset=%.2f\n", 
                grillCalibration.slope, grillCalibration.offset);
}

// Set calibration point 1 with specific ADC and temperature
void setCalibrationPoint1(int adc, float temp) {
  grillCalibration.adc1 = adc;
  grillCalibration.temp1 = temp;
  grillCalibration.point1Set = true;
  grillCalibration.point1Name = "Custom Point 1";
  
  calculateCalibration();
  saveCalibrationData();
  
  Serial.printf("✅ Point 1 set: ADC=%d, Temp=%.1f°F\n", adc, temp);
}

// Set calibration point 2 with specific ADC and temperature
void setCalibrationPoint2(int adc, float temp) {
  grillCalibration.adc2 = adc;
  grillCalibration.temp2 = temp;
  grillCalibration.point2Set = true;
  grillCalibration.point2Name = "Custom Point 2";
  
  calculateCalibration();
  saveCalibrationData();
  
  Serial.printf("✅ Point 2 set: ADC=%d, Temp=%.1f°F\n", adc, temp);
}

// Set calibration point 1 using current ADC reading
void setCalibrationPoint1Current(float temp) {
  int currentADC = analogRead(GRILL_TEMP_PIN);
  setCalibrationPoint1(currentADC, temp);
  grillCalibration.point1Name = "Current Reading";
}

// Set calibration point 2 using current ADC reading  
void setCalibrationPoint2Current(float temp) {
  int currentADC = analogRead(GRILL_TEMP_PIN);
  setCalibrationPoint2(currentADC, temp);
  grillCalibration.point2Name = "Current Reading";
}

// Remove point 2 (revert to single-point calibration)
void deleteCalibrationPoint2() {
  grillCalibration.adc2 = 0;
  grillCalibration.temp2 = 0.0;
  grillCalibration.point2Set = false;
  grillCalibration.point2Name = "Not Set";
  grillCalibration.calibrated = false;
  grillCalibration.slope = 0.0;
  grillCalibration.offset = 0.0;
  
  saveCalibrationData();
  Serial.println("🗑️ Point 2 deleted - using single-point calibration");
}

// Get current calibration data
CalibrationData getCalibrationData() {
  CalibrationData data;
  data.adc1 = grillCalibration.adc1;
  data.temp1 = grillCalibration.temp1;
  data.adc2 = grillCalibration.adc2;
  data.temp2 = grillCalibration.temp2;
  data.slope = grillCalibration.slope;
  data.offset = grillCalibration.offset;
  data.calibrated = grillCalibration.calibrated;
  data.point1Set = grillCalibration.point1Set;
  data.point2Set = grillCalibration.point2Set;
  return data;
}

// Check if calibration is reasonable
bool isCalibrationValid() {
  if (!grillCalibration.calibrated) return false;
  
  // Check for reasonable slope (should be negative - higher ADC = lower temp typically)
  if (abs(grillCalibration.slope) < 0.001 || abs(grillCalibration.slope) > 1.0) {
    return false;
  }
  
  // Check for reasonable temperature range
  float tempRange = abs(grillCalibration.temp2 - grillCalibration.temp1);
  if (tempRange < 10.0 || tempRange > 500.0) {
    return false;
  }
  
  return true;
}

// Get calibration equation as string
String getCalibrationEquation() {
  if (grillCalibration.calibrated) {
    return "Temp = " + String(grillCalibration.offset, 2) + " + (" + 
           String(grillCalibration.slope, 6) + " × ADC)";
  } else if (grillCalibration.point1Set) {
    return "Temp = " + String(grillCalibration.temp1, 1) + " + 0.05 × (ADC - " + 
           String(grillCalibration.adc1) + ")";
  } else {
    return "No calibration active";
  }
}

// Predict temperature for given ADC value
float predictTemperature(int adc) {
  if (grillCalibration.calibrated) {
    return grillCalibration.offset + (adc * grillCalibration.slope);
  } else if (grillCalibration.point1Set) {
    return grillCalibration.temp1 + (adc - grillCalibration.adc1) * 0.05;
  } else {
    return 80.0 + (adc - 3200) * 0.05;  // Default estimate
  }
}

// Validate and fix calibration issues
void validateCalibration() {
  if (!grillCalibration.point1Set && !grillCalibration.point2Set) {
    Serial.println("⚠️ No calibration points set");
    return;
  }
  
  if (grillCalibration.point1Set && grillCalibration.point2Set) {
    if (grillCalibration.adc1 == grillCalibration.adc2) {
      Serial.println("⚠️ Calibration points have same ADC - swapping needed");
      return;
    }
    
    // Check if points should be swapped (point 1 should be lower ADC typically)
    if (grillCalibration.adc1 > grillCalibration.adc2) {
      Serial.println("ℹ️ Suggestion: Consider swapping points for logical order");
    }
    
    if (!isCalibrationValid()) {
      Serial.println("⚠️ Calibration appears invalid - check your measurements");
    }
  }
}

// Swap calibration points
void swapCalibrationPoints() {
  if (!grillCalibration.point1Set || !grillCalibration.point2Set) {
    Serial.println("❌ Cannot swap: both points must be set");
    return;
  }
  
  // Swap values
  int tempADC = grillCalibration.adc1;
  float tempTemp = grillCalibration.temp1;
  String tempName = grillCalibration.point1Name;
  
  grillCalibration.adc1 = grillCalibration.adc2;
  grillCalibration.temp1 = grillCalibration.temp2;
  grillCalibration.point1Name = grillCalibration.point2Name;
  
  grillCalibration.adc2 = tempADC;
  grillCalibration.temp2 = tempTemp;
  grillCalibration.point2Name = tempName;
  
  calculateCalibration();
  saveCalibrationData();
  
  Serial.println("🔄 Calibration points swapped");
}

// Get calibration status as JSON
String getCalibrationStatusJSON() {
  String json = "{";
  json += "\"point1Set\":" + String(grillCalibration.point1Set ? "true" : "false") + ",";
  json += "\"point2Set\":" + String(grillCalibration.point2Set ? "true" : "false") + ",";
  json += "\"calibrated\":" + String(grillCalibration.calibrated ? "true" : "false") + ",";
  json += "\"adc1\":" + String(grillCalibration.adc1) + ",";
  json += "\"temp1\":" + String(grillCalibration.temp1, 1) + ",";
  json += "\"adc2\":" + String(grillCalibration.adc2) + ",";
  json += "\"temp2\":" + String(grillCalibration.temp2, 1) + ",";
  json += "\"slope\":" + String(grillCalibration.slope, 6) + ",";
  json += "\"offset\":" + String(grillCalibration.offset, 2) + ",";
  json += "\"point1Name\":\"" + grillCalibration.point1Name + "\",";
  json += "\"point2Name\":\"" + grillCalibration.point2Name + "\",";
  json += "\"equation\":\"" + getCalibrationEquation() + "\",";
  json += "\"valid\":" + String(isCalibrationValid() ? "true" : "false");
  json += "}";
  return json;
}

// =============================================================================
// DATA PERSISTENCE
// =============================================================================

// Save calibration data to preferences
void saveCalibrationData() {
  preferences.begin("grill_cal", false);
  preferences.putInt("adc1", grillCalibration.adc1);
  preferences.putFloat("temp1", grillCalibration.temp1);
  preferences.putInt("adc2", grillCalibration.adc2);
  preferences.putFloat("temp2", grillCalibration.temp2);
  preferences.putFloat("slope", grillCalibration.slope);
  preferences.putFloat("offset", grillCalibration.offset);
  preferences.putBool("calibrated", grillCalibration.calibrated);
  preferences.putBool("point1Set", grillCalibration.point1Set);
  preferences.putBool("point2Set", grillCalibration.point2Set);
  preferences.putString("point1Name", grillCalibration.point1Name);
  preferences.putString("point2Name", grillCalibration.point2Name);
  preferences.end();
  
  Serial.println("📁 Enhanced calibration data saved");
}

// Load calibration data from preferences
void loadCalibrationData() {
  preferences.begin("grill_cal", true);
  
  if (preferences.isKey("calibrated")) {
    grillCalibration.adc1 = preferences.getInt("adc1", 3281);
    grillCalibration.temp1 = preferences.getFloat("temp1", 82.0);
    grillCalibration.adc2 = preferences.getInt("adc2", 0);
    grillCalibration.temp2 = preferences.getFloat("temp2", 0.0);
    grillCalibration.slope = preferences.getFloat("slope", 0.0);
    grillCalibration.offset = preferences.getFloat("offset", 0.0);
    grillCalibration.calibrated = preferences.getBool("calibrated", false);
    grillCalibration.point1Set = preferences.getBool("point1Set", true);
    grillCalibration.point2Set = preferences.getBool("point2Set", false);
    grillCalibration.point1Name = preferences.getString("point1Name", "Room Temperature");
    grillCalibration.point2Name = preferences.getString("point2Name", "Not Set");
    
    Serial.println("📁 Enhanced calibration data loaded");
  } else {
    Serial.println("📁 No saved calibration - using defaults");
  }
  
  preferences.end();
  printCalibrationStatus();
}

// Print enhanced calibration status
void printCalibrationStatus() {
  Serial.println("\n=== ENHANCED TEMPERATURE CALIBRATION STATUS ===");
  
  if (grillCalibration.point1Set) {
    Serial.printf("Point 1: ADC=%d, Temp=%.1f°F (%s)\n", 
                  grillCalibration.adc1, grillCalibration.temp1, grillCalibration.point1Name.c_str());
  } else {
    Serial.println("Point 1: Not set");
  }
  
  if (grillCalibration.point2Set) {
    Serial.printf("Point 2: ADC=%d, Temp=%.1f°F (%s)\n", 
                  grillCalibration.adc2, grillCalibration.temp2, grillCalibration.point2Name.c_str());
  } else {
    Serial.println("Point 2: Not set");
  }
  
  if (grillCalibration.calibrated) {
    Serial.printf("Equation: %s\n", getCalibrationEquation().c_str());
    Serial.printf("Status: ✅ Two-point calibration ACTIVE\n");
    Serial.printf("Valid: %s\n", isCalibrationValid() ? "YES" : "NO");
    
    // Show predicted temperatures
    Serial.println("\nPredicted temperatures:");
    int testADCs[] = {2000, 2500, 3000, 3500, 4000};
    for (int i = 0; i < 5; i++) {
      float temp = predictTemperature(testADCs[i]);
      Serial.printf("  ADC %d → %.1f°F\n", testADCs[i], temp);
    }
  } else if (grillCalibration.point1Set) {
    Serial.printf("Status: ⚠️ Single-point calibration\n");
    Serial.printf("Equation: %s\n", getCalibrationEquation().c_str());
  } else {
    Serial.printf("Status: ❌ No calibration\n");
  }
  
  Serial.println("================================================\n");
}

// Reset calibration to defaults
void resetCalibration() {
  grillCalibration.adc1 = 3281;
  grillCalibration.temp1 = 82.0;
  grillCalibration.adc2 = 0;
  grillCalibration.temp2 = 0.0;
  grillCalibration.slope = 0.0;
  grillCalibration.offset = 0.0;
  grillCalibration.calibrated = false;
  grillCalibration.point1Set = true;
  grillCalibration.point2Set = false;
  grillCalibration.point1Name = "Room Temperature";
  grillCalibration.point2Name = "Not Set";
  
  saveCalibrationData();
  Serial.println("🔄 Calibration reset to defaults");
}

// =============================================================================
// COMMAND HANDLERS (Legacy compatibility + new commands)
// =============================================================================

void handleCalibrationCommands(String command) {
  if (command == "cal_status") {
    printCalibrationStatus();
    
  } else if (command == "cal_reset") {
    resetCalibration();
    
  } else if (command.startsWith("cal_set1_current ")) {
    // Usage: cal_set1_current 82.5
    float temp = command.substring(17).toFloat();
    if (temp >= -50.0 && temp <= 800.0) {
      setCalibrationPoint1Current(temp);
    } else {
      Serial.println("❌ Invalid temperature range");
    }
    
  } else if (command.startsWith("cal_set2_current ")) {
    // Usage: cal_set2_current 200.5
    float temp = command.substring(17).toFloat();
    if (temp >= -50.0 && temp <= 800.0) {
      setCalibrationPoint2Current(temp);
    } else {
      Serial.println("❌ Invalid temperature range");
    }
    
  } else if (command.startsWith("cal_set2 ")) {
    // Legacy compatibility
    float temp = command.substring(9).toFloat();
    setCalibrationPoint2Current(temp);
    
  } else if (command == "cal_delete2") {
    deleteCalibrationPoint2();
    
  } else if (command == "cal_swap") {
    swapCalibrationPoints();
    
  } else if (command == "cal_validate") {
    validateCalibration();
    
  } else if (command.startsWith("cal_test")) {
    int adc = analogRead(GRILL_TEMP_PIN);
    double temp = readGrillTemperature();
    Serial.printf("🧪 Current: ADC=%d, Temp=%.1f°F\n", adc, temp);
    
  } else if (command == "cal_help") {
    Serial.println("\n=== ENHANCED CALIBRATION COMMANDS ===");
    Serial.println("cal_status              - Show detailed calibration status");
    Serial.println("cal_test                - Test current temperature reading");
    Serial.println("cal_set1_current <temp> - Set point 1 using current ADC");
    Serial.println("cal_set2_current <temp> - Set point 2 using current ADC");
    Serial.println("cal_set2 <temp>         - Legacy: set point 2 with current ADC");
    Serial.println("cal_delete2             - Remove point 2 (single-point mode)");
    Serial.println("cal_swap                - Swap point 1 and point 2");
    Serial.println("cal_validate            - Check calibration for issues");
    Serial.println("cal_reset               - Reset to default calibration");
    Serial.println("cal_help                - Show this help");
    Serial.println();
    Serial.println("ENHANCED CALIBRATION PROCEDURE:");
    Serial.println("1. Set point 1: Heat/cool to known temp, run cal_set1_current <temp>");
    Serial.println("2. Set point 2: Change to different temp, run cal_set2_current <temp>");
    Serial.println("3. System automatically calculates linear calibration");
    Serial.println("4. Use cal_validate to check for issues");
    Serial.println("=============================\n");
  }
}

// Setup function
void setupTemperatureCalibration() {
  Serial.println("Initializing enhanced temperature calibration system...");
  loadCalibrationData();
  Serial.println("Type 'cal_help' for calibration commands");
}

// =============================================================================
// DIAGNOSTIC FUNCTIONS (unchanged)
// =============================================================================

void runTemperatureDiagnostics() {
  Serial.println("\n=== INDIVIDUAL SENSOR DIAGNOSTICS ===");
  
  // Test grill sensor
  Serial.println("\n--- GRILL SENSOR TEST ---");
  bool oldGrillDebug = debugGrillSensor;
  debugGrillSensor = true;  
  double grillTemp = readGrillTemperature();
  debugGrillSensor = oldGrillDebug;  
  
  Serial.printf("🔥 Grill temperature: %.1f°F - %s\n", 
                grillTemp, isValidTemperature(grillTemp) ? "VALID" : "INVALID");
  
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
  
  // Get current raw readings
  int adc = analogRead(AMBIENT_TEMP_PIN);
  double voltage = (adc / 4095.0) * 5.0;
  double actualResistance = 10000.0 * (5.0 - voltage) / voltage;
  
  Serial.printf("Current readings: ADC=%d, Voltage=%.3fV, Resistance=%.0fΩ\n", 
                adc, voltage, actualResistance);
  
  Serial.println("=====================================\n");
}