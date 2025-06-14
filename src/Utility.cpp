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
  double grillVoltage = (grillADC / 4095.0) * 3.3;
  
  Serial.printf("Raw ADC Reading: %d (0-4095 range)\n", grillADC);
  Serial.printf("Calculated Voltage: %.3fV (should be 0.1-3.2V)\n", grillVoltage);
  
  // Check for common PT100 issues
  if (grillADC < 100) {
    Serial.println("⚠️  ISSUE: ADC too low - Check PT100 connection/short circuit");
  } else if (grillADC > 3900) {
    Serial.println("⚠️  ISSUE: ADC too high - Check PT100 connection/open circuit");
  } else {
    Serial.println("✅ ADC reading in normal range");
  }
  
  // Calculate PT100 resistance
  const float SERIES_R = 150.0;
  if (grillVoltage > 0.1 && grillVoltage < 3.2) {
    double rtdResistance = SERIES_R * grillVoltage / (3.3 - grillVoltage);
    Serial.printf("Calculated PT100 Resistance: %.1fΩ (should be ~100-200Ω at room temp)\n", rtdResistance);
  }
  
  double grillTemp = readGrillTemperature();
  Serial.printf("Final Temperature: %.1f°F\n", grillTemp);
  
  // 2. GPIO13 NTC Ambient Temperature Diagnostics
  Serial.println("\n--- GPIO13 10K NTC AMBIENT TEMPERATURE ---");
  int ambientADC = analogRead(AMBIENT_TEMP_PIN);
  double ambientVoltage = (ambientADC / 4095.0) * 3.3;
  
  Serial.printf("Raw ADC Reading: %d (0-4095 range)\n", ambientADC);
  Serial.printf("Calculated Voltage: %.3fV (should be 1.0-3.0V)\n", ambientVoltage);
  
  // Check for common NTC issues
  if (ambientADC < 200) {
    Serial.println("⚠️  ISSUE: ADC too low - Check NTC connection/short circuit");
  } else if (ambientADC > 3800) {
    Serial.println("⚠️  ISSUE: ADC too high - Check NTC connection/open circuit");
  } else {
    Serial.println("✅ ADC reading in normal range");
  }
  
  // Calculate NTC resistance
  const float SERIES_R_NTC = 10000.0;
  if (ambientVoltage > 0.01 && ambientVoltage < 3.29) {
    double ntcResistance = SERIES_R_NTC * (3.3 / ambientVoltage - 1.0);
    Serial.printf("Calculated NTC Resistance: %.0fΩ (should be ~10kΩ at room temp)\n", ntcResistance);
  } else {
    Serial.println("⚠️  Cannot calculate NTC resistance - voltage out of range");
  }
  
  double ambientTemp = readAmbientTemperature();
  Serial.printf("Final Temperature: %.1f°F\n", ambientTemp);
  
  if (ambientTemp == 70.0 || ambientTemp == -999.0) {
    Serial.println("⚠️  ISSUE: Reading fallback value - sensor not working");
  }
  
  // 3. ADS1115 Meat Probe Diagnostics
  Serial.println("\n--- ADS1115 MEAT PROBE DIAGNOSTICS ---");
  
  // Check if ADS1115 is responding
  Wire.beginTransmission(0x48);
  int i2cError = Wire.endTransmission();
  
  if (i2cError == 0) {
    Serial.println("✅ ADS1115 I2C communication OK");
    
    // Test each channel
    for (int channel = 0; channel < 4; channel++) {
      Serial.printf("\n-- Meat Probe %d (ADS1115 A%d) --\n", channel + 1, channel);
      
      // Read raw ADC value
      int16_t rawADC = tempSensor.ads.readADC_SingleEnded(channel);
      float voltage = tempSensor.ads.computeVolts(rawADC);
      
      Serial.printf("Raw ADC: %d (signed 16-bit)\n", rawADC);
      Serial.printf("Voltage: %.3fV (should be 1.0-4.0V)\n", voltage);
      
      // Calculate thermistor resistance
      const float MEAT_SERIES_R = 1000.0;
      const float ADS_SUPPLY = 5.0;
      
      if (voltage > 0.1 && voltage < 4.9) {
        float thermistorR = MEAT_SERIES_R * (ADS_SUPPLY / voltage - 1.0);
        Serial.printf("Calculated 1K NTC Resistance: %.0fΩ (should be ~1kΩ at room temp)\n", thermistorR);
        
        // Check for common issues
        if (thermistorR < 500) {
          Serial.println("⚠️  ISSUE: Resistance too low - Check for short circuit");
        } else if (thermistorR > 5000) {
          Serial.println("⚠️  ISSUE: Resistance too high - Check connection/open circuit");
        }
      } else {
        Serial.println("⚠️  ISSUE: Voltage out of range - check wiring");
      }
      
      float temp = tempSensor.getFoodTemperature(channel + 1);
      Serial.printf("Final Temperature: %.1f°F\n", temp);
      
      if (temp == 158.0 || (temp > 155 && temp < 160)) {
        Serial.println("⚠️  ISSUE: Suspicious temperature reading - likely calibration issue");
      }
    }
  } else {
    Serial.printf("❌ ADS1115 I2C ERROR: %d\n", i2cError);
    Serial.println("Check I2C wiring (SDA=21, SCL=22) and ADS1115 power");
  }
  
  // 4. I2C Bus Scan
  Serial.println("\n--- I2C BUS SCAN ---");
  Serial.println("Scanning for I2C devices...");
  int deviceCount = 0;
  
  for (int address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    int error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.printf("I2C device found at address 0x%02X", address);
      if (address == 0x48) Serial.print(" (ADS1115)");
      if (address == 0x3C) Serial.print(" (OLED)");
      Serial.println();
      deviceCount++;
    }
  }
  
  if (deviceCount == 0) {
    Serial.println("❌ No I2C devices found - check wiring and power");
  } else {
    Serial.printf("✅ Found %d I2C device(s)\n", deviceCount);
  }
  
  // 5. Circuit Connection Test
  Serial.println("\n--- CIRCUIT CONNECTION TEST ---");
  
  // Test all pins for basic connectivity
  Serial.println("Pin voltage readings:");
  Serial.printf("GPIO35 (PT100): %.3fV\n", (analogRead(35) / 4095.0) * 3.3);
  Serial.printf("GPIO13 (Ambient): %.3fV\n", (analogRead(13) / 4095.0) * 3.3);
  
  // 6. Recommendations
  Serial.println("\n--- TROUBLESHOOTING RECOMMENDATIONS ---");
  
  if (ambientTemp == 70.0 || ambientTemp == -999.0) {
    Serial.println("🔧 Ambient Temperature (GPIO13) FAILED:");
    Serial.println("   1. Check 10K NTC thermistor connection");
    Serial.println("   2. Add 10K pullup resistor from GPIO13 to 3.3V");
    Serial.println("   3. Test thermistor resistance: should be ~10kΩ at room temp");
    Serial.println("   4. Verify GPIO13 is not used by another component");
  }
  
  if (grillTemp == 75.0 || grillTemp == -999.0) {
    Serial.println("🔧 Grill Temperature (GPIO35) FAILED:");
    Serial.println("   1. Check PT100 RTD connection");
    Serial.println("   2. Verify 150Ω series resistor from PT100 to GND");
    Serial.println("   3. Test PT100 resistance: should be ~100Ω at room temp");
    Serial.println("   4. Check that PT100 positive goes to GPIO35");
  }
  
  if (i2cError != 0) {
    Serial.println("🔧 ADS1115 (Meat Probes) FAILED:");
    Serial.println("   1. Check I2C connections: SDA=GPIO21, SCL=GPIO22");
    Serial.println("   2. Verify ADS1115 has 5V power supply");
    Serial.println("   3. Check I2C pullup resistors (usually on ADS1115 board)");
    Serial.println("   4. Test with I2C scanner to verify address 0x48");
  }
  
  Serial.println("\n🔧 If meat probes read ~158°F consistently:");
  Serial.println("   1. Check that 1K NTC thermistors are used (not 10K)");
  Serial.println("   2. Verify 1K series resistors on each channel");
  Serial.println("   3. Ensure ADS1115 reference is 5V, not 3.3V");
  Serial.println("   4. Test thermistor resistance: should be ~1kΩ at room temp");
  
  Serial.println("\n=====================================");
  Serial.println("DIAGNOSTICS COMPLETE");
  Serial.println("=====================================\n");
}

