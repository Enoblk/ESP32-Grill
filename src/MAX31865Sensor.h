// MAX31865Sensor.h - Minimal version for 100Ω resistor temperature reading
#ifndef MAX31865_SENSOR_H
#define MAX31865_SENSOR_H

#include <Arduino.h>
#include <SPI.h>

class MAX31865Sensor {
private:
  bool initialized;
  uint8_t csPin;
  float rref;
  float rnominal;
  
  uint8_t readRegister8(uint8_t reg);
  uint16_t readRegister16(uint8_t reg);
  bool writeRegister(uint8_t reg, uint8_t data);
  
public:
  MAX31865Sensor();
  
  bool begin(uint8_t cs_pin, float ref_resistor = 430.0, float nominal_resistor = 100.0);
  float readTemperatureF();
  float readRTD();
  bool isInitialized() { return initialized; }
  void setDebug(bool enable) {} // Dummy for compatibility
};

extern MAX31865Sensor grillSensor;

#endif