// MAX31865Sensor.cpp - Implementation for MAX31865 RTD sensor
#include "MAX31865Sensor.h"
#include "Globals.h"
#include "Utility.h"

// Global instance
MAX31865Sensor grillSensor;

MAX31865Sensor::MAX31865Sensor() {
  rtd = nullptr;
  initialized = false;
  debugEnabled = false;
  csPin = 0;
  rref = 430.0;
  rnominal = 100.0;
  is3Wire = false;
  lastValidTemp = 70.0;  // Room temperature default
  lastUpdate = 0;
  errorCount = 0;
  tempOffset = 0.0;
  tempScale = 1.0;
  lastError = "";
  lastFaultCode = 0;
}

MAX31865Sensor::~MAX31865Sensor() {
  if (rtd != nullptr) {
    delete rtd;
  }
}

bool MAX31865Sensor::begin(uint8_t cs_pin, float ref_resistor, float nominal_resistor, bool three_wire) {
  csPin = cs_pin;
  rref = ref_resistor;
  rnominal = nominal_resistor;
  is3Wire = three_wire;
  
  Serial.println("Initializing MAX31865 RTD sensor...");
  Serial.printf("Config: CS=GPIO%d, RREF=%.1fΩ, RNOMINAL=%.1fΩ, %s-wire\n", 
                cs_pin, ref_resistor, nominal_resistor, three_wire ? "3" : "4");
  
  // Create MAX31865 instance with CS pin - FIXED CONSTRUCTOR
  if (rtd != nullptr) {
    delete rtd;
  }
  rtd = new Adafruit_MAX31865(cs_pin);  // CS pin required in constructor
  
  // Initialize MAX31865 with wire configuration - FIXED BEGIN CALL
  max31865_numwires_t wireMode = three_wire ? MAX31865_3WIRE : MAX31865_4WIRE;
  if (!rtd->begin(wireMode)) {  // Only wire mode parameter
    Serial.println("❌ Failed to initialize MAX31865");
    lastError = "Initialization failed";
    return false;
  }
  
  Serial.println("✅ MAX31865 initialized successfully");
  
  // Configure the sensor
  setWireMode(three_wire);
  setFilterFrequency(true);  // 60Hz filter
  enableBias(true);
  enableAutoConvert(true);
  
  // Test initial reading
  delay(100);  // Let it settle
  float testTemp = readTemperatureF();
  
  if (testTemp > -200.0 && testTemp < 1000.0) {
    Serial.printf("✅ Initial reading: %.1f°F\n", testTemp);
    initialized = true;
    lastValidTemp = testTemp;
    lastUpdate = millis();
    return true;
  } else {
    Serial.printf("❌ Initial reading failed: %.1f°F\n", testTemp);
    
    // Check for faults
    uint8_t fault = readFault();
    if (fault) {
      Serial.printf("❌ Fault detected: 0x%02X - %s\n", fault, getFaultDescription(fault).c_str());
      lastError = "Fault: " + getFaultDescription(fault);
    } else {
      lastError = "Invalid temperature reading";
    }
    return false;
  }
}

