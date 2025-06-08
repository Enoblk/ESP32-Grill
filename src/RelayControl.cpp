#include "RelayControl.h"
#include "Globals.h"
#include <Arduino.h>

// Internal state
static RelayRequest currentAutoRequest = {RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE};
static RelayRequest manualOverride = {RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE};
static bool manualMode = false;

// Call this at boot (if you want to set all relays LOW at power-on)
void relay_control_init() {
    pinMode(RELAY_IGNITER_PIN, OUTPUT);
    pinMode(RELAY_AUGER_PIN, OUTPUT);
    pinMode(RELAY_HOPPER_FAN_PIN, OUTPUT);
    pinMode(RELAY_BLOWER_FAN_PIN, OUTPUT);
    digitalWrite(RELAY_IGNITER_PIN, LOW);
    digitalWrite(RELAY_AUGER_PIN, LOW);
    digitalWrite(RELAY_HOPPER_FAN_PIN, LOW);
    digitalWrite(RELAY_BLOWER_FAN_PIN, LOW);
}

void relay_request_auto(const RelayRequest* req) {
    currentAutoRequest = *req;
}

void relay_request_manual(const RelayRequest* req) {
    manualOverride = *req;
    manualMode = true;
}

void relay_clear_manual(void) {
    manualMode = false;
}

void relay_commit(void) {
    const RelayRequest* req = manualMode ? &manualOverride : &currentAutoRequest;
    if (req->igniter != RELAY_NOCHANGE)
        digitalWrite(RELAY_IGNITER_PIN, req->igniter == RELAY_ON ? HIGH : LOW);
    if (req->auger != RELAY_NOCHANGE)
        digitalWrite(RELAY_AUGER_PIN, req->auger == RELAY_ON ? HIGH : LOW);
    if (req->hopperFan != RELAY_NOCHANGE)
        digitalWrite(RELAY_HOPPER_FAN_PIN, req->hopperFan == RELAY_ON ? HIGH : LOW);
    if (req->blowerFan != RELAY_NOCHANGE)
        digitalWrite(RELAY_BLOWER_FAN_PIN, req->blowerFan == RELAY_ON ? HIGH : LOW);
}
