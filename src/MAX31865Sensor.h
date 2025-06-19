// MAX31865Sensor.h - Dedicated class for MAX31865 RTD sensor
#ifndef MAX31865_SENSOR_H
#define MAX31865_SENSOR_H

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_MAX31865.h>

class MAX31865Sensor {
private:
  Adafruit_MAX31865 *rtd;
  bool initialized;
  bool debugEnabled;
  
  // Configuration
  uint8_t csPin;
  float rref;
  float rnominal;
  bool is3Wire;
  
  // Temperature tracking
  float lastValidTemp;
  unsigned long lastUpdate;
  unsigned long errorCount;
  
  // Calibration data
  float tempOffset;
  float tempScale;
  
  // Error tracking
  String lastError;
  uint8_t lastFaultCode;
  
public:
  MAX31865Sensor();
  ~MAX31865Sensor();
  
  // Initialization
  bool begin(uint8_t cs_pin, float ref_resistor = 430.0, float nominal_resistor = 100.0, bool three_wire = false);
  
    // Add this to MAX31865Sensor.h in the public section:
    bool testSPICommunication();
    String getSPITestResults();

  // Temperature reading
  float readTemperature();
  float readTemperatureF();
  float readTemperatureC();
  
  // Raw resistance reading
  float readRTD();
  uint16_t readRTDRaw();
  
  // Fault detection
  uint8_t readFault();
  bool hasFault();
  String getFaultString();
  String getFaultDescription(uint8_t fault);
  void clearFault();
  
  // Status and diagnostics
  bool isConnected();
  bool isInitialized() { return initialized; }
  String getLastError() { return lastError; }
  unsigned long getErrorCount() { return errorCount; }
  
  // Calibration
  void setCalibration(float offset, float scale = 1.0);
  void calibrateOffset(float knownTemp);
  void calibrateTwoPoint(float temp1, float reading1, float temp2, float reading2);
  
  // Configuration
  void setDebug(bool enable) { debugEnabled = enable; }
  bool getDebug() { return debugEnabled; }
  
  // Diagnostic functions
  void printDiagnostics();
  void runDiagnosticTest();
  bool testConnection();
  
  // Advanced configuration
  void setWireMode(bool threeWire);
  void setFilterFrequency(bool freq60Hz = true);  // true = 60Hz, false = 50Hz
  void enableBias(bool enable = true);
  void enableAutoConvert(bool enable = true);
  
  // Temperature validation
  bool isValidTemperature(float temp);
  
  // Statistics
  void resetErrorCount() { errorCount = 0; }
  float getLastValidTemp() { return lastValidTemp; }
  unsigned long getLastUpdateTime() { return lastUpdate; }
};

extern MAX31865Sensor grillSensor;

#endif