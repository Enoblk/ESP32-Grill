// Main.ino - Complete file with debugging and adjustable pellet feed parameters
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <SPI.h>

// ESP32 system includes for debugging
#include "esp_system.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_task_wdt.h"

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

// Debug and safety monitoring variables
static unsigned long lastHealthCheck = 0;
static unsigned long lastWatchdogFeed = 0;
static bool systemHealthy = true;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  // Wait a moment for serial to stabilize
  delay(1000);
  
  Serial.println("\n=====================================");
  Serial.println("ESP32 GRILL CONTROLLER - DEBUG VERSION");
  Serial.println("=====================================");
  
  // CRITICAL: Print reset reason to understand restarts
  printResetReason();
  
  // Initialize watchdog timer for main task
  esp_task_wdt_init(30, true); // 30 second timeout, panic on timeout
  esp_task_wdt_add(NULL); // Add current task to watchdog
  
  // TEMPORARY: Disable brownout detector for testing
  Serial.println("⚠️ TEMPORARILY DISABLING BROWNOUT DETECTOR FOR DEBUGGING");
  Serial.println("This helps identify if brownout is the real issue");
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  // Print system information
  Serial.printf("ESP32 Chip ID: %08X\n", (uint32_t)ESP.getEfuseMac());
  Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Flash Size: %d bytes\n", ESP.getFlashChipSize());
  Serial.printf("Arduino Core Version: %s\n", ESP.getSdkVersion());
  
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
  Serial.println("Initializing relay control...");
  relay_init();
  Serial.println("✅ Relay control initialized");
  
  // Feed watchdog after each major initialization step
  esp_task_wdt_reset();
  
  // Initialize MAX31865 RTD sensor for grill temperature
  Serial.println("Initializing MAX31865 for 100Ω resistor...");
  if (!grillSensor.begin(MAX31865_CS_PIN, RREF, RNOMINAL)) {
    Serial.println("❌ Failed to initialize MAX31865 sensor");
    Serial.println("Check wiring!");
  } else {
    Serial.println("✅ MAX31865 initialized successfully");
  }
  
  esp_task_wdt_reset();
  
  // Setup temperature calibration system
  setupTemperatureCalibration();
  
  // Initialize other temperature sensors
  Serial.println("Initializing other temperature sensors...");
  
  // Initialize ADS1115 for meat probes
  if (tempSensor.begin()) {
    Serial.println("✅ ADS1115 meat probes initialized");
  } else {
    Serial.println("❌ ADS1115 meat probes failed to initialize");
  }
  
  esp_task_wdt_reset();
  
  // Initialize other components
  Serial.println("Initializing pellet control...");
  pellet_init();
  Serial.println("✅ Pellet control initialized");
  
  Serial.println("Initializing ignition system...");
  ignition_init();
  Serial.println("✅ Ignition system initialized");
  
  Serial.println("Initializing button input...");
  button_init();
  Serial.println("✅ Button input initialized");
  
  esp_task_wdt_reset();
  
  // Initialize OLED (optional)
  Serial.println("Initializing OLED display...");
  if (oledDisplay.begin()) {
    Serial.println("✅ OLED display initialized");
  } else {
    Serial.println("⚠️  OLED display not found - continuing without it");
  }
  
  // Initialize WiFi
  Serial.println("Initializing WiFi...");
  wifiManager.begin();
  Serial.println("✅ WiFi manager initialized");
  
  esp_task_wdt_reset();
  
  // Load settings
  Serial.println("Loading settings...");
  load_setpoint();
  Serial.println("✅ Settings loaded");
  
  // Initialize web server
  Serial.println("Initializing web server...");
  setup_grill_server();
  Serial.println("✅ Web server initialized");
  
  esp_task_wdt_reset();
  
  Serial.println("Setup complete! Debug version active!");
  
  // Show sensor status
  printCalibrationStatus();
  
  Serial.println("\n🔧 DEBUG MODE FEATURES:");
  Serial.println("- Reset reason detection");
  Serial.println("- System health monitoring");
  Serial.println("- Auger operation debugging");
  Serial.println("- Memory and stack monitoring");
  Serial.println("- Adjustable pellet feed parameters");
  Serial.println("- Brownout detector temporarily disabled");
  
  Serial.println("\n📋 AVAILABLE COMMANDS:");
  Serial.println("  test_temp       - Test 100Ω resistor reading");
  Serial.println("  debug_on/off    - Toggle debug output");
  Serial.println("  health          - Show system health");
  Serial.println("  relay_status    - Show relay status");
  Serial.println("  pellet_status   - Show pellet control status");
  Serial.println("  reset_reason    - Show last reset reason");
  Serial.println("  help            - Show all commands");
  Serial.println("=====================================\n");
  
  // Final system check
  systemHealthy = true;
  Serial.println("🚀 System ready and healthy!");
}

