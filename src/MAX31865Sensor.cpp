// MAX31865Sensor.cpp - Minimal version that just reads 100Ω resistor for temperature
#include "MAX31865Sensor.h"

// Global instance
MAX31865Sensor grillSensor;

// Register definitions
#define MAX31865_CONFIG_REG    0x00
#define MAX31865_RTD_MSB_REG   0x01
#define MAX31865_RTD_LSB_REG   0x02

// Config bits
#define MAX31865_CONFIG_BIAS     0x80
#define MAX31865_CONFIG_MODEAUTO 0x40
#define MAX31865_CONFIG_FILT60HZ 0x00

MAX31865Sensor::MAX31865Sensor() {
  initialized = false;
  csPin = 0;
  rref = 430.0;
  rnominal = 100.0;
}

bool MAX31865Sensor::begin(uint8_t cs_pin, float ref_resistor, float nominal_resistor) {
  csPin = cs_pin;
  rref = ref_resistor;
  rnominal = nominal_resistor;
  
  Serial.printf("MAX31865: CS=GPIO%d, RREF=%.0fΩ, Test Resistor=%.0fΩ\n", cs_pin, ref_resistor, nominal_resistor);
  
  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH);
  delay(200);
  
  // Simple config: enable bias and auto mode
  uint8_t config = MAX31865_CONFIG_BIAS | MAX31865_CONFIG_MODEAUTO | MAX31865_CONFIG_FILT60HZ | 0x02; // FAULTCLR

  
  if (writeRegister(MAX31865_CONFIG_REG, config)) {
    Serial.println("MAX31865: Config written successfully");
    delay(200); // Wait for first conversion
    
    // Test read
    float testTemp = readTemperatureF();
    if (testTemp > -100 && testTemp < 500) {
      Serial.printf("MAX31865: Initial reading %.1f°F\n", testTemp);
      initialized = true;
      return true;
    }
  }
  
  Serial.println("MAX31865: Initialization failed");
  return false;
}

uint8_t MAX31865Sensor::readRegister8(uint8_t reg) {
  digitalWrite(csPin, LOW);
  delayMicroseconds(10);
  
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  SPI.transfer(reg);
  uint8_t data = SPI.transfer(0x00);
  SPI.endTransaction();
  
  delayMicroseconds(10);
  digitalWrite(csPin, HIGH);
  
  return data;
}

uint16_t MAX31865Sensor::readRegister16(uint8_t reg) {
  uint8_t msb = readRegister8(reg);
  uint8_t lsb = readRegister8(reg + 1);
  return ((uint16_t)msb << 8) | lsb;
}

bool MAX31865Sensor::writeRegister(uint8_t reg, uint8_t data) {
  digitalWrite(csPin, LOW);
  delayMicroseconds(10);
  
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  SPI.transfer(reg | 0x80);  // Write command
  SPI.transfer(data);
  SPI.endTransaction();
  
  delayMicroseconds(10);
  digitalWrite(csPin, HIGH);
  
  delay(50);
  return true; // Assume success for simplicity
}

float MAX31865Sensor::readRTD() {
  //if (!initialized) return -1.0;
  
  uint16_t rtdValue = readRegister16(MAX31865_RTD_MSB_REG);
  uint16_t rtdData = rtdValue >> 1;  // Remove fault bit
  
  // Convert to resistance
  float resistance = (rtdData * rref) / 32768.0;
  return resistance;
}

float MAX31865Sensor::readTemperatureF() {
  //if (!initialized) return -999.0;
  
  float resistance = readRTD();
  
  if (resistance < 50.0 || resistance > 200.0) {
    return -999.0; // Invalid resistance
  }
  
  // Simple linear conversion for 100Ω resistor
  // Assume resistance changes ~0.39Ω per °C
  // R(T) = R0 * (1 + 0.00385 * T)
  // Where R0 = 100Ω, and 0.00385 is temperature coefficient
  
  float tempC = (resistance - rnominal) / (rnominal * 0.00385);
  float tempF = tempC * 9.0 / 5.0 + 32.0;
  
  return tempF;
}