// Main.ino - REPLACE ENTIRE FILE - Remove conflicting includes
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <SPI.h>

// Keep essential includes only
#include "Globals.h"
#include "Utility.h"
#include "MAX31865Sensor.h"
#include "Ignition.h"
#include "GrillWebServer.h"

// Comment out the conflicting includes
// #include "RelayControl.h"         // CONFLICTS with PelletControl.cpp
// #include "PelletControl.h"        // CONFLICTS with RelayControl.cpp
// #include "ButtonInput.h"          
// #include "OLEDDisplay.h"          
// #include "WiFiManager.h"          
// #include "TemperatureSensor.h"    
// #include "Settings.h"             

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== SIMPLIFIED GRILL CONTROLLER v1.1 ===");
  Serial.printf("Build: %s %s\n", __DATE__, __TIME__);
  
  // Initialize I2C and SPI
  Wire.begin(SDA_PIN, SCL_PIN);
  SPI.begin();
    
  // Initialize ignition system (now handles all control including relays)
  ignition_init();
  
  // Initialize MAX31865 RTD sensor - original approach
  Serial.println("Initializing MAX31865 RTD sensor...");
  if (!grillSensor.begin(MAX31865_CS_PIN, RREF, RNOMINAL)) {
    Serial.println("MAX31865 sensor failed to initialize");
  } else {
    Serial.println("MAX31865 sensor initialized successfully");
  }
  
  // Load settings
  load_setpoint();
  
  // Load and connect to WiFi if credentials exist
  preferences.begin("wifi", true);
  String savedSSID = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");
  preferences.end();
  
  if (savedSSID.length() > 0) {
    Serial.println("Found saved WiFi credentials, attempting to connect...");
    Serial.print("SSID: ");
    Serial.println(savedSSID);
    
    WiFi.mode(WIFI_AP_STA);  // Both AP and Station mode
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    
    // Wait up to 15 seconds for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println("WiFi connected successfully!");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println();
      Serial.println("WiFi connection failed, staying in AP mode");
    }
  } else {
    Serial.println("No saved WiFi credentials found");
  }
  
  // Always ensure AP mode is active for initial setup
  if (!WiFi.softAPgetStationNum()) {  // If no stations connected to AP
    WiFi.softAP("GrillController", "12345678");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  }
  
  // Setup web server
  setup_grill_server();
  
  Serial.println("Setup complete - simplified mode");
}

void loop() {
  static unsigned long lastTempUpdate = 0;
  static unsigned long lastStatus = 0;
  
  unsigned long now = millis();
  
  // Temperature updates every 2 seconds
  if (now - lastTempUpdate >= 2000) {
    // Temperature reading is handled in ignition_loop()
    lastTempUpdate = now;
  }
  
  // Run simplified ignition control (handles everything now)
  ignition_loop();
  
  // Status every 60 seconds (minimal output)
  if (now - lastStatus >= 60000) {
    double temp = readGrillTemperature();
    Serial.printf("%s: %.1fF -> %.1fF, Heap: %d\n", 
                  ignition_get_status_string().c_str(), temp, setpoint, ESP.getFreeHeap());
    lastStatus = now;
  }
  
  delay(100);
}