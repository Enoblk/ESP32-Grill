// WiFiManager.cpp - Fixed Implementation
#include "WiFiManager.h"
#include "Globals.h"

GrillWiFiManager wifiManager;

GrillWiFiManager::GrillWiFiManager() {
  currentStatus = GRILL_WIFI_DISCONNECTED;
  lastConnectionAttempt = 0;
  connectionTimeout = 30000; // 30 seconds
  connectionAttempts = 0;
  apModeEnabled = false;
  
  // Default AP settings - use MAC address for unique ID
  uint8_t mac[6];
  WiFi.macAddress(mac);
  apSSID = "GrillController-" + String(mac[4], HEX) + String(mac[5], HEX);
  apPassword = "grillpass123";
  apIP = IPAddress(192, 168, 4, 1);
  
  // Default STA settings
  config.hostname = "GrillController";
  config.useStaticIP = false;
}

void GrillWiFiManager::begin() {
  Serial.println("Initializing WiFi Manager...");
  
  // Load saved configuration
  loadConfig();
  
  WiFi.mode(WIFI_AP_STA); // Enable both modes initially
  WiFi.setHostname(config.hostname.c_str());
  
  // Debug: Show what we loaded
  Serial.printf("Loaded WiFi Config - SSID: '%s', Hostname: '%s'\n", 
                config.ssid.c_str(), config.hostname.c_str());
  
  // Try to connect if we have credentials
  if (config.ssid.length() > 0) {
    Serial.printf("Attempting to connect to: %s\n", config.ssid.c_str());
    reconnect();
    
    // Wait up to 15 seconds for connection
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
      delay(500);
      Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi connected successfully!");
      currentStatus = GRILL_WIFI_CONNECTED;
    } else {
      Serial.println("\nWiFi connection failed, starting AP mode");
      startAPMode();
    }
  } else {
    Serial.println("No WiFi credentials found, starting AP mode");
    startAPMode();
  }
  
  // Setup web configuration interface
  setupWebServer();
}

void GrillWiFiManager::loop() {
  static unsigned long lastCheck = 0;
  unsigned long now = millis();
  
  // Check WiFi status every 5 seconds
  if (now - lastCheck < 5000) return;
  lastCheck = now;
  
  GrillWiFiStatus oldStatus = currentStatus;
  
  // Update current status
  if (WiFi.status() == WL_CONNECTED) {
    currentStatus = GRILL_WIFI_CONNECTED;
    connectionAttempts = 0;
    
    // Disable AP mode if we're connected and it's enabled
    if (WiFi.getMode() != WIFI_STA && apModeEnabled) {
      Serial.println("WiFi connected, disabling AP mode");
      WiFi.mode(WIFI_STA);
      apModeEnabled = false;
    }
  } else {
    // Not connected
    if (currentStatus == GRILL_WIFI_CONNECTING) {
      // Check timeout
      if (now - lastConnectionAttempt > connectionTimeout) {
        Serial.println("WiFi connection timeout");
        connectionAttempts++;
        
        if (connectionAttempts >= 3) {
          Serial.println("Multiple connection failures, starting AP mode");
          startAPMode();
        } else {
          Serial.printf("Retrying connection (attempt %d/3)\n", connectionAttempts + 1);
          reconnect();
        }
      }
    } else if (currentStatus == GRILL_WIFI_CONNECTED) {
      // Lost connection
      Serial.println("WiFi connection lost, attempting reconnect");
      currentStatus = GRILL_WIFI_DISCONNECTED;
      reconnect();
    }
  }
  
  // Status change notification
  if (oldStatus != currentStatus) {
    Serial.printf("WiFi status changed: %s -> %s\n", 
                  getStatusString(oldStatus).c_str(), 
                  getStatusString().c_str());
  }
}