void loop() {
  static unsigned long lastMainLoop = 0;
  static unsigned long lastTempUpdate = 0;
  static unsigned long lastStatusPrint = 0;
  
  unsigned long now = millis();
  
  // Feed watchdog timer
  feedWatchdog();
  
  // Monitor system health
  monitorSystemHealth();
  
  // Handle serial commands
  handleSerialCommands();
  
  // Main control loop - every 100ms
  if (now - lastMainLoop >= MAIN_LOOP_INTERVAL) {
    
    // Handle buttons
    handle_buttons();
    
    // Update WiFi manager
    wifiManager.loop();
    
    // Update relay control (includes manual override timeout)
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
    
    // Run pellet control if grill is running (with debugging)
    debugPelletFeedLoop();
    
    // Check for emergency conditions
    double grillTemp = readGrillTemperature();
    if (isValidTemperature(grillTemp) && grillTemp > EMERGENCY_TEMP) {
      Serial.printf("🚨 EMERGENCY: Temperature %.1f°F exceeds limit!\n", grillTemp);
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

void printResetReason() {
  esp_reset_reason_t reset_reason = esp_reset_reason();
  
  Serial.println("\n=== ESP32 RESET REASON ===");
  switch (reset_reason) {
    case ESP_RST_UNKNOWN:
      Serial.println("❓ Reset reason: UNKNOWN");
      break;
    case ESP_RST_POWERON:
      Serial.println("🔌 Reset reason: Power-on reset (normal startup)");
      break;
    case ESP_RST_EXT:
      Serial.println("🔄 Reset reason: External pin reset");
      break;
    case ESP_RST_SW:
      Serial.println("💻 Reset reason: Software reset via esp_restart()");
      break;
    case ESP_RST_PANIC:
      Serial.println("🚨 Reset reason: Software reset due to exception/panic");
      Serial.println("   ❗ Check for stack overflow, null pointer, or infinite loop!");
      systemHealthy = false;
      break;
    case ESP_RST_INT_WDT:
      Serial.println("⏱️ Reset reason: Interrupt watchdog timeout");
      Serial.println("   ❗ Check for infinite loops in ISR or blocking code!");
      systemHealthy = false;
      break;
    case ESP_RST_TASK_WDT:
      Serial.println("⏲️ Reset reason: Task watchdog timeout");
      Serial.println("   ❗ Check for infinite loops in main code!");
      systemHealthy = false;
      break;
    case ESP_RST_WDT:
      Serial.println("🐕 Reset reason: Other watchdog timeout");
      systemHealthy = false;
      break;
    case ESP_RST_DEEPSLEEP:
      Serial.println("😴 Reset reason: Deep sleep wake up");
      break;
    case ESP_RST_BROWNOUT:
      Serial.println("🔋 Reset reason: Brownout reset (power supply issue)");
      Serial.println("   ❗ Check power supply voltage and current capacity!");
      systemHealthy = false;
      break;
    case ESP_RST_SDIO:
      Serial.println("📱 Reset reason: SDIO reset");
      break;
    default:
      Serial.printf("❔ Reset reason: Other (%d)\n", reset_reason);
      break;
  }
  
  if (!systemHealthy) {
    Serial.println("⚠️ LAST RESTART WAS DUE TO AN ERROR - INVESTIGATE!");
  }
  
  Serial.println("========================\n");
}

void debugPelletFeedLoop() {
  static unsigned long lastAugerState = 0;
  static bool lastAugerValue = false;
  static unsigned long augerOperationCount = 0;
  
  // Check if auger state changed
  bool currentAugerValue = digitalRead(RELAY_AUGER_PIN);
  if (currentAugerValue != lastAugerValue) {
    augerOperationCount++;
    
    Serial.printf("🎯 AUGER STATE CHANGE #%lu: %s -> %s (time: %lu ms since last change)\n", 
                  augerOperationCount,
                  lastAugerValue ? "ON" : "OFF",
                  currentAugerValue ? "ON" : "OFF",
                  millis() - lastAugerState);
    
    // Log system state during auger operation
    Serial.printf("   📊 System state: Free heap=%d, Stack remaining=%d\n", 
                  ESP.getFreeHeap(), uxTaskGetStackHighWaterMark(NULL));
    Serial.printf("   🕐 Uptime: %lu seconds\n", millis() / 1000);
    Serial.printf("   🔧 Manual override: %s\n", relay_get_manual_override_status() ? "ACTIVE" : "INACTIVE");
    
    lastAugerState = millis();
    lastAugerValue = currentAugerValue;
    
    // Small delay after auger state change to let system stabilize
    delay(50);
    
    // Verify we're still alive
    Serial.println("   ✅ System stable after auger state change");
    
    // Extra watchdog feed during critical operations
    esp_task_wdt_reset();
  }
  
  // Run the actual pellet feed loop
  pellet_feed_loop();
}

void monitorSystemHealth() {
  if (millis() - lastHealthCheck > 5000) { // Every 5 seconds
    uint32_t freeHeap = ESP.getFreeHeap();
    UBaseType_t stackRemaining = uxTaskGetStackHighWaterMark(NULL);
    
    Serial.printf("💗 Health: Heap=%d bytes, Stack=%d bytes, Uptime=%lu sec\n", 
                  freeHeap, stackRemaining, millis() / 1000);
    
    // Check for critically low memory
    if (freeHeap < 10000) {
      Serial.println("⚠️ WARNING: Low memory detected!");
      systemHealthy = false;
    }
    
    // Check stack usage
    if (stackRemaining < 500) {
      Serial.printf("⚠️ WARNING: Low stack space: %d bytes remaining\n", stackRemaining);
      systemHealthy = false;
    }
    
    // Check relay states for inconsistencies
    bool ignOn = digitalRead(RELAY_IGNITER_PIN);
    bool augOn = digitalRead(RELAY_AUGER_PIN);
    bool hopOn = digitalRead(RELAY_HOPPER_FAN_PIN);
    bool bloOn = digitalRead(RELAY_BLOWER_FAN_PIN);
    
    static bool lastIgn = false, lastAug = false, lastHop = false, lastBlo = false;
    static bool firstCheck = true;
    
    if (!firstCheck) {
      if (ignOn != lastIgn) Serial.printf("🔧 Igniter changed: %s\n", ignOn ? "ON" : "OFF");
      if (augOn != lastAug) Serial.printf("🔧 Auger changed: %s\n", augOn ? "ON" : "OFF");
      if (hopOn != lastHop) Serial.printf("🔧 Hopper changed: %s\n", hopOn ? "ON" : "OFF");
      if (bloOn != lastBlo) Serial.printf("🔧 Blower changed: %s\n", bloOn ? "ON" : "OFF");
    }
    
    lastIgn = ignOn; lastAug = augOn; lastHop = hopOn; lastBlo = bloOn;
    firstCheck = false;
    
    lastHealthCheck = millis();
  }
}

void feedWatchdog() {
  if (millis() - lastWatchdogFeed > 1000) { // Feed every second
    esp_task_wdt_reset();
    lastWatchdogFeed = millis();
  }
}

void printSystemStatus() {
  Serial.println("\n--- DETAILED SYSTEM STATUS ---");
  
  // System health
  Serial.printf("🏥 System Health: %s\n", systemHealthy ? "HEALTHY" : "⚠️ ISSUES DETECTED");
  Serial.printf("💾 Free Memory: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("📚 Stack Remaining: %d bytes\n", uxTaskGetStackHighWaterMark(NULL));
  Serial.printf("⏰ Uptime: %lu seconds\n", millis() / 1000);
  
  // Grill temperature
  double grillTemp = readGrillTemperature();
  Serial.printf("🔥 Grill Temp: %.1f°F", grillTemp);
  if (!isValidTemperature(grillTemp)) {
    Serial.print(" (SENSOR ERROR)");
  } else {
    Serial.printf(" (R: %.1fΩ)", grillSensor.readRTD());
  }
  Serial.println();
  
  // Ambient temperature
  double ambientTemp = readAmbientTemperature();
  Serial.printf("🌡️ Ambient Temp: %.1f°F", ambientTemp);
  if (!isValidTemperature(ambientTemp)) Serial.print(" (SENSOR ERROR)");
  Serial.println();
  
  Serial.printf("🎯 Target: %.1f°F\n", setpoint);
  
  // Meat probes
  for (int i = 1; i <= 4; i++) {
    float temp = tempSensor.getFoodTemperature(i);
    Serial.printf("🥩 Meat Probe %d: %.1f°F", i, temp);
    if (!isValidTemperature(temp)) {
      Serial.print(" (NO PROBE)");
    }
    Serial.println();
  }
  
  // System status
  Serial.printf("🔥 Grill: %s\n", grillRunning ? "RUNNING" : "STOPPED");
  if (grillRunning) {
    Serial.printf("🚀 Ignition: %s\n", ignition_get_status_string().c_str());
    Serial.printf("🌾 Pellet Control: %s\n", pellet_get_status().c_str());
  }
  
  // Relay status with debugging info
  Serial.printf("🔌 Relays: IGN=%s AUG=%s HOP=%s BLO=%s\n",
                digitalRead(RELAY_IGNITER_PIN) ? "ON" : "OFF",
                digitalRead(RELAY_AUGER_PIN) ? "ON" : "OFF", 
                digitalRead(RELAY_HOPPER_FAN_PIN) ? "ON" : "OFF",
                digitalRead(RELAY_BLOWER_FAN_PIN) ? "ON" : "OFF");
  
  Serial.printf("🎛️ Manual Override: %s\n", relay_get_manual_override_status() ? "ACTIVE" : "INACTIVE");
  
  // MAX31865 status
  Serial.printf("🌡️ MAX31865: %s\n", grillSensor.isInitialized() ? "OK" : "ERROR");
  
  // WiFi status
  Serial.printf("📶 WiFi: %s", wifiManager.getStatusString().c_str());
  if (wifiManager.isConnected()) {
    Serial.printf(" (%s)", wifiManager.getIP().toString().c_str());
  }
  Serial.println();
  
  // Pellet control parameters
  Serial.printf("🌾 Pellet Feed: Initial=%lums, Lighting=%lums, Normal Cycle=%lums\n",
                pellet_get_initial_feed_duration(),
                pellet_get_lighting_feed_duration(), 
                pellet_get_normal_feed_duration());
  
  Serial.println("------------------------------\n");
}

void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    // Handle calibration and temperature commands
    if (command.startsWith("max_") || command.startsWith("cal_")) {
      handleCalibrationCommands(command);
      return;
    }
    
    // Debug and system commands
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
    } else if (command == "health") {
      Serial.printf("System Health: %s\n", systemHealthy ? "HEALTHY" : "ISSUES DETECTED");
      Serial.printf("Free Memory: %d bytes\n", ESP.getFreeHeap());
      Serial.printf("Stack Remaining: %d bytes\n", uxTaskGetStackHighWaterMark(NULL));
      Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
    } else if (command == "relay_status") {
      relay_print_status();
    } else if (command == "pellet_status") {
      pellet_print_diagnostics();
    } else if (command == "reset_reason") {
      printResetReason();
    } else if (command == "debug_on") {
      setAllDebug(true);
    } else if (command == "debug_off") {
      setAllDebug(false);
    } else if (command == "clear_manual") {
      relay_force_clear_manual();
    } else if (command == "emergency_stop") {
      relay_emergency_stop();
      grillRunning = false;
      Serial.println("Emergency stop activated!");
    } else if (command == "restart") {
      Serial.println("Restarting ESP32...");
      delay(1000);
      ESP.restart();
    } else if (command == "help") {
      Serial.println("\n=== AVAILABLE COMMANDS ===");
      Serial.println("TEMPERATURE TESTING:");
      Serial.println("  test_temp       - Test 100Ω resistor reading");
      Serial.println("  test_meat       - Test all meat probes");
      Serial.println("  test_ambient    - Test ambient sensor");
      Serial.println("  diag            - Run temperature diagnostics");
      Serial.println("");
      Serial.println("SYSTEM MONITORING:");
      Serial.println("  status          - Print detailed system status");
      Serial.println("  health          - Show system health summary");
      Serial.println("  relay_status    - Show relay control status");
      Serial.println("  pellet_status   - Show pellet control diagnostics");
      Serial.println("  reset_reason    - Show last reset reason");
      Serial.println("");
      Serial.println("DEBUG CONTROL:");
      Serial.println("  debug_on/off    - Toggle debug output");
      Serial.println("  clear_manual    - Force clear manual relay override");
      Serial.println("  emergency_stop  - Emergency stop all operations");
      Serial.println("  restart         - Restart ESP32");
      Serial.println("");
      Serial.println("HELP:");
      Serial.println("  help            - Show this help message");
      Serial.println("=========================\n");
    } else if (command.length() > 0) {
      Serial.println("Unknown command. Type 'help' for available commands.");
    }
  }
}