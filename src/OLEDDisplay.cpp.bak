// OLEDDisplay.cpp - Fixed Implementation with proper includes
#include "OLEDDisplay.h"
#include "Globals.h"
#include "Utility.h"
#include "WiFiManager.h"
#include "Ignition.h"
#include "RelayControl.h"  // Added this include for relay_is_safe_state()
#include <WiFi.h>

OLEDDisplayManager oledDisplay;

OLEDDisplayManager::OLEDDisplayManager() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {
  currentPage = PAGE_MAIN;
  lastUpdate = 0;
  pageStartTime = 0;
  displayConnected = false;
  autoRotate = true;
  autoRotateInterval = 5000; // 5 seconds per page
}

bool OLEDDisplayManager::begin() {
  Serial.println("Initializing OLED display...");
  
  // Try to initialize the display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("⚠️  OLED display not found - continuing without display");
    displayConnected = false;
    return false;
  }
  
  displayConnected = true;
  Serial.println("OLED display initialized successfully");
  
  // Initial setup
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  
  // Show boot screen
  showBootScreen();
  
  return true;
}

void OLEDDisplayManager::showBootScreen() {
  if (!displayConnected) return;
  
  display.clearDisplay();
  
  // Title
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.println("GRILL");
  display.println("MASTER");
  
  // Version info
  display.setTextSize(1);
  display.setCursor(0, 45);
  display.println("ESP32 Controller");
  display.setCursor(0, 55);
  display.println("Initializing...");
  
  display.display();
  delay(2000);
}

void OLEDDisplayManager::update() {
  if (!displayConnected) return;
  
  unsigned long now = millis();
  
  // Auto-rotate pages if enabled
  if (autoRotate && now - pageStartTime > autoRotateInterval) {
    nextPage();
    pageStartTime = now;
  }
  
  // Update display every 500ms
  if (now - lastUpdate < 500) return;
  lastUpdate = now;
  
  display.clearDisplay();
  
  // Draw current page
  switch (currentPage) {
    case PAGE_MAIN:
      drawMainPage();
      break;
    case PAGE_GRILL:
      drawGrillPage();
      break;
    case PAGE_WIFI:
      drawWiFiPage();
      break;
    case PAGE_RELAYS:
      drawRelaysPage();
      break;
    case PAGE_DEBUG:
      drawDebugPage();
      break;
  }
  
  display.display();
}

void OLEDDisplayManager::drawMainPage() {
  drawHeader("TEMPERATURE");
  
  // Large temperature display
  double temp = readTemperature();
  display.setTextSize(3);
  display.setCursor(10, 20);
  if (temp == 75.0) {
    display.println("--.-");
    display.setTextSize(1);
    display.setCursor(10, 50);
    display.println("NO PROBE");
  } else {
    display.printf("%.0f", temp);
    display.setTextSize(1);
    display.setCursor(90, 25);
    display.println("F");
  }
  
  // Target temperature
  display.setTextSize(1);
  display.setCursor(10, 50);
  display.printf("Target: %.0fF", setpoint);
  
  // Status indicator
  display.setCursor(0, 57);
  if (grillRunning) {
    display.println("RUNNING");
  } else {
    display.println("IDLE");
  }
  
  // WiFi status icon
  drawWiFiIcon(115, 57, wifiManager.isConnected());
}

void OLEDDisplayManager::drawGrillPage() {
  drawHeader("GRILL STATUS");
  
  display.setTextSize(1);
  display.setCursor(0, 15);
  
  if (grillRunning) {
    // Ignition status
    String ignitionStatus = ignition_get_status_string();
    display.printf("Ignition: %s\n", ignitionStatus.c_str());
    
    // Temperature error
    double temp = readTemperature();
    double error = setpoint - temp;
    display.printf("Temp Error: %.1fF\n", error);
    
    // Running time (simplified)
    display.printf("Running: %s\n", formatTime(millis() / 1000).c_str());
    
    // Progress bar for ignition (if active)
    if (ignition_get_state() != IGNITION_OFF && ignition_get_state() != IGNITION_COMPLETE) {
      int progress = 50; // Simplified progress indicator
      drawProgressBar(0, 50, 128, 8, progress);
      display.setCursor(0, 60);
      display.println("Ignition Progress");
    }
  } else {
    display.println("Grill is OFF");
    display.println("");
    display.println("Press START to begin");
    display.println("ignition sequence");
  }
}

void OLEDDisplayManager::drawWiFiPage() {
  drawHeader("WIFI STATUS");
  
  display.setTextSize(1);
  display.setCursor(0, 15);
  
  if (wifiManager.isConnected()) {
    display.printf("SSID: %s\n", wifiManager.getSSID().c_str());
    display.printf("IP: %s\n", wifiManager.getIP().toString().c_str());
    display.printf("RSSI: %d dBm\n", WiFi.RSSI());
    
    // Signal strength bar
    int rssi = WiFi.RSSI();
    int bars = map(constrain(rssi, -100, -30), -100, -30, 0, 4);
    display.setCursor(0, 50);
    display.print("Signal: ");
    for (int i = 0; i < 4; i++) {
      if (i < bars) {
        display.print("█");
      } else {
        display.print("░");
      }
    }
  } else if (wifiManager.getStatus() == GRILL_WIFI_AP_MODE) {
    display.println("AP MODE ACTIVE");
    display.println("");
    uint8_t mac[6];
    WiFi.macAddress(mac);
    display.printf("SSID: GrillCtrl-%02X%02X\n", mac[4], mac[5]);
    display.printf("Pass: grillpass123\n");
    display.printf("IP: %s\n", wifiManager.getIP().toString().c_str());
  } else {
    display.println("DISCONNECTED");
    display.println("");
    display.println("Check WiFi settings");
    display.println("via web interface");
  }
}