float MAX31865Sensor::readTemperatureF() {
  if (!initialized || rtd == nullptr) {
    if (debugEnabled) {
      Serial.println("🔴 GRILL MAX31865: Not initialized");
    }
    return -999.0;
  }
  
  // Read temperature from MAX31865
  float tempC = rtd->temperature(rnominal, rref);
  
  // Check for faults first
  uint8_t fault = rtd->readFault();
  if (fault) {
    errorCount++;
    lastFaultCode = fault;
    lastError = getFaultDescription(fault);
    
    if (debugEnabled) {
      Serial.printf("🔴 GRILL MAX31865: Fault 0x%02X - %s\n", fault, lastError.c_str());
    }
    
    // Clear the fault for next reading
    rtd->clearFault();
    
    // Return last valid reading if recent (within 30 seconds)
    if (millis() - lastUpdate < 30000) {
      if (debugEnabled) {
        Serial.printf("🔄 GRILL MAX31865: Using last valid reading %.1f°F\n", lastValidTemp);
      }
      return lastValidTemp;
    }
    
    return -999.0;  // Fault condition
  }
  
  // Convert to Fahrenheit
  float tempF = tempC * 9.0 / 5.0 + 32.0;
  
  // Apply calibration
  tempF = (tempF * tempScale) + tempOffset;
  
  // Validate temperature range
  if (!isValidTemperature(tempF)) {
    errorCount++;
    lastError = "Temperature out of range";
    
    if (debugEnabled) {
      Serial.printf("🔴 GRILL MAX31865: Temperature out of range: %.1f°F\n", tempF);
    }
    
    // Return last valid reading if recent
    if (millis() - lastUpdate < 30000) {
      return lastValidTemp;
    }
    
    return -999.0;
  }
  
  // Good reading
  lastValidTemp = tempF;
  lastUpdate = millis();
  lastError = "";
  
  if (debugEnabled) {
    float resistance = readRTD();
    Serial.printf("🔥 GRILL MAX31865: Temp=%.1f°F (%.1f°C), Resistance=%.1fΩ, Raw=0x%04X\n", 
                  tempF, tempC, resistance, readRTDRaw());
  }
  
  return tempF;
}

float MAX31865Sensor::readTemperatureC() {
  float tempF = readTemperatureF();
  if (tempF <= -900.0) return -999.0;
  return (tempF - 32.0) * 5.0 / 9.0;
}

float MAX31865Sensor::readTemperature() {
  return readTemperatureF();  // Default to Fahrenheit
}

float MAX31865Sensor::readRTD() {
  if (!initialized || rtd == nullptr) return -1.0;
  
  uint16_t rtdValue = rtd->readRTD();
  float resistance = rtdValue;
  resistance /= 32768;
  resistance *= rref;
  
  return resistance;
}

uint16_t MAX31865Sensor::readRTDRaw() {
  if (!initialized || rtd == nullptr) return 0;
  return rtd->readRTD();
}

uint8_t MAX31865Sensor::readFault() {
  if (!initialized || rtd == nullptr) return 0xFF;
  return rtd->readFault();
}

bool MAX31865Sensor::hasFault() {
  return readFault() != 0;
}

String MAX31865Sensor::getFaultString() {
  uint8_t fault = readFault();
  if (fault == 0) return "No fault";
  return getFaultDescription(fault);
}

String MAX31865Sensor::getFaultDescription(uint8_t fault) {
  String desc = "";
  
  if (fault & MAX31865_FAULT_HIGHTHRESH) {
    desc += "RTD High Threshold; ";
  }
  if (fault & MAX31865_FAULT_LOWTHRESH) {
    desc += "RTD Low Threshold; ";
  }
  if (fault & MAX31865_FAULT_REFINLOW) {
    desc += "REFIN- > 0.85 x Bias; ";
  }
  if (fault & MAX31865_FAULT_REFINHIGH) {
    desc += "REFIN- < 0.85 x Bias - FORCE- open; ";
  }
  if (fault & MAX31865_FAULT_RTDINLOW) {
    desc += "RTDIN- < 0.85 x Bias - FORCE- open; ";
  }
  if (fault & MAX31865_FAULT_OVUV) {
    desc += "Under/Over voltage; ";
  }
  
  if (desc.length() > 0) {
    desc = desc.substring(0, desc.length() - 2);  // Remove trailing "; "
  } else {
    desc = "Unknown fault (0x" + String(fault, HEX) + ")";
  }
  
  return desc;
}

void MAX31865Sensor::clearFault() {
  if (initialized && rtd != nullptr) {
    rtd->clearFault();
    lastFaultCode = 0;
  }
}

