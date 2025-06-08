// =========================
// DisplayUI.h
// OLED Display abstraction for ESP32 Grill Controller
// =========================
#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define SDA_PIN        21
#define SCL_PIN        22

extern Adafruit_SSD1306 display;
extern double setpoint;
extern bool grillRunning;
extern bool igniting;

void display_init();
void display_update();
void display_hold_feedback(unsigned long seconds);
void display_clear_hold_feedback();
void display_grill_starting();


#endif // DISPLAY_UI_H
