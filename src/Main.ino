#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>

#include "Globals.h"
#include "GrillWebServer.h"
#include "Ignition.h"
#include "MAX31865Sensor.h"
#include "Utility.h"

static void setupWiFi() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.softAP("GrillController", "12345678");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  if (!preferences.begin("wifi", true)) {
    Serial.println("WiFi preferences unavailable");
    return;
  }
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  preferences.end();

  if (ssid.isEmpty()) {
    Serial.println("No saved WiFi credentials");
    return;
  }

  Serial.printf("Connecting to WiFi SSID: %s\n", ssid.c_str());
  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(100);
    yield();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connect timed out; AP remains available");
  }
}

void setup() {
  Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT
  unsigned long serialStart = millis();
  while (!Serial && millis() - serialStart < 1500) {
    delay(10);
  }
#endif
  delay(200);
  Serial.println();
  Serial.println("=== ESP32 Grill Controller stable build ===");
  Serial.printf("Board: %s\n", GRILL_BOARD_NAME);
  Serial.printf("Build: %s %s\n", __DATE__, __TIME__);

  Wire.begin(SDA_PIN, SCL_PIN);
  SPI.begin();

  ignition_init();
  load_setpoint();

  if (!grillSensor.begin(MAX31865_CS_PIN, RREF, RNOMINAL)) {
    Serial.println("MAX31865 did not pass initial read; controller will refuse starts until it reads cleanly");
  }

  setupWiFi();
  setup_grill_server();
  Serial.println("Setup complete");
}

void loop() {
  handle_grill_server();
  ignition_loop();

  static unsigned long lastStatus = 0;
  unsigned long now = millis();
  if (now - lastStatus >= 60000) {
    double temp = readGrillTemperature();
    Serial.printf("%s temp=%s target=%.1fF heap=%u\n",
                  ignition_get_status_string().c_str(),
                  isValidTemperature(temp) ? String(temp, 1).c_str() : "ERR",
                  setpoint,
                  ESP.getFreeHeap());
    lastStatus = now;
  }

  delay(20);
}