bool MAX31865Sensor::isConnected() {
  if (!initialized || rtd == nullptr) return false;
  
  // Try to read RTD raw value
  uint16_t rawValue = readRTDRaw();
  
  // Check if we get a reasonable value (not 0 or 0xFFFF)
  if (rawValue == 0 || rawValue == 0xFFFF) {
    return false;
  }
  
  // Check for faults
  uint8_t fault = readFault();
  if (fault != 0) {
    return false;
  }
  
  return true;
}

bool MAX31865Sensor::testSPICommunication() {
  Serial.println("\n=== MAX31865 SPI COMMUNICATION TEST ===");
  
  if (!initialized) {
    Serial.println("❌ MAX31865 not initialized");
    return false;
  }
  
  bool testPassed = true;
  
  // Test 1: Basic SPI Setup
  Serial.println("Test 1: SPI Setup");
  Serial.printf("CS Pin: GPIO%d\n", csPin);
  Serial.printf("SPI Clock: %d Hz\n", 1000000);  // 1MHz typical
  
  // Test 2: CS Pin Control
  Serial.println("\nTest 2: CS Pin Control");
  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH);
  delay(10);
  digitalWrite(csPin, LOW);
  delay(10);
  digitalWrite(csPin, HIGH);
  Serial.println("✅ CS pin toggle test complete");
  
  // Test 3: Read Configuration Register
  Serial.println("\nTest 3: Configuration Register Read");
  digitalWrite(csPin, LOW);
  delayMicroseconds(10);
  
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  uint8_t configAddr = 0x00;  // Config register address
  SPI.transfer(configAddr);
  uint8_t configValue = SPI.transfer(0x00);
  SPI.endTransaction();
  
  delayMicroseconds(10);
  digitalWrite(csPin, HIGH);
  
  Serial.printf("Config register: 0x%02X\n", configValue);
  
  if (configValue == 0x00 || configValue == 0xFF) {
    Serial.println("❌ Config register read failed - no communication");
    testPassed = false;
  } else {
    Serial.println("✅ Config register read successful");
  }
  
  // Test 4: Read RTD Data Registers
  Serial.println("\nTest 4: RTD Data Register Read");
  digitalWrite(csPin, LOW);
  delayMicroseconds(10);
  
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  SPI.transfer(0x01);  // RTD MSB register
  uint8_t rtdMSB = SPI.transfer(0x00);
  SPI.endTransaction();
  
  delayMicroseconds(10);
  digitalWrite(csPin, HIGH);
  delayMicroseconds(10);
  
  digitalWrite(csPin, LOW);
  delayMicroseconds(10);
  
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  SPI.transfer(0x02);  // RTD LSB register
  uint8_t rtdLSB = SPI.transfer(0x00);
  SPI.endTransaction();
  
  delayMicroseconds(10);
  digitalWrite(csPin, HIGH);
  
  uint16_t rtdRaw = ((uint16_t)rtdMSB << 8) | rtdLSB;
  Serial.printf("RTD Raw: 0x%04X (MSB: 0x%02X, LSB: 0x%02X)\n", rtdRaw, rtdMSB, rtdLSB);
  
  if (rtdRaw == 0x0000 || rtdRaw == 0xFFFF) {
    Serial.println("❌ RTD register read failed");
    testPassed = false;
  } else {
    Serial.println("✅ RTD register read successful");
  }
  
  // Test 5: Read Fault Status
  Serial.println("\nTest 5: Fault Status Register");
  digitalWrite(csPin, LOW);
  delayMicroseconds(10);
  
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  SPI.transfer(0x07);  // Fault status register
  uint8_t faultStatus = SPI.transfer(0x00);
  SPI.endTransaction();
  
  delayMicroseconds(10);
  digitalWrite(csPin, HIGH);
  
  Serial.printf("Fault Status: 0x%02X\n", faultStatus);
  
  if (faultStatus == 0xFF) {
    Serial.println("❌ All faults set - communication or hardware issue");
    testPassed = false;
  } else if (faultStatus == 0x00) {
    Serial.println("✅ No faults detected");
  } else {
    Serial.printf("⚠️ Some faults detected: 0x%02X\n", faultStatus);
  }
  
  // Test 6: Write/Read Test (Configuration Register)
  Serial.println("\nTest 6: Write/Read Test");
  
  // Read current config
  digitalWrite(csPin, LOW);
  delayMicroseconds(10);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  SPI.transfer(0x00);
  uint8_t originalConfig = SPI.transfer(0x00);
  SPI.endTransaction();
  delayMicroseconds(10);
  digitalWrite(csPin, HIGH);
  delay(10);
  
  // Write new config (enable bias)
  uint8_t testConfig = originalConfig | 0x80;  // Set bias bit
  digitalWrite(csPin, LOW);
  delayMicroseconds(10);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  SPI.transfer(0x80);  // Write to config register
  SPI.transfer(testConfig);
  SPI.endTransaction();
  delayMicroseconds(10);
  digitalWrite(csPin, HIGH);
  delay(10);
  
  // Read back config
  digitalWrite(csPin, LOW);
  delayMicroseconds(10);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  SPI.transfer(0x00);
  uint8_t readbackConfig = SPI.transfer(0x00);
  SPI.endTransaction();
  delayMicroseconds(10);
  digitalWrite(csPin, HIGH);
  
  Serial.printf("Original: 0x%02X, Wrote: 0x%02X, Read: 0x%02X\n", 
                originalConfig, testConfig, readbackConfig);
  
  if (readbackConfig == testConfig) {
    Serial.println("✅ Write/Read test successful");
  } else {
    Serial.println("❌ Write/Read test failed");
    testPassed = false;
  }
  
  Serial.println("\n=== SPI TEST SUMMARY ===");
  if (testPassed) {
    Serial.println("✅ SPI Communication: WORKING");
    Serial.println("The MAX31865 is responding to SPI commands");
  } else {
    Serial.println("❌ SPI Communication: FAILED");
    Serial.println("Check wiring, power, and pin definitions");
  }
  
  return testPassed;
}