void GrillWiFiManager::startAPMode() {
  Serial.printf("Starting AP mode: %s\n", apSSID.c_str());
  
  // Configure AP
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID.c_str(), apPassword.c_str());
  
  currentStatus = GRILL_WIFI_AP_MODE;
  apModeEnabled = true;
  
  Serial.printf("AP started - SSID: %s, Password: %s\n", apSSID.c_str(), apPassword.c_str());
  Serial.printf("AP IP address: %s\n", WiFi.softAPIP().toString().c_str());
  Serial.println("Connect to the AP and go to http://192.168.4.1 to configure WiFi");
}

void GrillWiFiManager::reconnect() {
  if (config.ssid.length() == 0) {
    Serial.println("No SSID configured");
    startAPMode();
    return;
  }
  
  Serial.printf("Connecting to WiFi: %s\n", config.ssid.c_str());
  
  WiFi.begin(config.ssid.c_str(), config.password.c_str());
  
  currentStatus = GRILL_WIFI_CONNECTING;
  lastConnectionAttempt = millis();
}

void GrillWiFiManager::setCredentials(String ssid, String password) {
  config.ssid = ssid;
  config.password = password;
  saveConfig();
}

void GrillWiFiManager::saveConfig() {
  preferences.begin("wifi", false);
  preferences.putString("ssid", config.ssid);
  preferences.putString("password", config.password);
  preferences.putString("hostname", config.hostname);
  preferences.end();
  Serial.println("WiFi configuration saved");
}

void GrillWiFiManager::loadConfig() {
  preferences.begin("wifi", true);
  config.ssid = preferences.getString("ssid", "");
  config.password = preferences.getString("password", "");
  config.hostname = preferences.getString("hostname", "GrillController");
  preferences.end();
  
  Serial.printf("WiFi configuration loaded - SSID: %s\n", config.ssid.c_str());
}

