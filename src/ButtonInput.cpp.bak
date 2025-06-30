// ButtonInput.cpp - No SELECT button, UP/DOWN only
#include "ButtonInput.h"
#include "Globals.h"
#include "PelletControl.h"
#include "Utility.h"
#include "Ignition.h"

void button_init() {
  // Initialize only UP and DOWN buttons (SELECT button disabled)
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  
  Serial.println("Button system initialized (UP/DOWN only - SELECT button disabled)");
}

void handle_buttons() {
  static unsigned long lastButtonTime = 0;
  const unsigned long BUTTON_DEBOUNCE = 200;
  static bool up_hold_active = false;
  static unsigned long up_pressed_at = 0;
  
  unsigned long now = millis();

  // --- HOLD UP BUTTON TO START ---
  if (!grillRunning && digitalRead(BUTTON_UP_PIN) == LOW) {
    if (!up_hold_active) {
      up_hold_active = true;
      up_pressed_at = now;
      Serial.println("Hold UP button to start grill");
    }
    unsigned long held_ms = now - up_pressed_at;
    
    // Show progress in serial every second
    if (held_ms > 1000 && (held_ms % 1000) < 100) {
      Serial.printf("Hold to start: %lu seconds (need 3)\n", held_ms / 1000);
    }
    
    // If held for 3+ seconds, start grill
    if (held_ms >= 3000) {
      grillRunning = true;
      ignition_start(readGrillTemperature());
      Serial.println("Grill starting - ignition sequence initiated!");
      up_hold_active = false;
    }
    
    return; // Don't process other buttons while holding to start
  } else if (up_hold_active && digitalRead(BUTTON_UP_PIN) == HIGH) {
    up_hold_active = false;
    Serial.println("Hold to start cancelled");
  }

  // --- UP/DOWN TEMPERATURE ADJUSTMENTS ---
  if (now - lastButtonTime > BUTTON_DEBOUNCE) {
    if (digitalRead(BUTTON_UP_PIN) == LOW) {
      if (grillRunning) {
        setpoint += 5;
        if (setpoint > MAX_SETPOINT) setpoint = MAX_SETPOINT;
        save_setpoint();
        Serial.printf("Temperature increased to %.0f°F\n", setpoint);
      }
      lastButtonTime = now;
      
    } else if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
      setpoint -= 5;
      if (setpoint < MIN_SETPOINT) setpoint = MIN_SETPOINT;
      save_setpoint();
      Serial.printf("Temperature decreased to %.0f°F\n", setpoint);
      lastButtonTime = now;
    }
  }
}