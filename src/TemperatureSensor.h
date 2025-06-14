// TemperatureSensor.h - Updated for 5V System with Correct Constants
#ifndef TEMPERATURESENSOR_H
#define TEMPERATURESENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

// Probe configuration
#define MAX_PROBES 4
#define ADS1115_ADDRESS 0x48  // Default I2C address

// Probe types for 1kΩ NTC thermistors
enum ProbeType {
  PROBE_DISABLED,
  PROBE_FOOD_1,         // Meat probe 1 (1kΩ NTC)
  PROBE_FOOD_2,         // Meat probe 2 (1kΩ NTC)
  PROBE_FOOD_3,         // Meat probe 3 (1kΩ NTC)
  PROBE_FOOD_4          // Meat probe 4 (1kΩ NTC)
};

// Probe configuration structure
struct ProbeConfig {
  ProbeType type;
  String name;
  bool enabled;
  float offset;         // Temperature offset calibration
  float minTemp;        // Minimum valid temperature
  float maxTemp;        // Maximum valid temperature
  uint32_t lastUpdate;  // Last update timestamp
  float lastValidTemp;  // Last valid reading
  bool isValid;         // Current reading validity
};

class TemperatureSensor {
private:
  bool initialized;
  
  // CORRECTED Constants for 1kΩ NTC thermistors with 10kΩ built-in pullups and 5V system
  static constexpr float THERMISTOR_NOMINAL = 1000.0;   // 1k ohm at 25°C
  static constexpr float TEMPERATURE_NOMINAL = 25.0;    // 25°C
  static constexpr float B_COEFFICIENT = 3435.0;        // NTC beta coefficient
  static constexpr float SERIES_RESISTOR = 10000.0;     // 10k built-in pullup (NOT 1k series)
  static constexpr float SUPPLY_VOLTAGE = 5.0;          // 5V reference voltage (was 3.3V)
  
  float calculateTemperature(int16_t adcValue);
  bool validateTemperature(float temp, int probeIndex);
  
public:
  Adafruit_ADS1115 ads;  // Public for direct access
  ProbeConfig probes[MAX_PROBES];  // Public for diagnostics
  
  TemperatureSensor();
  bool begin();
  void configureProbe(int probeIndex, ProbeType type, String name, float offset = 0.0);
  void disableProbe(int probeIndex);
  
  // Temperature reading functions
  float readProbe(int probeIndex);
  float getFoodTemperature(int foodProbe);  // Returns food probe temp (1, 2, 3, or 4)
  
  // Status and diagnostics
  bool isProbeValid(int probeIndex);
  String getProbeName(int probeIndex);
  ProbeType getProbeType(int probeIndex);
  void printDiagnostics();
  void calibrateProbe(int probeIndex, float actualTemp);
  
  // Testing functions
  void testProbe(int probeIndex);           // Test specific probe
  void testBetaCoefficients(int probeIndex); // Try different beta values
  
  // Update all probes (call regularly)
  void updateAll();
  
  // Get probe data for web interface
  String getProbeDataJSON();
};

extern TemperatureSensor tempSensor;

#endif