// Main.ino - Fixed file - rename from Main.ino to main.cpp or ensure proper .ino extension
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <SPI.h>  // Added for MAX31865

// Include all your project headers
#include "Globals.h"
#include "Utility.h"
#include "TemperatureSensor.h"
#include "MAX31865Sensor.h"  // New MAX31865 support
#include "RelayControl.h"
#include "PelletControl.h"
#include "Ignition.h"
#include "ButtonInput.h"
#include "OLEDDisplay.h"
#include "WiFiManager.h"
#include "GrillWebServer.h"
#include "Settings.h"

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println("\n=====================================");
  Serial.println("ESP32 GRILL CONTROLLER - MAX31865 RTD MODE");
  Serial.println("=====================================");
  
 
  // Initialize I2C for ADS1115 and OLED
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.printf("I2C initialized: SDA=%d, SCL=%d\n", SDA_PIN, SCL_PIN);
 
  // Initialize HSPI for MAX31865 BEFORE I2C
  SPI.begin(MAX31865_CLK_PIN, MAX31865_MISO_PIN, MAX31865_MOSI_PIN);
  Serial.printf("HSPI initialized: CLK=%d, MISO=%d, MOSI=%d\n", 
                MAX31865_CLK_PIN, MAX31865_MISO_PIN, MAX31865_MOSI_PIN);
    
  // Initialize relay control first (safety)
  relay_init();
  
  // Initialize MAX31865 RTD sensor for grill temperature
  Serial.println("\nInitializing MAX31865 RTD sensor...");
  setupTemperatureCalibration();  // This now initializes MAX31865
  
  // Initialize other temperature sensors
  Serial.println("\nInitializing other temperature sensors...");
  
  // Initialize ADS1115 for meat probes
  if (tempSensor.begin()) {
    Serial.println("✅ ADS1115 meat probes initialized");
  } else {
    Serial.println("❌ ADS1115 meat probes failed to initialize");
  }
  
  // Initialize other components
  pellet_init();
  ignition_init();
  button_init();
  
  // Initialize OLED (optional)
  if (oledDisplay.begin()) {
    Serial.println("✅ OLED display initialized");
  } else {
    Serial.println("⚠️  OLED display not found - continuing without it");
  }
  
  // Initialize WiFi
  wifiManager.begin();
  
  // Load settings
  load_setpoint();
  
  // Initialize web server
  setup_grill_server();
  
  Serial.println("\nSetup complete! MAX31865 RTD sensor active!");
  
  // Show sensor status
  printCalibrationStatus();
  
  Serial.println("\n🔧 MAX31865 RTD MODE ACTIVE");
  Serial.println("Type 'max_help' for MAX31865 commands");
  Serial.println("Type 'help' for all available commands");
  Serial.println("=====================================\n");
}

void loop() {
  static unsigned long lastMainLoop = 0;
  static unsigned long lastTempUpdate = 0;
  static unsigned long lastStatusPrint = 0;
  static unsigned long lastDiagnostic = 0;
  
  unsigned long now = millis();
  
  // Handle serial commands
  handleSerialCommands();
  
  // Main control loop - every 100ms
  if (now - lastMainLoop >= MAIN_LOOP_INTERVAL) {
    
    // Handle buttons
    handle_buttons();
    
    // Update WiFi manager
    wifiManager.loop();
    
    // Update relay control
    relay_update();
    
    // Update OLED display
    oledDisplay.update();
    
    lastMainLoop = now;
  }
  
  // Temperature and control updates - every 1 second
  if (now - lastTempUpdate >= TEMP_UPDATE_INTERVAL) {
    
    // Update meat probe sensors
    tempSensor.updateAll();
    
    // Run ignition sequence if active
    ignition_loop();
    
    // Run pellet control if grill is running
    pellet_feed_loop();
    
    // Check for emergency conditions using MAX31865
    double grillTemp = readGrillTemperature();
    if (isValidTemperature(grillTemp) && grillTemp > EMERGENCY_TEMP) {
      Serial.printf("EMERGENCY: Temperature %.1f°F exceeds limit!\n", grillTemp);
      relay_emergency_stop();
      grillRunning = false;
    }
    
    // Check for MAX31865 faults
    if (grillSensor.hasFault()) {
      Serial.printf("⚠️ MAX31865 Fault: %s\n", grillSensor.getFaultString().c_str());
      grillSensor.clearFault();  // Auto-clear faults for continuous operation
    }
    
    lastTempUpdate = now;
  }
  
  // Status printing - every 10 seconds
  if (now - lastStatusPrint >= STATUS_PRINT_INTERVAL) {
    printSystemStatus();
    lastStatusPrint = now;
  }
  
  // DIAGNOSTIC MODE - Run detailed diagnostics every 30 seconds
  if (now - lastDiagnostic >= 30000) {
    if (getSystemDebug()) {  // Only if system debug is enabled
      Serial.println("\n" + String("=").substring(0, 60));
      Serial.println("PERIODIC DIAGNOSTIC CHECK");
      Serial.println(String("=").substring(0, 60));
      
      runTemperatureDiagnostics();
      
      lastDiagnostic = now;
    }
    lastDiagnostic = now;  // Update timestamp even if debug is off
  }
  
  // Allow other tasks to run
  yield();
}

