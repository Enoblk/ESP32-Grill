// =========================
// ButtonInput.h
// 3-Button UI handling for ESP32 Grill Controller
// =========================
#ifndef BUTTON_INPUT_H
#define BUTTON_INPUT_H

#include <Arduino.h>

#define BUTTON_UP_PIN     32
#define BUTTON_DOWN_PIN   33
#define BUTTON_SELECT_PIN 34

extern double setpoint;

void button_init();
void handle_buttons();

#endif // BUTTON_INPUT_H
