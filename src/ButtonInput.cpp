#include "ButtonInput.h"
#include "Globals.h"
#include "PelletControl.h"
#include "DisplayUI.h" // <-- for OLED feedback
#include "Utility.h"
#include "Ignition.h"


static unsigned long lastButtonTime = 0;
const unsigned long BUTTON_DEBOUNCE = 200;

// For hold-to-start logic
static bool up_hold_active = false;
static unsigned long up_pressed_at = 0;

void button_init() {
  pinMode(BUTTON_UP_PIN,     INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN,   INPUT_PULLUP);
  pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP);
}

void handle_buttons() {
  unsigned long now = millis();

  // --- HOLD LEFT/UP BUTTON TO START ---
  if (!grillRunning && digitalRead(BUTTON_UP_PIN) == LOW) {
    if (!up_hold_active) {
      up_hold_active = true;
      up_pressed_at = now;
    }
    unsigned long held_ms = now - up_pressed_at;
    // Show hold feedback every 250ms
    if (held_ms > 250) {
      display_hold_feedback(held_ms / 1000);
    }
    // If held for 3+ seconds, start grill
    if (held_ms >= 3000) {
      grillRunning = true;
      ignition_start(readTemperature());  // <-- This triggers the proper ignition phase!
      display_grill_starting();
      up_hold_active = false;
  }
  
    return; // Don't process other buttons while holding to start
  } else if (up_hold_active && digitalRead(BUTTON_UP_PIN) == HIGH) {
    up_hold_active = false;
    display_clear_hold_feedback();
  }

  // --- UP/DOWN ADJUSTMENTS (when grill is running) ---
  if (now - lastButtonTime > BUTTON_DEBOUNCE) {
    if (grillRunning && digitalRead(BUTTON_UP_PIN) == LOW) {
      setpoint += 5;
      save_setpoint();
      lastButtonTime = now;
    } else if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
      setpoint -= 5;
      save_setpoint();
      lastButtonTime = now;
    }
  }
}