void printSystemStatus() {
  Serial.println("\n--- SYSTEM STATUS ---");
  
  // Grill temperature with MAX31865 info
  double grillTemp = readGrillTemperature();
  Serial.printf("Grill Temp: %.1f°F", grillTemp);
  if (!isValidTemperature(grillTemp)) {
    Serial.print(" (SENSOR ERROR)");
    if (grillSensor.hasFault()) {
      Serial.printf(" - %s", grillSensor.getFaultString().c_str());
    }
  } else {
    Serial.printf(" (R: %.1fΩ)", grillSensor.readRTD());
  }
  Serial.println();
  
  // Ambient temperature
  double ambientTemp = readAmbientTemperature();
  Serial.printf("Ambient Temp: %.1f°F", ambientTemp);
  if (!isValidTemperature(ambientTemp)) Serial.print(" (SENSOR ERROR)");
  Serial.println();
  
  Serial.printf("Target: %.1f°F\n", setpoint);
  
  // Meat probes
  for (int i = 1; i <= 4; i++) {
    float temp = tempSensor.getFoodTemperature(i);
    Serial.printf("Meat Probe %d: %.1f°F", i, temp);
    if (!isValidTemperature(temp)) {
      Serial.print(" (NO PROBE)");
    }
    Serial.println();
  }
  
  // System status
  Serial.printf("Grill: %s\n", grillRunning ? "RUNNING" : "STOPPED");
  if (grillRunning) {
    Serial.printf("Ignition: %s\n", ignition_get_status_string().c_str());
    Serial.printf("Pellet Control: %s\n", pellet_get_status().c_str());
  }
  
  // Relay status
  Serial.printf("Relays: IGN=%s AUG=%s HOP=%s BLO=%s\n",
                digitalRead(RELAY_IGNITER_PIN) ? "ON" : "OFF",
                digitalRead(RELAY_AUGER_PIN) ? "ON" : "OFF", 
                digitalRead(RELAY_HOPPER_FAN_PIN) ? "ON" : "OFF",
                digitalRead(RELAY_BLOWER_FAN_PIN) ? "ON" : "OFF");
  
  // MAX31865 status
  Serial.printf("MAX31865: %s", grillSensor.isConnected() ? "OK" : "ERROR");
  if (grillSensor.hasFault()) {
    Serial.printf(" (Fault: %s)", grillSensor.getFaultString().c_str());
  }
  Serial.println();
  
  // WiFi status
  Serial.printf("WiFi: %s", wifiManager.getStatusString().c_str());
  if (wifiManager.isConnected()) {
    Serial.printf(" (%s)", wifiManager.getIP().toString().c_str());
  }
  Serial.println();
  
  // Memory
  Serial.printf("Free Memory: %d bytes\n", ESP.getFreeHeap());
  
  Serial.println("-------------------\n");
}