String MAX31865Sensor::getSPITestResults() {
  String results = "SPI Test Results:\\n";
  
  if (!initialized) {
    return "MAX31865 not initialized\\n";
  }
  
  // Quick SPI test
  digitalWrite(csPin, LOW);
  delayMicroseconds(10);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  SPI.transfer(0x00);  // Config register
  uint8_t config = SPI.transfer(0x00);
  SPI.endTransaction();
  delayMicroseconds(10);
  digitalWrite(csPin, HIGH);
  
  results += "Config Register: 0x" + String(config, HEX) + "\\n";
  
  if (config == 0x00 || config == 0xFF) {
    results += "Status: FAILED - No SPI communication\\n";
    results += "Check: Wiring, power, CS pin\\n";
  } else {
    results += "Status: SPI communication detected\\n";
    
    // Read RTD
    digitalWrite(csPin, LOW);
    delayMicroseconds(10);
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
    SPI.transfer(0x01);
    uint8_t rtdMSB = SPI.transfer(0x00);
    SPI.endTransaction();
    delayMicroseconds(10);
    digitalWrite(csPin, HIGH);
    
    digitalWrite(csPin, LOW);
    delayMicroseconds(10);
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
    SPI.transfer(0x02);
    uint8_t rtdLSB = SPI.transfer(0x00);
    SPI.endTransaction();
    delayMicroseconds(10);
    digitalWrite(csPin, HIGH);
    
    uint16_t rtdValue = ((uint16_t)rtdMSB << 8) | rtdLSB;
    results += "RTD Raw: 0x" + String(rtdValue, HEX) + "\\n";
    
    if (rtdValue == 0x0000 || rtdValue == 0xFFFF) {
      results += "RTD Status: No valid data\\n";
    } else {
      results += "RTD Status: Valid data detected\\n";
    }
  }
  
  return results;
}

