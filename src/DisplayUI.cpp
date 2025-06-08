#include "DisplayUI.h"
#include "Globals.h"
#include "Utility.h"
#include <WiFi.h>

void display_init() {
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
}

void display_update() {
  double temp = readTemperature();
  String status = getStatus(temp);
  display.clearDisplay();
  int16_t x1, y1; uint16_t w, h;
  // Title
  display.setTextSize(1);
  const char* title = "TEMPERATURE";
  display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 0);
  display.print(title);
  // Temp/Setpoint
  display.setTextSize(1);
  String ts = String(temp, 1) + " / " + String((int)setpoint);
  display.getTextBounds(ts.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 14);
  display.print(ts);
  // Status
  display.setTextSize(1);
  display.getTextBounds("Status:", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 32);
  display.print("Status:");
  display.getTextBounds(status.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 42);
  display.print(status);
  // IP
  String ip = "IP:" + WiFi.localIP().toString();
  display.getTextBounds(ip.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT - h);
  display.print(ip);
  display.display();
}
void display_hold_feedback(unsigned long seconds) {
  display.fillRect(0, 54, 128, 10, SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(0, 54);
  display.print("Hold LEFT to Start: ");
  display.print(seconds);
  display.print("s");
  display.display();
}

void display_clear_hold_feedback() {
  display.fillRect(0, 54, 128, 10, SSD1306_BLACK);
  display.display();
}

void display_grill_starting() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(8, 28);
  display.print("Starting...");
  display.display();
  delay(1000); // Show for 1 second
}