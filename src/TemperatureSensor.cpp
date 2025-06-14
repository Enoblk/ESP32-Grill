// TemperatureSensor.cpp - FIXED voltage divider formula for 5V system
#include "TemperatureSensor.h"
#include <math.h>

TemperatureSensor tempSensor;

TemperatureSensor::TemperatureSensor() : initialized(false) {
  // Initialize all probes as disabled
  for (int i = 0; i < MAX_PROBES; i++) {
    probes[i].type = PROBE_DISABLED;
    probes[i].name = "Disabled";
    probes[i].enabled = false;
    probes[i].offset = 0.0;
    probes[i].minTemp = 32.0;   // 32°F minimum
    probes[i].maxTemp = 250.0;  // 250°F maximum for food probes
    probes[i].lastUpdate = 0;
    probes[i].lastValidTemp = 70.0;  // Room temperature default
    probes[i].isValid = false;
  }
}

bool TemperatureSensor::begin() {
  Serial.println("Initializing ADS1115 for meat probes (1kΩ NTC)...");
  
  Wire.begin();
  
  // Test I2C communication first
  Wire.beginTransmission(ADS1115_ADDRESS);
  int error = Wire.endTransmission();
  
  if (error != 0) {
    Serial.printf("ERROR: ADS1115 I2C communication failed (error %d)\n", error);
    Serial.println("Check I2C wiring: SDA=GPIO21, SCL=GPIO22");
    Serial.println("Check ADS1115 power supply (should be 5V)");
    return false;
  }
  
  if (!ads.begin(ADS1115_ADDRESS)) {
    Serial.println("ERROR: ADS1115 initialization failed!");
    return false;
  }
  
  // Configure ADS1115 for better accuracy with 1kΩ NTC and 5V reference
  ads.setGain(GAIN_ONE);  // +/- 4.096V range (good for 5V reference)
  ads.setDataRate(RATE_ADS1115_860SPS);  // Fast sampling
  
  initialized = true;
  Serial.println("✅ ADS1115 initialized successfully for 1kΩ thermistors with 5V reference");
  
  // Configure all 4 ADS1115 channels for meat probes
  configureProbe(0, PROBE_FOOD_1, "Meat Probe 1");
  configureProbe(1, PROBE_FOOD_2, "Meat Probe 2");
  configureProbe(2, PROBE_FOOD_3, "Meat Probe 3");
  configureProbe(3, PROBE_FOOD_4, "Meat Probe 4");
  
  // Test all channels
  Serial.println("Testing all ADS1115 channels:");
  for (int i = 0; i < 4; i++) {
    int16_t testReading = ads.readADC_SingleEnded(i);
    float testVoltage = ads.computeVolts(testReading);
    Serial.printf("  Channel %d: ADC=%d, Voltage=%.3fV\n", i, testReading, testVoltage);
  }
  
  return true;
}

void TemperatureSensor::configureProbe(int probeIndex, ProbeType type, String name, float offset) {
  if (probeIndex < 0 || probeIndex >= MAX_PROBES) return;
  
  probes[probeIndex].type = type;
  probes[probeIndex].name = name;
  probes[probeIndex].enabled = (type != PROBE_DISABLED);
  probes[probeIndex].offset = offset;
  probes[probeIndex].isValid = false;
  
  Serial.printf("Probe %d configured: %s (%s)\n", 
                probeIndex, name.c_str(), 
                type == PROBE_DISABLED ? "Disabled" : "Enabled");
}

void TemperatureSensor::disableProbe(int probeIndex) {
  configureProbe(probeIndex, PROBE_DISABLED, "Disabled");
}

