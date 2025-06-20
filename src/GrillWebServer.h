// =========================
// GrillWebServer.h
// HTTP/Web UI and Troubleshooting for ESP32 Grill Controller
// =========================
#ifndef GRILL_WEBSERVER_H
#define GRILL_WEBSERVER_H

#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;
extern double setpoint;
extern bool grillRunning;
extern bool igniting;

void setup_grill_server();

#endif // GRILL_WEBSERVER_H