#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

// Use plain enum (old-school C++ style)
typedef enum {
    RELAY_NOCHANGE = -1,
    RELAY_OFF = 0,
    RELAY_ON = 1
} RelayState;

typedef struct {
    RelayState igniter;
    RelayState auger;
    RelayState hopperFan;
    RelayState blowerFan;
} RelayRequest;

// Call this in setup() to initialize relay logic if needed
void relay_control_init();

// Main logic sets what it *wants* the relays to be; do not call digitalWrite elsewhere!
void relay_request_auto(const RelayRequest* req);

// For manual/troubleshooting override; persists until cleared
void relay_request_manual(const RelayRequest* req);

// Actually applies requested relay states (call in loop() **once per loop**)
void relay_commit(void);

// Call to clear manual mode
void relay_clear_manual(void);

#endif // RELAY_CONTROL_H