void MAX31865Sensor::setCalibration(float offset, float scale) {
  tempOffset = offset;
  tempScale = scale;
  Serial.printf("MAX31865 calibration set: offset=%.2f°F, scale=%.4f\n", offset, scale);
}

void MAX31865Sensor::calibrateOffset(float knownTemp) {
  float currentReading = readTemperatureF();
  if (currentReading > -900.0) {
    tempOffset = knownTemp - currentReading;
    Serial.printf("MAX31865 offset calibration: %.2f°F (was reading %.1f°F, should be %.1f°F)\n", 
                  tempOffset, currentReading, knownTemp);
  } else {
    Serial.println("❌ Cannot calibrate - invalid current reading");
  }
}

void MAX31865Sensor::calibrateTwoPoint(float temp1, float reading1, float temp2, float reading2) {
  if (abs(reading1 - reading2) < 1.0) {
    Serial.println("❌ Two-point calibration failed: readings too close");
    return;
  }
  
  tempScale = (temp2 - temp1) / (reading2 - reading1);
  tempOffset = temp1 - (reading1 * tempScale);
  
  Serial.printf("MAX31865 two-point calibration: scale=%.4f, offset=%.2f°F\n", tempScale, tempOffset);
}

bool MAX31865Sensor::isValidTemperature(float temp) {
  if (isnan(temp) || isinf(temp)) return false;
  if (temp < -50.0 || temp > 1000.0) return false;  // Reasonable range for PT100
  return true;
}

void MAX31865Sensor::setWireMode(bool threeWire) {
  is3Wire = threeWire;
  if (initialized && rtd != nullptr) {
    // Note: Adafruit library sets this during begin(), so this is informational
    Serial.printf("Wire mode set to: %s-wire\n", threeWire ? "3" : "4");
  }
}

void MAX31865Sensor::setFilterFrequency(bool freq60Hz) {
  if (initialized && rtd != nullptr) {
    // This would require direct register access - Adafruit library may not expose this
    Serial.printf("Filter frequency: %dHz\n", freq60Hz ? 60 : 50);
  }
}

void MAX31865Sensor::enableBias(bool enable) {
  if (initialized && rtd != nullptr) {
    // This is handled automatically by the Adafruit library during readings
    Serial.printf("Bias voltage: %s\n", enable ? "enabled" : "disabled");
  }
}

void MAX31865Sensor::enableAutoConvert(bool enable) {
  if (initialized && rtd != nullptr) {
    // This is handled by the Adafruit library
    Serial.printf("Auto-convert: %s\n", enable ? "enabled" : "disabled");
  }
}

bool MAX31865Sensor::testConnection() {
  if (!initialized) return false;
  
  Serial.println("Testing MAX31865 connection...");
  
  // Read raw RTD value
  uint16_t raw = readRTDRaw();
  Serial.printf("Raw RTD value: 0x%04X (%d)\n", raw, raw);
  
  // Read resistance
  float resistance = readRTD();
  Serial.printf("RTD resistance: %.2fΩ\n", resistance);
  
  // Read temperature
  float temp = readTemperatureF();
  Serial.printf("Temperature: %.1f°F\n", temp);
  
  // Check for faults
  uint8_t fault = readFault();
  if (fault) {
    Serial.printf("❌ Fault detected: 0x%02X - %s\n", fault, getFaultDescription(fault).c_str());
    return false;
  }
  
  // Validate readings
  if (raw == 0 || raw == 0xFFFF) {
    Serial.println("❌ Invalid raw reading");
    return false;
  }
  
  if (resistance < 50.0 || resistance > 500.0) {
    Serial.printf("❌ Resistance out of expected range: %.2fΩ\n", resistance);
    return false;
  }
  
  if (!isValidTemperature(temp)) {
    Serial.printf("❌ Temperature out of valid range: %.1f°F\n", temp);
    return false;
  }
  
  Serial.println("✅ Connection test passed");
  return true;
}

