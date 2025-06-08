#include "Utility.h"
#include "Globals.h"
#include <Arduino.h>

double readTemperature() {
  int raw = analogRead(35); // Or use THERM_PIN if you prefer
  return map(raw, 0, 4095, 50, 550);
}

String getStatus(double temp) {
  if (!grillRunning)        return "IDLE";
  if (igniting)             return "IGNITING";
  if (temp < setpoint - 10) return "HEATING";
  if (abs(temp - setpoint) < 5) return "AT TEMP";
  if (temp > setpoint + 10) return "HOT";
  return "STABLE";
}

String relayColor(bool on) {
  if (!grillRunning) return "gray";
  return on ? "green" : "red";
}