void OLEDDisplayManager::drawRelaysPage() {
  drawHeader("RELAY STATUS");
  
  display.setTextSize(1);
  display.setCursor(0, 15);
  
  // Relay status with indicators
  display.printf("Igniter:   %s\n", digitalRead(RELAY_IGNITER_PIN) ? "ON " : "OFF");
  display.printf("Auger:     %s\n", digitalRead(RELAY_AUGER_PIN) ? "ON " : "OFF");
  display.printf("Hopper:    %s\n", digitalRead(RELAY_HOPPER_FAN_PIN) ? "ON " : "OFF");
  display.printf("Blower:    %s\n", digitalRead(RELAY_BLOWER_FAN_PIN) ? "ON " : "OFF");
  
  // Safety status
  display.setCursor(0, 57);
  if (relay_is_safe_state()) {
    display.println("SAFE");
  } else {
    display.println("WARNING");
  }
}

void OLEDDisplayManager::drawDebugPage() {
  drawHeader("DEBUG INFO");
  
  display.setTextSize(1);
  display.setCursor(0, 15);
  
  // System info
  display.printf("Uptime: %s\n", formatTime(millis() / 1000).c_str());
  display.printf("Free RAM: %d\n", ESP.getFreeHeap());
  display.printf("CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());
  
  // Sensor status
  double temp = readTemperature();
  if (temp == 75.0) {
    display.println("Temp: NO PROBE");
  } else {
    display.printf("Temp: %.1fF\n", temp);
  }
  
  // Loop timing
  static unsigned long lastLoop = 0;
  unsigned long loopTime = millis() - lastLoop;
  lastLoop = millis();
  display.printf("Loop: %lu ms", loopTime);
}

void OLEDDisplayManager::drawHeader(String title) {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(title);
  
  // Draw separator line
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
}

void OLEDDisplayManager::drawWiFiIcon(int x, int y, bool connected) {
  if (connected) {
    // Connected WiFi icon
    display.drawPixel(x+2, y+4, SSD1306_WHITE);
    display.drawLine(x+1, y+3, x+3, y+3, SSD1306_WHITE);
    display.drawLine(x, y+2, x+4, y+2, SSD1306_WHITE);
  } else {
    // Disconnected WiFi icon (X)
    display.drawLine(x, y, x+4, y+4, SSD1306_WHITE);
    display.drawLine(x+4, y, x, y+4, SSD1306_WHITE);
  }
}

void OLEDDisplayManager::drawProgressBar(int x, int y, int width, int height, int percent) {
  // Draw border
  display.drawRect(x, y, width, height, SSD1306_WHITE);
  
  // Draw fill
  int fillWidth = (width - 2) * percent / 100;
  display.fillRect(x + 1, y + 1, fillWidth, height - 2, SSD1306_WHITE);
}

String OLEDDisplayManager::formatTime(unsigned long seconds) {
  unsigned long hours = seconds / 3600;
  unsigned long minutes = (seconds % 3600) / 60;
  unsigned long secs = seconds % 60;
  
  if (hours > 0) {
    return String(hours) + "h " + String(minutes) + "m";
  } else if (minutes > 0) {
    return String(minutes) + "m " + String(secs) + "s";
  } else {
    return String(secs) + "s";
  }
}

void OLEDDisplayManager::drawCenteredText(String text, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, y);
  display.print(text);
}

void OLEDDisplayManager::setPage(DisplayPage page) {
  currentPage = page;
  pageStartTime = millis();
}

void OLEDDisplayManager::nextPage() {
  int nextPageNum = (int)currentPage + 1;
  if (nextPageNum > PAGE_DEBUG) {
    nextPageNum = PAGE_MAIN;
  }
  setPage((DisplayPage)nextPageNum);
}

void OLEDDisplayManager::previousPage() {
  int prevPageNum = (int)currentPage - 1;
  if (prevPageNum < PAGE_MAIN) {
    prevPageNum = PAGE_DEBUG;
  }
  setPage((DisplayPage)prevPageNum);
}

DisplayPage OLEDDisplayManager::getCurrentPage() {
  return currentPage;
}

void OLEDDisplayManager::enableAutoRotate(bool enable, unsigned long interval) {
  autoRotate = enable;
  autoRotateInterval = interval;
  if (enable) {
    pageStartTime = millis();
  }
}

void OLEDDisplayManager::showError(String error) {
  if (!displayConnected) return;
  
  display.clearDisplay();
  display.setTextSize(1);
  
  drawCenteredText("ERROR", 20);
  drawCenteredText(error, 35);
  
  display.display();
}

void OLEDDisplayManager::showMessage(String message, unsigned long duration) {
  if (!displayConnected) return;
  
  display.clearDisplay();
  display.setTextSize(1);
  
  drawCenteredText("MESSAGE", 20);
  drawCenteredText(message, 35);
  
  display.display();
  
  if (duration > 0) {
    delay(duration);
  }
}

void OLEDDisplayManager::clear() {
  if (!displayConnected) return;
  display.clearDisplay();
  display.display();
}

void OLEDDisplayManager::setBrightness(uint8_t brightness) {
  if (!displayConnected) return;
  // SSD1306 doesn't have brightness control, but we can simulate it
  // by using contrast settings
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(brightness);
}