void GrillWiFiManager::setupWebServer() {
  // WiFi configuration page
  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *req) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>WiFi Configuration</title>";
    html += "<style>";
    html += "body { background: #1a1a1a; color: #fff; font-family: Arial, sans-serif; padding: 20px; }";
    html += ".container { max-width: 600px; margin: 0 auto; }";
    html += "h1 { color: #60a5fa; text-align: center; margin-bottom: 30px; }";
    html += ".form-group { margin: 20px 0; }";
    html += "label { display: block; margin-bottom: 5px; font-weight: bold; }";
    html += "input, select { width: 100%; padding: 10px; font-size: 1em; border-radius: 5px; border: 1px solid #555; background: #333; color: #fff; }";
    html += ".btn { padding: 15px 30px; background: #059669; color: white; border: none; border-radius: 5px; font-size: 1.1em; cursor: pointer; margin: 10px 5px; }";
    html += ".btn:hover { background: #047857; }";
    html += ".btn-danger { background: #dc2626; }";
    html += ".btn-danger:hover { background: #b91c1c; }";
    html += ".status { padding: 15px; margin: 20px 0; border-radius: 5px; }";
    html += ".status-connected { background: #059669; }";
    html += ".status-ap { background: #f59e0b; }";
    html += ".status-disconnected { background: #dc2626; }";
    html += ".network-list { background: rgba(255,255,255,0.1); padding: 15px; border-radius: 5px; margin: 15px 0; }";
    html += ".network-item { padding: 10px; margin: 5px 0; background: rgba(255,255,255,0.1); border-radius: 5px; cursor: pointer; }";
    html += ".network-item:hover { background: rgba(255,255,255,0.2); }";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>WiFi Configuration</h1>";
    
    // Current status
    html += "<div class='status ";
    if (wifiManager.isConnected()) {
      html += "status-connected'>Connected to: " + wifiManager.getSSID();
      html += "<br>IP Address: " + wifiManager.getIP().toString();
    } else if (wifiManager.getStatus() == GRILL_WIFI_AP_MODE) {
      html += "status-ap'>AP Mode Active<br>Connect to: " + wifiManager.apSSID;
      html += "<br>AP IP: " + WiFi.softAPIP().toString();
    } else {
      html += "status-disconnected'>Disconnected";
    }
    html += "</div>";
    
    // Available networks (scan results)
    html += "<div class='network-list'>";
    html += "<h3>Available Networks:</h3>";
    html += "<div id='networks'>Click Scan to find networks</div>";
    html += "<button class='btn' onclick='scanNetworks()'>Scan Networks</button>";
    html += "</div>";
    
    // Configuration form
    html += "<form onsubmit='saveWiFi(event)'>";
    html += "<div class='form-group'>";
    html += "<label>Network Name (SSID):</label>";
    html += "<input type='text' id='ssid' name='ssid' value='" + wifiManager.config.ssid + "' required>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label>Password:</label>";
    html += "<input type='password' id='password' name='password' placeholder='Enter WiFi password'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label>Hostname:</label>";
    html += "<input type='text' id='hostname' name='hostname' value='" + wifiManager.config.hostname + "'>";
    html += "</div>";
    
    html += "<button type='submit' class='btn'>Save & Connect</button>";
    html += "<button type='button' class='btn btn-danger' onclick='resetWiFi()'>Reset WiFi Settings</button>";
    html += "</form>";
    
    html += "<a href='/' class='btn' style='display: block; text-align: center; margin: 20px 0; text-decoration: none;'>Back to Grill Control</a>";
    html += "</div>";

    // JavaScript (rest of the code continues...)
    html += "<script>";
    html += "function saveWiFi(event) {";
    html += "  event.preventDefault();";
    html += "  const ssid = document.getElementById('ssid').value;";
    html += "  const password = document.getElementById('password').value;";
    html += "  const hostname = document.getElementById('hostname').value;";
    html += "  ";
    html += "  fetch('/wifi_save', {";
    html += "    method: 'POST',";
    html += "    headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
    html += "    body: 'ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(password) + '&hostname=' + encodeURIComponent(hostname)";
    html += "  })";
    html += "  .then(response => response.text())";
    html += "  .then(data => {";
    html += "    alert(data);";
    html += "    setTimeout(() => location.reload(), 2000);";
    html += "  });";
    html += "}";
    
    html += "function resetWiFi() {";
    html += "  if (confirm('Reset all WiFi settings? This will restart the device in AP mode.')) {";
    html += "    fetch('/wifi_reset', {method: 'POST'})";
    html += "      .then(response => response.text())";
    html += "      .then(data => {";
    html += "        alert(data);";
    html += "        setTimeout(() => location.reload(), 2000);";
    html += "      });";
    html += "  }";
    html += "}";
    
    html += "function selectNetwork(ssid) {";
    html += "  document.getElementById('ssid').value = ssid;";
    html += "}";
    
    html += "function scanNetworks() {";
    html += "  document.getElementById('networks').innerHTML = 'Scanning...';";
    html += "  fetch('/wifi_scan')";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      document.getElementById('networks').innerHTML = data;";
    html += "    });";
    html += "}";
    
    html += "</script></body></html>";
    
    req->send(200, "text/html", html);
  });

  // Save WiFi settings
  server.on("/wifi_save", HTTP_POST, [](AsyncWebServerRequest *req) {
    String ssid = req->getParam("ssid", true)->value();
    String password = req->getParam("password", true)->value();
    String hostname = req->getParam("hostname", true)->value();
    
    wifiManager.setCredentials(ssid, password);
    wifiManager.setHostname(hostname);
    
    req->send(200, "text/plain", "WiFi settings saved. Attempting to connect...");
    
    // Attempt connection after a short delay
    delay(1000);
    wifiManager.reconnect();
  });

  // Reset WiFi settings
  server.on("/wifi_reset", HTTP_POST, [](AsyncWebServerRequest *req) {
    wifiManager.resetSettings();
    req->send(200, "text/plain", "WiFi settings reset. Restarting in AP mode...");
    delay(1000);
    ESP.restart();
  });

  // Scan for networks - simplified without JSON
  server.on("/wifi_scan", HTTP_GET, [](AsyncWebServerRequest *req) {
    int n = WiFi.scanNetworks();
    String html = "";
    
    if (n == 0) {
      html = "No networks found";
    } else {
      for (int i = 0; i < n; i++) {
        html += "<div class='network-item' onclick='selectNetwork(\"" + WiFi.SSID(i) + "\")'>";
        html += WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm)";
        if (WiFi.encryptionType(i) != WIFI_AUTH_OPEN) {
          html += " 🔒";
        }
        html += "</div>";
      }
    }
    
    req->send(200, "text/html", html);
  });

  // WiFi debug endpoint - Fixed hostname concatenation
  server.on("/wifi_debug", HTTP_GET, [](AsyncWebServerRequest *req) {
    String debug = "WiFi Debug Info:\\n";
    debug += "Status: " + wifiManager.getStatusString() + "\\n";
    debug += "SSID: " + WiFi.SSID() + "\\n";
    debug += "IP: " + WiFi.localIP().toString() + "\\n";
    debug += "Gateway: " + WiFi.gatewayIP().toString() + "\\n";
    debug += "DNS: " + WiFi.dnsIP().toString() + "\\n";
    debug += "RSSI: " + String(WiFi.RSSI()) + " dBm\\n";
    debug += "Hostname: " + String(WiFi.getHostname()) + "\\n";  // Fixed line
    debug += "MAC: " + WiFi.macAddress() + "\\n";
    
    uint8_t mac[6];
    WiFi.macAddress(mac);
    debug += "AP SSID: GrillController-" + String(mac[4], HEX) + String(mac[5], HEX) + "\\n";
    debug += "AP IP: " + WiFi.softAPIP().toString() + "\\n";
    
    req->send(200, "text/plain", debug);
  });
}