float TemperatureSensor::calculateTemperature(int16_t adcValue) {
  if (!initialized) return -999.0;
  
  // Convert ADC value to voltage
  float voltage = ads.computeVolts(adcValue);
  
  // CHECK FOR DISCONNECTED PROBE FIRST
  if (adcValue >= 32760) {  // Near maximum ADC value = disconnected
    return -999.0;  // Mark as disconnected, don't calculate temperature
  }
  
  // Handle edge cases for 5V system
  if (voltage <= 0.1 || voltage >= 4.9) {
    return -999.0;
  }
  
  // FIXED: Calculate thermistor resistance using CORRECT voltage divider formula
  // Circuit: 5V → 10kΩ built-in pullup → ADS input → 1kΩ NTC → GND
  // 
  // Voltage divider: V = 5V × R_thermistor / (R_pullup + R_thermistor)
  // Solving for R_thermistor: R_thermistor = R_pullup × V / (5V - V)
  //
  // This is the CORRECT formula for this circuit configuration
  float thermistorResistance = SERIES_RESISTOR * voltage / (SUPPLY_VOLTAGE - voltage);
  
  // Sanity check for 1K NTC (should be ~500-5000Ω in normal cooking range)
  if (thermistorResistance < 100 || thermistorResistance > 10000) {
    return -999.0;
  }
  
  // Standard Steinhart-Hart equation for NTC thermistor
  float steinhart = thermistorResistance / THERMISTOR_NOMINAL;
  steinhart = log(steinhart);
  steinhart /= B_COEFFICIENT;
  steinhart += 1.0 / (TEMPERATURE_NOMINAL + 273.15);
  steinhart = 1.0 / steinhart;
  steinhart -= 273.15;  // Convert to Celsius
  
  // Convert to Fahrenheit
  float tempF = steinhart * 9.0 / 5.0 + 32.0;
  
  return tempF;
}

bool TemperatureSensor::validateTemperature(float temp, int probeIndex) {
  if (probeIndex < 0 || probeIndex >= MAX_PROBES) return false;
  
  // Check for invalid values
  if (isnan(temp) || isinf(temp) || temp == -999.0) {
    return false;
  }
  
  // Check temperature range for food probes (more restrictive than grill temp)
  if (temp < probes[probeIndex].minTemp || temp > probes[probeIndex].maxTemp) {
    return false;
  }
  
  return true;
}

float TemperatureSensor::readProbe(int probeIndex) {
  if (!initialized || probeIndex < 0 || probeIndex >= MAX_PROBES) {
    return -999.0;
  }
  
  if (!probes[probeIndex].enabled) {
    return -999.0;
  }
  
  // Read ADC value with retry mechanism
  int16_t adcValue = -1;
  int retries = 3;
  
  for (int i = 0; i < retries; i++) {
    adcValue = ads.readADC_SingleEnded(probeIndex);
    if (adcValue != -1) break;
    delay(10);
  }
  
  if (adcValue == -1) {
    return -999.0;
  }
  
  // Quick disconnected check
  if (adcValue >= 32760) {
    return -999.0;  // Disconnected
  }
  
  // Calculate temperature
  float temp = calculateTemperature(adcValue);
  
  // Apply calibration offset
  if (temp > -900.0) {  // Valid reading
    temp += probes[probeIndex].offset;
  }
  
  // Validate reading
  if (validateTemperature(temp, probeIndex)) {
    probes[probeIndex].lastValidTemp = temp;
    probes[probeIndex].isValid = true;
    probes[probeIndex].lastUpdate = millis();
    return temp;
  } else {
    probes[probeIndex].isValid = false;
    
    // Use last valid reading if recent (within 30 seconds)
    if ((millis() - probes[probeIndex].lastUpdate) < 30000) {
      return probes[probeIndex].lastValidTemp;
    }
    
    return -999.0;  // No valid reading available
  }
}

float TemperatureSensor::getFoodTemperature(int foodProbe) {
  if (foodProbe < 1 || foodProbe > 4) {
    return -999.0;
  }
  
  // Map food probe number to ADS1115 channel
  int channel = foodProbe - 1;  // Food probe 1 = channel 0, etc.
  return readProbe(channel);
}

bool TemperatureSensor::isProbeValid(int probeIndex) {
  if (probeIndex < 0 || probeIndex >= MAX_PROBES) return false;
  return probes[probeIndex].isValid;
}

