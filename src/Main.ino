// Main.ino - Fixed for minimal MAX31865 implementation
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <SPI.h>

// Include all your project headers
#include "Globals.h"
#include "Utility.h"
#include "TemperatureSensor.h"
#include "MAX31865Sensor.h"
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
  Serial.println("ESP32 GRILL CONTROLLER - SIMPLE MAX31865");
  Serial.println("=====================================");
  
  // Initialize I2C for ADS1115 and OLED
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.printf("I2C initialized: SDA=%d, SCL=%d\n", SDA_PIN, SCL_PIN);
 
  // Initialize DEFAULT SPI for MAX31865
  SPI.begin();  // Use default ESP32 pins: SCK=18, MISO=19, MOSI=23
  Serial.println("SPI initialized with ESP32 default pins:");
  Serial.printf("- CS Pin: GPIO%d\n", MAX31865_CS_PIN);
  Serial.println("- CLK Pin: GPIO18 (ESP32 default)");
  Serial.println("- MOSI Pin: GPIO23 (ESP32 default)");
  Serial.println("- MISO Pin: GPIO19 (ESP32 default)");
    
  // Initialize relay control first (safety)
  relay_init();
  
  // Initialize MAX31865 RTD sensor for grill temperature
  Serial.println("\nInitializing MAX31865 for 100Ω resistor...");
  if (!grillSensor.begin(MAX31865_CS_PIN, RREF, RNOMINAL)) {  // 3 parameters only
    Serial.println("❌ Failed to initialize MAX31865 sensor");
    Serial.println("Check wiring!");
  } else {
    Serial.println("✅ MAX31865 initialized successfully");
  }
  
  // Setup temperature calibration system
  setupTemperatureCalibration();
  
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
  
  Serial.println("\nSetup complete! Simple MAX31865 active!");
  
  // Show sensor status
  printCalibrationStatus();
  
  Serial.println("\n🔧 SIMPLE MAX31865 MODE ACTIVE");
  Serial.println("Type 'test_temp' to test temperature reading");
  Serial.println("Type 'help' for all available commands");
  Serial.println("=====================================\n");
}

void loop() {
  static unsigned long lastMainLoop = 0;
  static unsigned long lastTempUpdate = 0;
  static unsigned long lastStatusPrint = 0;
  
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
    
    // Check for emergency conditions
    double grillTemp = readGrillTemperature();
    if (isValidTemperature(grillTemp) && grillTemp > EMERGENCY_TEMP) {
      Serial.printf("EMERGENCY: Temperature %.1f°F exceeds limit!\n", grillTemp);
      relay_emergency_stop();
      grillRunning = false;
    }
    
    lastTempUpdate = now;
  }
  
  // Status printing - every 10 seconds
  if (now - lastStatusPrint >= STATUS_PRINT_INTERVAL) {
    printSystemStatus();
    lastStatusPrint = now;
  }
  
  // Allow other tasks to run
  yield();
}

void printSystemStatus() {
  Serial.println("\n--- SYSTEM STATUS ---");
  
  // Grill temperature
  double grillTemp = readGrillTemperature();
  Serial.printf("Grill Temp: %.1f°F", grillTemp);
  if (!isValidTemperature(grillTemp)) {
    Serial.print(" (SENSOR ERROR)");
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
  
  // Simple MAX31865 status
  Serial.printf("MAX31865: %s\n", grillSensor.isInitialized() ? "OK" : "ERROR");
  
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

// Simple serial command handler
void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    // Handle simple commands
    if (command.startsWith("max_") || command.startsWith("cal_")) {
      handleCalibrationCommands(command);
      return;
    }
    
    if (command == "test_temp") {
      testGrillSensor();
    } else if (command == "test_meat") {
      testSpecificProbe();
    } else if (command == "test_ambient") {
      testAmbientSensor();
    } else if (command == "diag") {
      runTemperatureDiagnostics();
    } else if (command == "status") {
      printSystemStatus();
    } else if (command == "debug_on") {
      setAllDebug(true);
    } else if (command == "debug_off") {
      setAllDebug(false);
    } else if (command == "help") {
      Serial.println("\nAvailable commands:");
      Serial.println("=== SIMPLE MAX31865 COMMANDS ===");
      Serial.println("  test_temp       - Test 100Ω resistor reading");
      Serial.println("  debug_on/off    - Toggle debug output");
      Serial.println("");
      Serial.println("=== DIAGNOSTIC COMMANDS ===");
      Serial.println("  test_meat       - Test all meat probes");
      Serial.println("  test_ambient    - Test ambient sensor");
      Serial.println("  diag            - Run temperature diagnostics");
      Serial.println("  status          - Print system status");
      Serial.println("");
      Serial.println("=== OTHER COMMANDS ===");
      Serial.println("  help            - Show this help message");
    } else if (command.length() > 0) {
      Serial.println("Unknown command. Type 'help' for available commands.");
    }
  }
}

// Simple test functions
//void testSpecificProbe() {
 // Serial.println("Testing meat probes...");
//  for (int i = 1; i <= 4; i++) {
//    float temp = tempSensor.getFoodTemperature(i);
//    Serial.printf("Meat Probe %d: %.1f°F\n", i, temp);
//  }
//}

//void testAmbientSensor() {
 // Serial.println("Testing ambient sensor...");
 // for (int i = 0; i < 5; i++) {
 //   double temp = readAmbientTemperature();
 //   Serial.printf("Reading %d: %.1f°F\n", i + 1, temp);
 //   delay(500);
 // }
//}