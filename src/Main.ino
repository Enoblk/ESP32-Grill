// ESP-Grill.ino - Minimal working Arduino .ino for ESP32 modular grill project
#include "Globals.h"
#include "Utility.h"
#include "Settings.h"
#include "PelletControl.h"
#include "Ignition.h"
#include "ButtonInput.h"
#include "DisplayUI.h"
#include "GrillWebServer.h"
#include <Wire.h>

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN); // Always before display
    display_init();
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    init_relay_pins();
    button_init();
    load_settings();
    setup_wifi_and_mdns();
    setup_grill_server();
    restore_setpoint();
    display_update();
    Serial.println("Setup complete!");
}

void loop() {
    ignition_loop();
    if (!ignition_active()) {
        pellet_feed_loop();
    }

    // FAN COOLDOWN LOGIC
    if (!grillRunning && readTemperature() > 100) {
        digitalWrite(RELAY_HOPPER_FAN_PIN, HIGH);
    } else if (!grillRunning) {
        digitalWrite(RELAY_HOPPER_FAN_PIN, LOW);
    }

    handle_buttons();
    display_update();
    delay(50);
}
