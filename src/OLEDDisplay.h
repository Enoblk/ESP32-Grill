// OLEDDisplay.h - OLED Display Management System
#ifndef OLEDDISPLAY_H
#define OLEDDISPLAY_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Display pages
enum DisplayPage {
  PAGE_MAIN,        // Temperature and basic status
  PAGE_GRILL,       // Grill status and ignition
  PAGE_WIFI,        // WiFi information
  PAGE_RELAYS,      // Relay status
  PAGE_DEBUG        // System debug info
};

// Display manager class
class OLEDDisplayManager {
private:
  Adafruit_SSD1306 display;
  DisplayPage currentPage;
  unsigned long lastUpdate;
  unsigned long pageStartTime;
  bool displayConnected;
  bool autoRotate;
  unsigned long autoRotateInterval;
  
  // Drawing functions for each page
  void drawMainPage();
  void drawGrillPage();
  void drawWiFiPage();
  void drawRelaysPage();
  void drawDebugPage();
  
  // Helper functions
  void drawHeader(String title);
  void drawWiFiIcon(int x, int y, bool connected);
  void drawTemperatureIcon(int x, int y);
  void drawProgressBar(int x, int y, int width, int height, int percent);
  String formatTime(unsigned long seconds);
  void drawCenteredText(String text, int y);
  
public:
  OLEDDisplayManager();
  
  // Initialization and control
  bool begin();
  void update();
  void clear();
  void setBrightness(uint8_t brightness);
  
  // Page management
  void setPage(DisplayPage page);
  void nextPage();
  void previousPage();
  DisplayPage getCurrentPage();
  
  // Auto-rotation
  void enableAutoRotate(bool enable = true, unsigned long interval = 5000);
  
  // Status display
  void showBootScreen();
  void showError(String error);
  void showMessage(String message, unsigned long duration = 3000);
  
  // Status check
  bool isConnected() { return displayConnected; }
};

extern OLEDDisplayManager oledDisplay;

#endif