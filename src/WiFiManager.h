// WiFiManager.h - Fixed WiFi Management System
#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>

// Custom WiFi modes to avoid conflicts
enum GrillWiFiMode {
  GRILL_WIFI_STA,    // Station mode (connect to existing network)
  GRILL_WIFI_AP,     // Access Point mode (create hotspot)
  GRILL_WIFI_MIXED   // Both AP and STA
};

// WiFi status
enum GrillWiFiStatus {
  GRILL_WIFI_DISCONNECTED,
  GRILL_WIFI_CONNECTING,
  GRILL_WIFI_CONNECTED,
  GRILL_WIFI_AP_MODE,
  GRILL_WIFI_FAILED
};

// WiFi configuration structure
struct GrillWiFiConfig {
  String ssid;
  String password;
  String hostname;
  bool useStaticIP;
  IPAddress staticIP;
  IPAddress gateway;
  IPAddress subnet;
  IPAddress dns1;
  IPAddress dns2;
};

// WiFi Manager class
class GrillWiFiManager {
private:
  GrillWiFiConfig config;
  GrillWiFiStatus currentStatus;
  unsigned long lastConnectionAttempt;
  unsigned long connectionTimeout;
  int connectionAttempts;
  bool apModeEnabled;
  
  // AP Mode settings
  String apSSID;
  String apPassword;
  IPAddress apIP;
  
  void startAPMode();
  void saveConfig();
  void loadConfig();
  void setupWebServer();
  
public:
  GrillWiFiManager();
  
  // Initialization and management
  void begin();
  void loop();
  
  // Configuration
  void setCredentials(String ssid, String password);
  void setHostname(String hostname);
  
  // Status and control
  GrillWiFiStatus getStatus();
  String getStatusString();
  String getStatusString(GrillWiFiStatus status);  // Add overload declaration
  bool isConnected();
  IPAddress getIP();
  String getSSID();
  
  // AP Mode control
  void enableAPMode(bool enable = true);
  void setAPCredentials(String ssid, String password);
  
  // Reset and reconnect
  void disconnect();
  void reconnect();
  void resetSettings();
};

extern GrillWiFiManager wifiManager;

#endif