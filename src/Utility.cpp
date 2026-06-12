#include "Utility.h"
#include "Globals.h"
#include "Ignition.h"
#include "MAX31865Sensor.h"

bool debugGrillSensor = false;
bool debugAmbientSensor = false;
bool debugMeatProbes = false;
bool debugRelays = false;
bool debugSystem = false;

static double cachedGrillTemp = NAN;
static unsigned long lastReadAttemptMs = 0;
static unsigned long lastGoodReadMs = 0;
static uint8_t badReadCount = 0;

bool isValidTemperature(double temp) {
  return !isnan(temp) && !isinf(temp) && temp > -100.0 && temp < 1000.0;
}

bool grillTemperatureHealthy() {
  if (!isValidTemperature(cachedGrillTemp)) return false;
  return millis() - lastGoodReadMs < 10000;
}

unsigned long grillTemperatureAgeMs() {
  return lastGoodReadMs == 0 ? ULONG_MAX : millis() - lastGoodReadMs;
}

double readGrillTemperature() {
  unsigned long now = millis();
  if (now - lastReadAttemptMs < 1000 && isValidTemperature(cachedGrillTemp)) {
    return cachedGrillTemp;
  }
  lastReadAttemptMs = now;

  double temp = grillSensor.readTemperatureF();
  if (isValidTemperature(temp)) {
    cachedGrillTemp = temp;
    lastGoodReadMs = now;
    badReadCount = 0;
    return cachedGrillTemp;
  }

  if (badReadCount < 255) badReadCount++;
  if (debugGrillSensor || badReadCount == 3) {
    Serial.printf("Grill temp read failed; bad=%u age=%lu ms\n",
                  badReadCount, grillTemperatureAgeMs());
  }

  if (grillTemperatureHealthy()) return cachedGrillTemp;
  return NAN;
}

double readTemperature() {
  return readGrillTemperature();
}

double readAmbientTemperature() {
  int raw = analogRead(AMBIENT_TEMP_PIN);
  if (raw <= 0 || raw >= 4095) return NAN;
  return NAN;
}

String getStatus(double temp) {
  if (!grillRunning) return "IDLE";
  if (!isValidTemperature(temp)) return "SENSOR ERROR";
  return ignition_get_status_string();
}

void setGrillDebug(bool enabled) { debugGrillSensor = enabled; }
void setAmbientDebug(bool enabled) { debugAmbientSensor = enabled; }
void setMeatProbesDebug(bool enabled) { debugMeatProbes = enabled; }
void setRelayDebug(bool enabled) { debugRelays = enabled; }
void setSystemDebug(bool enabled) { debugSystem = enabled; }
void setAllDebug(bool enabled) {
  debugGrillSensor = enabled;
  debugAmbientSensor = enabled;
  debugMeatProbes = enabled;
  debugRelays = enabled;
  debugSystem = enabled;
}

bool getGrillDebug() { return debugGrillSensor; }
bool getAmbientDebug() { return debugAmbientSensor; }
bool getMeatProbesDebug() { return debugMeatProbes; }
bool getRelayDebug() { return debugRelays; }
bool getSystemDebug() { return debugSystem; }

void setupTemperatureCalibration() {}
void printCalibrationStatus() {}
void handleCalibrationCommands(String command) { (void)command; }
void runTemperatureDiagnostics() {}
void testGrillSensor() {}
void resetCalibration() {}
void saveCalibrationData() {}
void loadCalibrationData() {}
void printTemperatureDiagnostics() {}
void debugTemperatureLoop() {}
void testAmbientNTC() {}
void testSpecificProbe() {}
void testAmbientSensor() {}
void setTemperatureDebugMode(bool enabled) { debugGrillSensor = enabled; }
bool isDebugModeEnabled() { return debugGrillSensor; }