String TemperatureSensor::getProbeName(int probeIndex) {
  if (probeIndex < 0 || probeIndex >= MAX_PROBES) return "Invalid";
  return probes[probeIndex].name;
}

ProbeType TemperatureSensor::getProbeType(int probeIndex) {
  if (probeIndex < 0 || probeIndex >= MAX_PROBES) return PROBE_DISABLED;
  return probes[probeIndex].type;
}

void TemperatureSensor::updateAll() {
  if (!initialized) return;
  
  static unsigned long lastUpdate = 0;
  
  // Update all probes every 1 second
  if (millis() - lastUpdate >= 1000) {
    for (int i = 0; i < MAX_PROBES; i++) {
      if (probes[i].enabled) {
        readProbe(i);  // This updates the probe data
      }
    }
    lastUpdate = millis();
  }
}

void TemperatureSensor::calibrateProbe(int probeIndex, float actualTemp) {
  if (probeIndex < 0 || probeIndex >= MAX_PROBES) return;
  
  float currentReading = readProbe(probeIndex);
  if (currentReading > -900.0) {  // Valid reading
    float newOffset = actualTemp - currentReading;
    probes[probeIndex].offset += newOffset;
    
    Serial.printf("Probe %d calibrated: offset now %.1f°F\n", 
                  probeIndex, probes[probeIndex].offset);
  }
}

String TemperatureSensor::getProbeDataJSON() {
  String json = "{\"probes\":[";
  
  for (int i = 0; i < MAX_PROBES; i++) {
    if (i > 0) json += ",";
    
    json += "{";
    json += "\"index\":" + String(i) + ",";
    json += "\"name\":\"" + probes[i].name + "\",";
    json += "\"enabled\":" + String(probes[i].enabled ? "true" : "false") + ",";
    json += "\"type\":" + String((int)probes[i].type) + ",";
    
    if (probes[i].enabled) {
      float temp = readProbe(i);
      if (temp > -900.0) {
        json += "\"temperature\":" + String(temp, 1) + ",";
        json += "\"valid\":true";
      } else {
        json += "\"temperature\":null,";
        json += "\"valid\":false";
      }
    } else {
      json += "\"temperature\":null,";
      json += "\"valid\":false";
    }
    
    json += "}";
  }
  
  json += "]}";
  return json;
}

void TemperatureSensor::printDiagnostics() {
  Serial.println("\n=== ADS1115 TEMPERATURE SENSOR DIAGNOSTICS ===");
  Serial.printf("Initialized: %s\n", initialized ? "YES" : "NO");
  Serial.printf("I2C Address: 0x%02X\n", ADS1115_ADDRESS);
  Serial.println("Thermistor: 1K NTC (4 Meat Probes)");
  Serial.printf("Beta coefficient: %.0f\n", B_COEFFICIENT);
  Serial.printf("Series resistor: %.0fΩ (10k built-in pullup)\n", SERIES_RESISTOR);
  Serial.printf("Supply voltage: %.1fV\n", SUPPLY_VOLTAGE);
  
  // Test I2C communication
  Wire.beginTransmission(ADS1115_ADDRESS);
  int i2cError = Wire.endTransmission();
  Serial.printf("I2C Communication: %s\n", i2cError == 0 ? "OK" : "FAILED");
  
  if (i2cError != 0) {
    Serial.println("❌ I2C Communication failed - check wiring:");
    Serial.println("   SDA = GPIO21, SCL = GPIO22");
    Serial.println("   Check ADS1115 power (5V)");
    Serial.println("   Check pullup resistors on I2C lines");
  }
  
  Serial.println("\nCURRENT CIRCUIT CONFIGURATION:");
  Serial.println("5V → 10kΩ built-in pullup → ADS input → 1kΩ NTC → GND");
  Serial.println("CORRECTED Formula: R_thermistor = R_pullup × V / (5V - V)");
  
  Serial.println("\nProbe Configuration:");
  for (int i = 0; i < MAX_PROBES; i++) {
    Serial.printf("Probe %d: %s - %s", i, probes[i].name.c_str(), 
                  probes[i].enabled ? "ENABLED" : "DISABLED");
    
    if (probes[i].enabled && initialized && i2cError == 0) {
      int16_t adc = ads.readADC_SingleEnded(i);
      float voltage = ads.computeVolts(adc);
      
      // Use CORRECT formula for resistance calculation
      float resistance = SERIES_RESISTOR * voltage / (SUPPLY_VOLTAGE - voltage);
      float temp = readProbe(i);
      
      Serial.printf(" | ADC: %d, V: %.3f, R: %.0fΩ, Temp: %.1f°F, Valid: %s", 
                    adc, voltage, resistance, temp, probes[i].isValid ? "YES" : "NO");
      
      if (probes[i].offset != 0.0) {
        Serial.printf(", Offset: %.1f°F", probes[i].offset);
      }
      
      // Check for reasonable room temperature readings
      if (temp >= 65 && temp <= 85) {
        Serial.print(" ✅ REASONABLE");
      } else if (temp > 150 && temp < 170) {
        Serial.print(" ⚠️ SUSPICIOUS (check calibration)");
      }
    }
    Serial.println();
  }
  
  Serial.println("==============================================\n");
}

