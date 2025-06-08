// ESP-Grill.ino - Modular version for ESP32 grill project

#include "Globals.h"
#include "Utility.h"
#include "Settings.h"
#include "PelletControl.h"
#include "Ignition.h"
#include "ButtonInput.h"
#include "DisplayUI.h"
#include "GrillWebServer.h"
#include "RelayControl.h"
#include <Wire.h>

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN); // Always before display
    display_init();
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    relay_control_init();
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
    // Use relay manager for cooldown control.
    if (!grillRunning && readTemperature() > 100) {
        RelayRequest req = { RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_ON, RELAY_NOCHANGE }; // hopper fan ON
        relay_request_auto(&req);
    } else if (!grillRunning) {
        RelayRequest req = { RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_OFF, RELAY_NOCHANGE }; // hopper fan OFF
        relay_request_auto(&req);
    }
    // If grillRunning, normal logic applies in PelletControl/Ignition.

    handle_buttons();
    display_update();
    relay_commit(); // Always apply latest requested relay states
    delay(50);
}
