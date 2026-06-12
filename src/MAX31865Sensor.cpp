#include "MAX31865Sensor.h"

MAX31865Sensor grillSensor;

#define MAX31865_CONFIG_REG    0x00
#define MAX31865_RTD_MSB_REG   0x01
#define MAX31865_FAULT_REG     0x07

#define MAX31865_CONFIG_BIAS     0x80
#define MAX31865_CONFIG_MODEAUTO 0x40
#define MAX31865_CONFIG_3WIRE    0x10
#define MAX31865_CONFIG_FAULTCLR 0x02
#define MAX31865_CONFIG_FILT60HZ 0x00

MAX31865Sensor::MAX31865Sensor()
    : initialized(false), csPin(0), rref(430.0), rnominal(100.0) {}

bool MAX31865Sensor::begin(uint8_t cs_pin, float ref_resistor, float nominal_resistor) {
  csPin = cs_pin;
  rref = ref_resistor;
  rnominal = nominal_resistor;

  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH);
  delay(20);

  clearFault();
  uint8_t config = MAX31865_CONFIG_BIAS | MAX31865_CONFIG_MODEAUTO |
                   MAX31865_CONFIG_3WIRE | MAX31865_CONFIG_FILT60HZ;
  writeRegister(MAX31865_CONFIG_REG, config);
  delay(250);

  float temp = readTemperatureF();
  initialized = temp > -100.0 && temp < 1000.0;
  Serial.printf("MAX31865: CS=GPIO%d, initial=%.1f F, initialized=%s\n",
                csPin, temp, initialized ? "yes" : "no");
  return initialized;
}

uint8_t MAX31865Sensor::readRegister8(uint8_t reg) {
  uint8_t data;
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  digitalWrite(csPin, LOW);
  delayMicroseconds(2);
  SPI.transfer(reg & 0x7F);
  data = SPI.transfer(0x00);
  digitalWrite(csPin, HIGH);
  SPI.endTransaction();
  return data;
}

uint16_t MAX31865Sensor::readRegister16(uint8_t reg) {
  uint16_t data;
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  digitalWrite(csPin, LOW);
  delayMicroseconds(2);
  SPI.transfer(reg & 0x7F);
  data = (uint16_t)SPI.transfer(0x00) << 8;
  data |= SPI.transfer(0x00);
  digitalWrite(csPin, HIGH);
  SPI.endTransaction();
  return data;
}

bool MAX31865Sensor::writeRegister(uint8_t reg, uint8_t data) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  digitalWrite(csPin, LOW);
  delayMicroseconds(2);
  SPI.transfer(reg | 0x80);
  SPI.transfer(data);
  digitalWrite(csPin, HIGH);
  SPI.endTransaction();
  return true;
}

uint8_t MAX31865Sensor::readFault() {
  return readRegister8(MAX31865_FAULT_REG);
}

void MAX31865Sensor::clearFault() {
  writeRegister(MAX31865_CONFIG_REG, MAX31865_CONFIG_BIAS | MAX31865_CONFIG_FAULTCLR);
  delay(10);
}

float MAX31865Sensor::readRTD() {
  if (!initialized && csPin == 0) return NAN;

  uint16_t raw = readRegister16(MAX31865_RTD_MSB_REG);
  if (raw & 0x0001) {
    uint8_t fault = readFault();
    Serial.printf("MAX31865 fault: 0x%02X\n", fault);
    clearFault();
    return NAN;
  }

  uint16_t rtdData = raw >> 1;
  if (rtdData == 0 || rtdData >= 32767) return NAN;
  return (rtdData * rref) / 32768.0f;
}

float MAX31865Sensor::calculateTemperatureC(float resistance) const {
  const float a = 3.9083e-3f;
  const float b = -5.775e-7f;
  float z1 = -a;
  float z2 = a * a - (4.0f * b);
  float z3 = (4.0f * b) / rnominal;
  float z4 = 2.0f * b;
  return (sqrtf(z2 + (z3 * resistance)) + z1) / z4;
}

float MAX31865Sensor::readTemperatureF() {
  float resistance = readRTD();
  if (isnan(resistance) || resistance < 40.0f || resistance > 400.0f) {
    return NAN;
  }

  float tempC = calculateTemperatureC(resistance);
  return tempC * 9.0f / 5.0f + 32.0f;
}
