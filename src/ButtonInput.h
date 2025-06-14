// ButtonInput.h - Updated for GPIO13
#ifndef BUTTON_INPUT_H
#define BUTTON_INPUT_H

#include <Arduino.h>

// Button pin definitions - Updated for your relay pins
#define BUTTON_UP_PIN     32  // GPIO32 - UP button
#define BUTTON_DOWN_PIN   33  // GPIO33 - DOWN button
//#define BUTTON_SELECT_PIN 13  // GPIO13 - SELECT button (available pin)

extern double setpoint;

void button_init();
void handle_buttons();

#endif // BUTTON_INPUT_H