// Additional debugging function to test specific probe
void TemperatureSensor::testProbe(int probeIndex) {
  if (!initialized || probeIndex < 0 || probeIndex >= MAX_PROBES) {
    Serial.printf("Cannot test probe %d - not initialized or invalid\n", probeIndex);
    return;
  }
  
  Serial.printf("\n=== TESTING PROBE %d (1kΩ NTC) ===\n", probeIndex);
  
  // Read raw values multiple times
  for (int i = 0; i < 5; i++) {
    int16_t adc = ads.readADC_SingleEnded(probeIndex);
    float voltage = ads.computeVolts(adc);
    float resistance = SERIES_RESISTOR * voltage / (SUPPLY_VOLTAGE - voltage);  // CORRECT formula
    
    Serial.printf("Reading %d: ADC=%d, V=%.3f, R=%.0fΩ\n", 
                  i + 1, adc, voltage, resistance);
    delay(500);
  }
  
  Serial.printf("=== END TEST PROBE %d ===\n\n", probeIndex);
}

// Function to try different beta coefficients
void TemperatureSensor::testBetaCoefficients(int probeIndex) {
  if (!initialized || probeIndex < 0 || probeIndex >= MAX_PROBES) return;
  
  Serial.printf("\n=== TESTING BETA COEFFICIENTS FOR PROBE %d ===\n", probeIndex);
  
  int16_t adc = ads.readADC_SingleEnded(probeIndex);
  float voltage = ads.computeVolts(adc);
  float resistance = SERIES_RESISTOR * voltage / (SUPPLY_VOLTAGE - voltage);  // CORRECT formula
  
  Serial.printf("Raw data: ADC=%d, V=%.3f, R=%.0fΩ\n", adc, voltage, resistance);
  
  // Test different beta values for 1kΩ NTC
  float betas[] = {3435, 3950, 4050, 3380, 3977};
  int numBetas = sizeof(betas) / sizeof(betas[0]);
  
  for (int i = 0; i < numBetas; i++) {
    float beta = betas[i];
    float steinhart = resistance / THERMISTOR_NOMINAL;
    steinhart = log(steinhart);
    steinhart /= beta;
    steinhart += 1.0 / (TEMPERATURE_NOMINAL + 273.15);
    steinhart = 1.0 / steinhart;
    steinhart -= 273.15;
    float tempF = steinhart * 9.0 / 5.0 + 32.0;
    
    Serial.printf("Beta %.0f: %.1f°F", beta, tempF);
    if (tempF >= 65 && tempF <= 85) {
      Serial.print(" ← REASONABLE");
    }
    Serial.println();
  }
  
  Serial.printf("=== END BETA TEST ===\n\n");
}