// Enhanced serial command handler with MAX31865 support
void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    // Handle MAX31865 commands first
    if (command.startsWith("max_")) {
      handleCalibrationCommands(command);
      return;
    }
    
    // Handle legacy calibration commands (redirect to MAX31865)
    if (command.startsWith("cal_")) {
      handleCalibrationCommands(command);
      return;
    }
    
    if (command == "test_meat") {
      // Test meat probes
      bool oldDebug = getMeatProbesDebug();
      setMeatProbesDebug(true);
      
      testSpecificProbe();
      
      setMeatProbesDebug(oldDebug);
    } else if (command == "test_ambient") {
      testAmbientSensor();
    } else if (command == "test_ambient_detailed") {
      testAmbientNTC();
    } else if (command == "test_grill") {
      testGrillSensor();  // Now tests MAX31865
    } else if (command == "diag") {
      runTemperatureDiagnostics();
    } else if (command == "status") {
      printSystemStatus();
    } else if (command == "i2c_scan") {
      Serial.println("\n=== I2C BUS SCAN ===");
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
        Serial.println("No I2C devices found");
      } else {
        Serial.printf("Found %d device(s)\n", deviceCount);
      }
    } else if (command == "spi_test") {
      Serial.println("\n=== MAX31865 SPI TEST ===");
      grillSensor.testConnection();
    } else if (command.startsWith("calibrate_probe")) {
      // Example: calibrate_probe 1 72.5
      int spaceIndex1 = command.indexOf(' ');
      int spaceIndex2 = command.indexOf(' ', spaceIndex1 + 1);
      
      if (spaceIndex1 > 0 && spaceIndex2 > spaceIndex1) {
        int probeNum = command.substring(spaceIndex1 + 1, spaceIndex2).toInt();
        float actualTemp = command.substring(spaceIndex2 + 1).toFloat();
        
        if (probeNum >= 1 && probeNum <= 4 && actualTemp > 30 && actualTemp < 300) {
          tempSensor.calibrateProbe(probeNum - 1, actualTemp);
          Serial.printf("Calibrated probe %d to %.1f°F\n", probeNum, actualTemp);
        } else {
          Serial.println("Invalid parameters. Example: calibrate_probe 1 72.5");
        }
      } else {
        Serial.println("Usage: calibrate_probe [1-4] [temperature]");
        Serial.println("Example: calibrate_probe 1 72.5");
      }
    } else if (command == "debug_meat_on") {
      setMeatProbesDebug(true);
    } else if (command == "debug_meat_off") {
      setMeatProbesDebug(false);
    } else if (command == "debug_grill_on") {
      setGrillDebug(true);
    } else if (command == "debug_grill_off") {
      setGrillDebug(false);
    } else if (command == "debug_ambient_on") {
      setAmbientDebug(true);
    } else if (command == "debug_ambient_off") {
      setAmbientDebug(false);
    } else if (command == "debug_on") {
      setAllDebug(true);
    } else if (command == "debug_off") {
      setAllDebug(false);
    } else if (command == "relay_status") {
      relay_print_status();
    } else if (command == "help") {
      Serial.println("\nAvailable commands:");
      Serial.println("=== MAX31865 RTD COMMANDS ===");
      Serial.println("  max_help        - Show MAX31865 commands");
      Serial.println("  max_status      - Show MAX31865 status");
      Serial.println("  max_test        - Test MAX31865 connection");
      Serial.println("  max_diag        - Detailed MAX31865 diagnostics");
      Serial.println("  max_cal_offset <temp> - Calibrate to known temperature");
      Serial.println("  max_debug_on/off - Toggle MAX31865 debug output");
      Serial.println("");
      Serial.println("=== DIAGNOSTIC COMMANDS ===");
      Serial.println("  test_meat       - Test all meat probes individually");
      Serial.println("  test_ambient    - Test ambient sensor 10 times");
      Serial.println("  test_grill      - Test MAX31865 grill sensor");
      Serial.println("  diag            - Run full temperature diagnostics");
      Serial.println("  status          - Print current system status");
      Serial.println("  i2c_scan        - Scan I2C bus for devices");
      Serial.println("  spi_test        - Test MAX31865 SPI communication");
      Serial.println("");
      Serial.println("=== DEBUG COMMANDS ===");
      Serial.println("  debug_meat_on/off    - Toggle meat probe debug");
      Serial.println("  debug_grill_on/off   - Toggle grill sensor debug");
      Serial.println("  debug_ambient_on/off - Toggle ambient sensor debug");
      Serial.println("  debug_on / debug_off - Toggle ALL debug output");
      Serial.println("");
      Serial.println("=== OTHER COMMANDS ===");
      Serial.println("  calibrate_probe [1-4] [temp] - Calibrate meat probe");
      Serial.println("  relay_status     - Show relay status");
      Serial.println("  help            - Show this help message");
    } else if (command.length() > 0) {
      Serial.println("Unknown command. Type 'help' for available commands.");
    }
  }
}

// Test functions for compatibility
void testSpecificProbe() {
  Serial.println("Testing all meat probes individually:");
  
  for (int i = 0; i < 4; i++) {
    Serial.printf("\n=== TESTING MEAT PROBE %d ===\n", i + 1);
    tempSensor.testProbe(i);
    delay(1000);
  }
}

void testAmbientSensor() {
  Serial.println("\n=== TESTING AMBIENT SENSOR ===");

  for (int i = 0; i < 10; i++) {
    int adc = analogRead(AMBIENT_TEMP_PIN);
    float voltage = (adc / 4095.0) * 5.0;
    float resistance = 10000.0 * (5.0 - voltage) / voltage;
    
    Serial.printf("Reading %d: ADC=%d, V=%.3f, R=%.0fΩ\n", 
                  i + 1, adc, voltage, resistance);
    delay(500);
  }
}