// IMPROVED AMBIENT TEMPERATURE READING
double readAmbientTemperature() {
  // NTC thermistor constants for ambient temperature
  const float THERMISTOR_NOMINAL = 100000.0;   // 10k ohm at 25°C
  const float TEMPERATURE_NOMINAL = 25.0;     // 25°C
  const float B_COEFFICIENT = 3950.0;         // NTC beta coefficient
  const float SERIES_RESISTOR = 10000.0;      // 10k series resistor
  const float SUPPLY_VOLTAGE = 3.3;           // ESP32 supply voltage
  const int ADC_RESOLUTION = 4095;            // 12-bit ADC
  
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
  
  double voltage = (adcReading / (double)ADC_RESOLUTION) * SUPPLY_VOLTAGE;
  
  if (debugMode) {
    Serial.printf("Ambient Voltage: %.3fV\n", voltage);
  }
  
  // More strict voltage validation
  if (voltage <= 0.05 || voltage >= (SUPPLY_VOLTAGE - 0.05)) {
    if (debugMode) {
      Serial.printf("Ambient voltage out of range: %.3fV\n", voltage);
    }
    return -999.0;  // Return error code
  }
  
  // Calculate thermistor resistance
  double thermistorResistance = SERIES_RESISTOR * (SUPPLY_VOLTAGE / voltage - 1.0);
  
  if (debugMode) {
    Serial.printf("Ambient NTC Resistance: %.0fΩ\n", thermistorResistance);
  }
  
  // Sanity check on resistance (should be reasonable for 10K NTC)
  if (thermistorResistance < 1000 || thermistorResistance > 1000000) {
    if (debugMode) {
      Serial.printf("Ambient resistance out of range: %.0fΩ\n", thermistorResistance);
    }
    return -999.0;
  }
  
  // Steinhart-Hart equation for NTC thermistor
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
  
  // Reasonable ambient temperature validation
  if (tempF < -40.0 || tempF > 150.0 || isnan(tempF) || isinf(tempF)) {
    if (debugMode) {
      Serial.printf("Ambient temp out of range: %.1f°F\n", tempF);
    }
    return -999.0;
  }
  
  lastValidAmbientTemp = tempF;
  return tempF;
}