String GrillWiFiManager::getStatusString() {
  switch (currentStatus) {
    case GRILL_WIFI_DISCONNECTED: return "Disconnected";
    case GRILL_WIFI_CONNECTING: return "Connecting";
    case GRILL_WIFI_CONNECTED: return "Connected";
    case GRILL_WIFI_AP_MODE: return "AP Mode";
    case GRILL_WIFI_FAILED: return "Failed";
    default: return "Unknown";
  }
}

String GrillWiFiManager::getStatusString(GrillWiFiStatus status) {
  switch (status) {
    case GRILL_WIFI_DISCONNECTED: return "Disconnected";
    case GRILL_WIFI_CONNECTING: return "Connecting";
    case GRILL_WIFI_CONNECTED: return "Connected";
    case GRILL_WIFI_AP_MODE: return "AP Mode";
    case GRILL_WIFI_FAILED: return "Failed";
    default: return "Unknown";
  }
}

GrillWiFiStatus GrillWiFiManager::getStatus() {
  return currentStatus;
}

bool GrillWiFiManager::isConnected() {
  return currentStatus == GRILL_WIFI_CONNECTED;
}

IPAddress GrillWiFiManager::getIP() {
  if (isConnected()) {
    return WiFi.localIP();
  } else if (currentStatus == GRILL_WIFI_AP_MODE) {
    return WiFi.softAPIP();
  }
  return IPAddress(0, 0, 0, 0);
}

String GrillWiFiManager::getSSID() {
  return WiFi.SSID();
}

void GrillWiFiManager::setHostname(String hostname) {
  config.hostname = hostname;
  WiFi.setHostname(hostname.c_str());
}

void GrillWiFiManager::resetSettings() {
  preferences.begin("wifi", false);
  preferences.clear();
  preferences.end();
  
  config.ssid = "";
  config.password = "";
  config.hostname = "GrillController";
  config.useStaticIP = false;
  
  Serial.println("WiFi settings reset");
}

void GrillWiFiManager::enableAPMode(bool enable) {
  if (enable && !apModeEnabled) {
    startAPMode();
  } else if (!enable && apModeEnabled) {
    WiFi.mode(WIFI_STA);
    apModeEnabled = false;
    Serial.println("AP mode disabled");
  }
}

void GrillWiFiManager::setAPCredentials(String ssid, String password) {
  apSSID = ssid;
  apPassword = password;
}

void GrillWiFiManager::disconnect() {
  WiFi.disconnect();
  currentStatus = GRILL_WIFI_DISCONNECTED;
  Serial.println("WiFi disconnected");
}