void MAX31865Sensor::printDiagnostics() {
  Serial.println("\n=== MAX31865 RTD SENSOR DIAGNOSTICS ===");
  Serial.printf("Initialized: %s\n", initialized ? "YES" : "NO");
  Serial.printf("CS Pin: GPIO%d\n", csPin);
  Serial.printf("Reference Resistor: %.1fΩ\n", rref);
  Serial.printf("Nominal Resistance: %.1fΩ\n", rnominal);
  Serial.printf("Wire Mode: %s-wire\n", is3Wire ? "3" : "4");
  Serial.printf("Debug Enabled: %s\n", debugEnabled ? "YES" : "NO");
  
  if (!initialized) {
    Serial.println("❌ Sensor not initialized");
    Serial.println("Check SPI wiring:");
    Serial.printf("  CS   = GPIO%d\n", MAX31865_CS_PIN);
    Serial.printf("  MOSI = GPIO%d\n", MAX31865_MOSI_PIN);
    Serial.printf("  MISO = GPIO%d\n", MAX31865_MISO_PIN);
    Serial.printf("  CLK  = GPIO%d\n", MAX31865_CLK_PIN);
    Serial.println("=========================================\n");
    return;
  }
  
  Serial.printf("Error Count: %lu\n", errorCount);
  Serial.printf("Last Error: %s\n", lastError.c_str());
  Serial.printf("Last Update: %lu ms ago\n", millis() - lastUpdate);
  Serial.printf("Last Valid Temp: %.1f°F\n", lastValidTemp);
  
  // Current readings
  Serial.println("\nCurrent Readings:");
  uint16_t raw = readRTDRaw();
  float resistance = readRTD();
  float tempF = readTemperatureF();
  uint8_t fault = readFault();
  
  Serial.printf("Raw RTD: 0x%04X (%d)\n", raw, raw);
  Serial.printf("Resistance: %.2fΩ\n", resistance);
  Serial.printf("Temperature: %.1f°F (%.1f°C)\n", tempF, (tempF - 32.0) * 5.0 / 9.0);
  Serial.printf("Fault Status: 0x%02X", fault);
  
  if (fault == 0) {
    Serial.println(" (No faults)");
  } else {
    Serial.printf(" (%s)\n", getFaultDescription(fault).c_str());
  }
  
  // Calibration info
  Serial.println("\nCalibration:");
  Serial.printf("Offset: %.2f°F\n", tempOffset);
  Serial.printf("Scale: %.4f\n", tempScale);
  
  // Expected values
  Serial.println("\nExpected Values:");
  Serial.println("Room temp (70°F): ~108Ω, ~0x37xx raw");
  Serial.println("Boiling (212°F): ~142Ω, ~0x47xx raw");
  Serial.println("Cooking (300°F): ~177Ω, ~0x5Axx raw");
  
  Serial.println("=========================================\n");
}

void MAX31865Sensor::runDiagnosticTest() {
  Serial.println("\n=== MAX31865 DIAGNOSTIC TEST ===");
  
  if (!initialized) {
    Serial.println("❌ Cannot run test - sensor not initialized");
    return;
  }
  
  // Clear any existing faults
  clearFault();
  
  // Take multiple readings
  Serial.println("Taking 5 consecutive readings:");
  for (int i = 0; i < 5; i++) {
    delay(100);
    
    uint16_t raw = readRTDRaw();
    float resistance = readRTD();
    float temp = readTemperatureF();
    uint8_t fault = readFault();
    
    Serial.printf("Reading %d: Raw=0x%04X, R=%.2fΩ, T=%.1f°F, Fault=0x%02X\n", 
                  i + 1, raw, resistance, temp, fault);
    
    if (fault) {
      Serial.printf("  ❌ Fault: %s\n", getFaultDescription(fault).c_str());
      clearFault();
    }
  }
  
  // Connection test
  testConnection();
  
  Serial.println("=================================\n");
}