// IMPROVED PT100 GRILL TEMPERATURE READING  
double readGrillTemperature() {
  // PT100 constants
  const float RTD_NOMINAL = 100.0;         // 100 ohm at 0°C (PT100)
  const float RTD_ALPHA = 0.00385;         // Temperature coefficient
  const float SERIES_RESISTOR = 150.0;     // 150Ω series resistor
  const float SUPPLY_VOLTAGE = 3.3;        // ESP32 supply voltage
  const int ADC_RESOLUTION = 4095;         // 12-bit ADC
  
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
  
  double voltage = (adcReading / (double)ADC_RESOLUTION) * SUPPLY_VOLTAGE;
  
  if (debugMode) {
    Serial.printf("Grill Voltage: %.3fV\n", voltage);
  }
  
  // Strict voltage validation for PT100
  if (voltage <= 0.1 || voltage >= (SUPPLY_VOLTAGE - 0.1)) {
    if (debugMode) {
      Serial.printf("Grill voltage out of range: %.3fV\n", voltage);
    }
    return -999.0;
  }
  
  // Calculate RTD resistance using voltage divider
  // Circuit: 3.3V → PT100 → GPIO35 → 150Ω → GND
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
  Serial.printf("  Grill Temp: GPIO%d (ESP32 ADC + PT100)\n", GRILL_TEMP_PIN);
  Serial.printf("  Ambient Temp: GPIO%d (ESP32 ADC + 10K NTC)\n", AMBIENT_TEMP_PIN);
  Serial.println("  Meat Probes: ADS1115 A0-A3 (High precision + 1K NTC)");
  
  Serial.println("\nCurrent Readings:");
  double grillTemp = readGrillTemperature();
  double ambientTemp = readAmbientTemperature();
  
  Serial.printf("  Grill Temperature (PT100): %.1f°F\n", grillTemp);
  Serial.printf("  Ambient Temperature (NTC): %.1f°F\n", ambientTemp);
  
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