// GrillWebServer.cpp - Complete version with MAX31865 support (no GRILL_TEMP_PIN references)
#include "Globals.h"
#include "Utility.h"
#include "PelletControl.h" 
#include "Ignition.h"
#include "RelayControl.h"
#include "OLEDDisplay.h"
#include "WiFiManager.h"
#include "TemperatureSensor.h"
#include "MAX31865Sensor.h"  // Add MAX31865 support
#include <WiFi.h>
#include <Preferences.h>
#include <WiFiClient.h>
#include <ElegantOTA.h>

void setup_grill_server() {
  // Main dashboard with all 6 temperatures
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
    // Get grill temperature (MAX31865)
    double grillTemp = readGrillTemperature();
    
    // Get ambient temperature (10K NTC)
    double ambientTemp = readAmbientTemperature();
    
    // Get meat probe temperatures (1K NTC via ADS1115)
    float meat1 = tempSensor.getFoodTemperature(1);
    float meat2 = tempSensor.getFoodTemperature(2);
    float meat3 = tempSensor.getFoodTemperature(3);
    float meat4 = tempSensor.getFoodTemperature(4);
    
    String status = getStatus(grillTemp);
    bool ignOn = digitalRead(RELAY_IGNITER_PIN) == HIGH;
    bool augerOn = digitalRead(RELAY_AUGER_PIN) == HIGH;
    bool hopperOn = digitalRead(RELAY_HOPPER_FAN_PIN) == HIGH;
    bool blowerOn = digitalRead(RELAY_BLOWER_FAN_PIN) == HIGH;
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1, user-scalable=no'>";
    html += "<title>Green Mountain Grill Controller</title>";
    html += "<style>";
    html += "* { box-sizing: border-box; margin: 0; padding: 0; }";
    html += "body { background: linear-gradient(135deg, #1e3c72, #2a5298); color: #fff; ";
    html += "font-family: Arial, sans-serif; padding: 10px; min-height: 100vh; }";
    html += ".container { max-width: 800px; margin: 0 auto; }";
    html += ".header { text-align: center; margin-bottom: 20px; }";
    html += ".header h1 { font-size: 2em; margin-bottom: 10px; }";
    
    // Main grill temperature display
    html += ".grill-temp { background: rgba(255,255,255,0.15); border-radius: 15px; ";
    html += "padding: 20px; margin-bottom: 20px; text-align: center; border: 2px solid #4ade80; }";
    html += ".grill-temp-main { font-size: 3em; font-weight: bold; margin-bottom: 10px; }";
    html += ".grill-temp-set { font-size: 1.2em; margin-bottom: 10px; }";
    html += ".status { font-size: 1.3em; font-weight: bold; padding: 10px; border-radius: 10px; }";
    
    // All temperature grid
    html += ".temp-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 15px; margin: 20px 0; }";
    html += ".temp-card { background: rgba(255,255,255,0.1); padding: 15px; border-radius: 10px; text-align: center; }";
    html += ".temp-card.grill { border: 2px solid #4ade80; }";
    html += ".temp-card.ambient { border: 2px solid #60a5fa; }";
    html += ".temp-card.meat { border: 2px solid #f59e0b; }";
    html += ".temp-card h3 { font-size: 0.9em; margin-bottom: 8px; opacity: 0.8; }";
    html += ".temp-value { font-size: 1.8em; font-weight: bold; margin-bottom: 5px; }";
    html += ".temp-type { font-size: 0.8em; opacity: 0.7; }";
    html += ".temp-invalid { color: #ef4444; }";
    
    html += ".controls { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin: 20px 0; }";
    html += ".btn { padding: 15px; font-size: 1.1em; font-weight: bold; border: none; ";
    html += "border-radius: 10px; color: white; cursor: pointer; text-align: center; text-decoration: none; display: block; }";
    html += ".btn-primary { background: #667eea; }";
    html += ".btn-danger { background: #f093fb; }";
    html += ".btn-success { background: #4facfe; }";
    html += ".btn-warning { background: #fbbf24; }";
    html += ".temp-presets { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; margin: 15px 0; }";
    html += ".temp-preset { padding: 12px; background: rgba(255,255,255,0.1); border: none; ";
    html += "border-radius: 8px; color: white; cursor: pointer; }";
    html += ".relays { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin: 20px 0; }";
    html += ".relay { display: flex; align-items: center; padding: 12px; ";
    html += "background: rgba(255,255,255,0.1); border-radius: 10px; }";
    html += ".relay-dot { width: 12px; height: 12px; border-radius: 50%; margin-right: 10px; }";
    html += ".relay-on { background: #4ade80; }";
    html += ".relay-off { background: #6b7280; }";
    html += ".link-btn { display: inline-block; margin: 5px; padding: 8px 15px; ";
    html += "background: rgba(255,255,255,0.2); color: white; text-decoration: none; border-radius: 5px; }";
    html += ".status.igniting { background: #ff6b35; }";
    html += ".status.heating { background: #ff9a56; }";
    html += ".status.at-temp { background: #4ecdc4; }";
    html += ".status.idle { background: rgba(255,255,255,0.2); }";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<div class='header'>";
    html += "<h1>Green Mountain Grill</h1>";
    html += "<div>Daniel Boone Controller - MAX31865 RTD</div>";
    html += "<div>IP: " + WiFi.localIP().toString() + "</div>";
    
    // Navigation links
    html += "<div style='margin: 15px 0;'>";
    html += "<a href='/wifi' class='link-btn'>WiFi Settings</a>";
    html += "<a href='/manual' class='link-btn'>Manual Control</a>";
    html += "<a href='/pid' class='link-btn'>PID Tuning</a>";
    html += "<a href='/debug' class='link-btn'>Debug</a>";
    html += "<a href='/update' class='link-btn'>OTA Update</a>";
    html += "<a href='/max31865' class='link-btn'>🌡️ MAX31865 Sensor</a>";
    html += "<a href='/reboot' class='link-btn'>Reboot</a>";
    html += "<a href='/spi_test' class='link-btn'>spi test</a>";
    html += "</div>";
    html += "</div>";
    
    // Speed control selector
    html += "<div style='margin: 10px 0; text-align: center;'>";
    html += "<label style='color: #bbb; font-size: 0.9em;'>";
    html += "Update Speed: ";
    html += "<select id='updateSpeed' onchange='changeUpdateSpeed()' style='background: #333; color: #fff; border: 1px solid #555; border-radius: 3px; padding: 2px;'>";
    html += "<option value='1000'>Fast (1s)</option>";
    html += "<option value='1500' selected>Normal (1.5s)</option>";
    html += "<option value='3000'>Slow (3s)</option>";
    html += "<option value='5000'>Very Slow (5s)</option>";
    html += "<option value='0'>Paused</option>";
    html += "</select>";
    html += "</label>";
    html += "</div>";

    // Main grill temperature display (prominent)
    html += "<div class='grill-temp'>";
    if (isValidTemperature(grillTemp)) {
      html += "<div class='temp-value' id='grill-temp-main'>" + String(grillTemp, 1) + "&deg;F</div>";
    } else {
      html += "<div class='temp-value temp-invalid'>ERROR</div>";
    }
    html += "<div class='grill-temp-set'>Target: <span id='setpoint'>" + String((int)setpoint) + "</span>&deg;F</div>";
    
    String statusClass = status;
    statusClass.toLowerCase();
    statusClass.replace(" ", "-");
    html += "<div class='status " + statusClass + "' id='status'>" + status + "</div>";
    html += "</div>";
    
    // All 6 temperatures grid
    html += "<div class='temp-grid'>";
    
    // Grill Temperature
    html += "<div class='temp-card grill'>";
    html += "<h3>GRILL TEMPERATURE</h3>";
    if (isValidTemperature(grillTemp)) {
      html += "<div class='temp-value' id='grill-temp-card'>" + String(grillTemp, 1) + "&deg;F</div>";
    } else {
      html += "<div class='temp-value temp-invalid'>ERROR</div>";
    }
    html += "<div class='temp-type'>MAX31865 RTD</div>";
    html += "</div>";
    
    // Ambient Temperature
    html += "<div class='temp-card ambient'>";
    html += "<h3>AMBIENT</h3>";
    if (ambientTemp > -900) {
      html += "<div class='temp-value' id='ambient-temp'>" + String(ambientTemp, 1) + "&deg;F</div>";
    } else {
      html += "<div class='temp-value temp-invalid'>N/A</div>";
    }
    html += "<div class='temp-type'>10K NTC</div>";
    html += "</div>";
    
    // Meat Probe 1
    html += "<div class='temp-card meat'>";
    html += "<h3>MEAT PROBE 1</h3>";
    if (meat1 > -900) {
      html += "<div class='temp-value' id='meat1-temp'>" + String(meat1, 1) + "&deg;F</div>";
    } else {
      html += "<div class='temp-value temp-invalid'>N/A</div>";
    }
    html += "<div class='temp-type'>1K NTC</div>";
    html += "</div>";
    
    // Meat Probe 2
    html += "<div class='temp-card meat'>";
    html += "<h3>MEAT PROBE 2</h3>";
    if (meat2 > -900) {
      html += "<div class='temp-value' id='meat2-temp'>" + String(meat2, 1) + "&deg;F</div>";
    } else {
      html += "<div class='temp-value temp-invalid'>N/A</div>";
    }
    html += "<div class='temp-type'>1K NTC</div>";
    html += "</div>";
    
    // Meat Probe 3
    html += "<div class='temp-card meat'>";
    html += "<h3>MEAT PROBE 3</h3>";
    if (meat3 > -900) {
      html += "<div class='temp-value' id='meat3-temp'>" + String(meat3, 1) + "&deg;F</div>";
    } else {
      html += "<div class='temp-value temp-invalid'>N/A</div>";
    }
    html += "<div class='temp-type'>1K NTC</div>";
    html += "</div>";
    
    // Meat Probe 4
    html += "<div class='temp-card meat'>";
    html += "<h3>MEAT PROBE 4</h3>";
    if (meat4 > -900) {
      html += "<div class='temp-value' id='meat4-temp'>" + String(meat4, 1) + "&deg;F</div>";
    } else {
      html += "<div class='temp-value temp-invalid'>N/A</div>";
    }
    html += "<div class='temp-type'>1K NTC</div>";
    html += "</div>";
    
    html += "</div>";
    
    // Temperature presets
    html += "<div class='temp-presets'>";
    html += "<button class='btn temp-preset' onclick='setTemp(225)'>225&deg;F Low</button>";
    html += "<button class='btn temp-preset' onclick='setTemp(275)'>275&deg;F Med</button>";
    html += "<button class='btn temp-preset' onclick='setTemp(325)'>325&deg;F High</button>";
    html += "<button class='btn temp-preset' onclick='setTemp(200)'>200&deg;F Warm</button>";
    html += "<button class='btn temp-preset' onclick='setTemp(250)'>250&deg;F Smoke</button>";
    html += "<button class='btn temp-preset' onclick='setTemp(375)'>375&deg;F Sear</button>";
    html += "</div>";
    
    // Control buttons
    html += "<div class='controls'>";
    if (grillRunning) {
      html += "<button class='btn btn-danger' onclick='stopGrill()'>STOP Grill</button>";
    } else {
      html += "<button class='btn btn-success' onclick='startGrill()'>START Grill</button>";
    }
    html += "<button class='btn btn-primary' onclick='adjustTemp()'>Adjust Temp</button>";
    html += "</div>";
    
    // Relay status
    html += "<div class='relays'>";
    html += "<div class='relay'>";
    html += "<div class='relay-dot " + String(ignOn ? "relay-on" : "relay-off") + "' id='ign-dot'></div>";
    html += "<span>Igniter</span></div>";
    html += "<div class='relay'>";
    html += "<div class='relay-dot " + String(augerOn ? "relay-on" : "relay-off") + "' id='aug-dot'></div>";
    html += "<span>Auger</span></div>";
    html += "<div class='relay'>";
    html += "<div class='relay-dot " + String(hopperOn ? "relay-on" : "relay-off") + "' id='hop-dot'></div>";
    html += "<span>Hopper Fan</span></div>";
    html += "<div class='relay'>";
    html += "<div class='relay-dot " + String(blowerOn ? "relay-on" : "relay-off") + "' id='blo-dot'></div>";
    html += "<span>Blower Fan</span></div>";
    html += "</div>";
    
    html += "</div>";

    // JavaScript for functionality and REAL-TIME auto-updates
    html += "<script>";
    html += "let updateInterval;";
    html += "let isPageVisible = true;";

    html += "document.addEventListener('visibilitychange', function() {";
    html += "  isPageVisible = !document.hidden;";
    html += "  if (isPageVisible) {";
    html += "    startRealTimeUpdates();";
    html += "  } else {";
    html += "    stopRealTimeUpdates();";
    html += "  }";
    html += "});";

    html += "function startRealTimeUpdates() {";
    html += "  const speed = parseInt(document.getElementById('updateSpeed').value);";
    html += "  if (speed === 0) return;";
    html += "  if (updateInterval) clearInterval(updateInterval);";
    html += "  updateTemperatures();";
    html += "  updateInterval = setInterval(updateTemperatures, speed);";
    html += "}";

    html += "function stopRealTimeUpdates() {";
    html += "  if (updateInterval) {";
    html += "    clearInterval(updateInterval);";
    html += "    updateInterval = null;";
    html += "  }";
    html += "}";

    html += "function updateTemperatures() {";
    html += "  if (!isPageVisible) return;";
    html += "  ";
    html += "  fetch('/status_all')";
    html += "    .then(response => {";
    html += "      if (!response.ok) throw new Error('Network response was not ok');";
    html += "      return response.json();";
    html += "    })";
    html += "    .then(data => {";

    // Update main grill temperature
    html += "      const grillTempElement = document.getElementById('grill-temp-main');";
    html += "      const grillTempCardElement = document.getElementById('grill-temp-card');";
    html += "      ";
    html += "      if (data.grillTemp > 0) {";
    html += "        const newTemp = data.grillTemp.toFixed(1);";
    html += "        if (grillTempElement.innerHTML !== newTemp + '&deg;F') {";
    html += "          grillTempElement.innerHTML = newTemp + '&deg;F';";
    html += "          grillTempCardElement.innerHTML = newTemp + '&deg;F';";
    html += "          grillTempElement.style.transition = 'color 0.3s ease';";
    html += "          grillTempElement.style.color = '#4ade80';";
    html += "          setTimeout(() => { grillTempElement.style.color = ''; }, 300);";
    html += "        }";
    html += "        grillTempElement.className = 'temp-value';";
    html += "        grillTempCardElement.className = 'temp-value';";
    html += "      } else {";
    html += "        grillTempElement.innerHTML = 'SENSOR ERROR';";
    html += "        grillTempCardElement.innerHTML = 'ERROR';";
    html += "        grillTempElement.className = 'temp-value temp-invalid';";
    html += "        grillTempCardElement.className = 'temp-value temp-invalid';";
    html += "      }";

    // Update other temperatures and controls
    html += "      const ambientTempElement = document.getElementById('ambient-temp');";
    html += "      if (data.ambientTemp > -900) {";
    html += "        ambientTempElement.innerHTML = data.ambientTemp.toFixed(1) + '&deg;F';";
    html += "        ambientTempElement.className = 'temp-value';";
    html += "      } else {";
    html += "        ambientTempElement.innerHTML = 'N/A';";
    html += "        ambientTempElement.className = 'temp-value temp-invalid';";
    html += "      }";

    html += "      ['meat1', 'meat2', 'meat3', 'meat4'].forEach((probe, index) => {";
    html += "        const temp = data[probe + 'Temp'];";
    html += "        const element = document.getElementById(probe + '-temp');";
    html += "        if (temp > -900) {";
    html += "          element.innerHTML = temp.toFixed(1) + '&deg;F';";
    html += "          element.className = 'temp-value';";
    html += "        } else {";
    html += "          element.innerHTML = 'N/A';";
    html += "          element.className = 'temp-value temp-invalid';";
    html += "        }";
    html += "      });";

    html += "      document.getElementById('setpoint').textContent = data.setpoint;";
    html += "      document.getElementById('status').textContent = data.status;";

    html += "      const relays = [";
    html += "        {id: 'ign-dot', state: data.ignOn},";
    html += "        {id: 'aug-dot', state: data.augerOn},";
    html += "        {id: 'hop-dot', state: data.hopperOn},";
    html += "        {id: 'blo-dot', state: data.blowerOn}";
    html += "      ];";
    html += "      relays.forEach(relay => {";
    html += "        const element = document.getElementById(relay.id);";
    html += "        element.className = 'relay-dot ' + (relay.state ? 'relay-on' : 'relay-off');";
    html += "      });";

    html += "    })";
    html += "    .catch(err => console.log('Update failed:', err));";
    html += "}";

    html += "function changeUpdateSpeed() {";
    html += "  stopRealTimeUpdates();";
    html += "  startRealTimeUpdates();";
    html += "}";

    html += "function setTemp(temp) {";
    html += "  document.getElementById('setpoint').textContent = temp;";
    html += "  fetch('/set_temp?temp=' + temp)";
    html += "    .then(response => response.text())";
    html += "    .then(data => console.log(data));";
    html += "}";

    html += "function startGrill() {";
    html += "  fetch('/start')";
    html += "    .then(response => response.text())";
    html += "    .then(data => console.log(data));";
    html += "}";

    html += "function stopGrill() {";
    html += "  fetch('/stop')";
    html += "    .then(response => response.text())";
    html += "    .then(data => console.log(data));";
    html += "}";

    html += "function adjustTemp() {";
    html += "  const newTemp = prompt('Enter target temperature (150-500F):', document.getElementById('setpoint').textContent);";
    html += "  if (newTemp && newTemp >= 150 && newTemp <= 500) {";
    html += "    setTemp(parseInt(newTemp));";
    html += "  }";
    html += "}";

    html += "document.addEventListener('DOMContentLoaded', function() {";
    html += "  startRealTimeUpdates();";
    html += "});";

    html += "</script>";
    html += "</body></html>";
    req->send(200, "text/html", html);
  });

  // Manual Control Page
  server.on("/manual", HTTP_GET, [](AsyncWebServerRequest *req) {
    bool ignOn = digitalRead(RELAY_IGNITER_PIN) == HIGH;
    bool augerOn = digitalRead(RELAY_AUGER_PIN) == HIGH;
    bool hopperOn = digitalRead(RELAY_HOPPER_FAN_PIN) == HIGH;
    bool blowerOn = digitalRead(RELAY_BLOWER_FAN_PIN) == HIGH;
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>Manual Control - Grill Controller</title>";
    html += "<style>";
    html += "body { background: #1a1a1a; color: #fff; font-family: Arial, sans-serif; padding: 20px; }";
    html += ".container { max-width: 600px; margin: 0 auto; }";
    html += "h1 { color: #60a5fa; text-align: center; margin-bottom: 30px; }";
    html += ".relay-control { background: rgba(255,255,255,0.1); padding: 20px; margin: 15px 0; border-radius: 10px; }";
    html += ".relay-status { display: flex; align-items: center; margin-bottom: 15px; }";
    html += ".status-dot { width: 20px; height: 20px; border-radius: 50%; margin-right: 15px; }";
    html += ".status-on { background: #4ade80; }";
    html += ".status-off { background: #6b7280; }";
    html += ".btn { padding: 10px 20px; margin: 5px; background: #059669; color: white; border: none; border-radius: 5px; cursor: pointer; }";
    html += ".btn-danger { background: #dc2626; }";
    html += ".btn-warning { background: #f59e0b; }";
    html += ".warning { background: #fbbf24; color: #000; padding: 15px; border-radius: 5px; margin: 20px 0; }";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>Manual Relay Control</h1>";
    
    html += "<div class='warning'>";
    html += "⚠️ <strong>WARNING:</strong> Manual control overrides automatic safety systems.";
    html += "</div>";
    
    // Relay controls (same as before)
    html += "<div class='relay-control'>";
    html += "<div class='relay-status'>";
    html += "<div class='status-dot " + String(ignOn ? "status-on" : "status-off") + "'></div>";
    html += "<h3>Igniter</h3>";
    html += "</div>";
    html += "<button class='btn' onclick='controlRelay(\"ignite\", \"on\")'>Turn ON</button>";
    html += "<button class='btn btn-danger' onclick='controlRelay(\"ignite\", \"off\")'>Turn OFF</button>";
    html += "</div>";
    
// Auger
html += "<div class='relay-control'>";
html += "<div class='relay-status'>";
html += "<div class='status-dot " + String(augerOn ? "status-on" : "status-off") + "'></div>";
html += "<h3>Auger</h3>";
html += "</div>";
html += "<button class='btn' onclick='controlRelay(\"auger\", \"on\")'>Turn ON</button>";
html += "<button class='btn btn-danger' onclick='controlRelay(\"auger\", \"off\")'>Turn OFF</button>";
html += "</div>";

// Hopper Fan
html += "<div class='relay-control'>";
html += "<div class='relay-status'>";
html += "<div class='status-dot " + String(hopperOn ? "status-on" : "status-off") + "'></div>";
html += "<h3>Hopper Fan</h3>";
html += "</div>";
html += "<button class='btn' onclick='controlRelay(\"hopper\", \"on\")'>Turn ON</button>";
html += "<button class='btn btn-danger' onclick='controlRelay(\"hopper\", \"off\")'>Turn OFF</button>";
html += "</div>";

// Blower Fan
html += "<div class='relay-control'>";
html += "<div class='relay-status'>";
html += "<div class='status-dot " + String(blowerOn ? "status-on" : "status-off") + "'></div>";
html += "<h3>Blower Fan</h3>";
html += "</div>";
html += "<button class='btn' onclick='controlRelay(\"blower\", \"on\")'>Turn ON</button>";
html += "<button class='btn btn-danger' onclick='controlRelay(\"blower\", \"off\")'>Turn OFF</button>";
html += "</div>";

    // Add other relay controls...
    
    html += "<a href='/' class='btn' style='display: block; text-align: center; margin: 20px 0; text-decoration: none;'>Back to Dashboard</a>";
    html += "</div>";

    html += "<script>";
    html += "function controlRelay(relay, state) {";
    html += "  fetch('/control?relay=' + relay + '&state=' + state)";
    html += "    .then(response => response.text())";
    html += "    .then(data => { alert(data); setTimeout(() => location.reload(), 1000); });";
    html += "}";
    html += "</script>";
    
    html += "</body></html>";
    req->send(200, "text/html", html);
  });

  // PID Tuning Page
  server.on("/pid", HTTP_GET, [](AsyncWebServerRequest *req) {
    float kp, ki, kd;
    getPIDParameters(&kp, &ki, &kd);
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='utf-8'>";
    html += "<title>PID Tuning</title>";
    html += "<style>";
    html += "body { background: #1a1a1a; color: #fff; font-family: Arial, sans-serif; padding: 20px; }";
    html += ".container { max-width: 600px; margin: 0 auto; }";
    html += ".btn { padding: 15px 30px; background: #059669; color: white; border: none; border-radius: 5px; cursor: pointer; }";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>PID Tuning</h1>";
    html += "<p>Current: Kp=" + String(kp, 3) + ", Ki=" + String(ki, 4) + ", Kd=" + String(kd, 3) + "</p>";
    html += "<a href='/' class='btn'>Back to Dashboard</a>";
    html += "</div></body></html>";
    
    req->send(200, "text/html", html);
  });

  // Debug Control Page
  server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *req) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='utf-8'>";
    html += "<title>Debug Control</title>";
    html += "<style>";
    html += "body { background: #1a1a1a; color: #fff; font-family: Arial, sans-serif; padding: 20px; }";
    html += ".container { max-width: 600px; margin: 0 auto; }";
    html += ".btn { padding: 15px 30px; background: #059669; color: white; border: none; border-radius: 5px; cursor: pointer; margin: 10px; }";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>Debug Control Center</h1>";
    html += "<button class='btn' onclick='toggleDebug(\"grill\")'>Toggle Grill Debug</button>";
    html += "<button class='btn' onclick='toggleDebug(\"meat\")'>Toggle Meat Debug</button>";
    html += "<a href='/' class='btn'>Back to Dashboard</a>";
    html += "</div>";
    
    html += "<script>";
    html += "function toggleDebug(sensor) {";
    html += "  fetch('/set_individual_debug?sensor=' + sensor + '&enabled=1')";
    html += "    .then(response => response.text())";
    html += "    .then(data => alert(data));";
    html += "}";
    html += "</script></body></html>";
    
    req->send(200, "text/html", html);
  });

  // MAX31865 Sensor Page
  server.on("/max31865", HTTP_GET, [](AsyncWebServerRequest *req) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>MAX31865 RTD Sensor</title>";
    html += "<style>";
    html += "body { background: #1a1a1a; color: #fff; font-family: Arial, sans-serif; padding: 20px; }";
    html += ".container { max-width: 800px; margin: 0 auto; }";
    html += "h1 { color: #60a5fa; text-align: center; margin-bottom: 30px; }";
    html += ".reading { background: rgba(255,255,255,0.1); padding: 20px; margin: 15px 0; border-radius: 10px; }";
    html += ".good { border-left: 5px solid #059669; }";
    html += ".bad { border-left: 5px solid #dc2626; }";
    html += ".value { font-size: 1.5em; font-weight: bold; margin: 10px 0; }";
    html += ".btn { padding: 15px 30px; background: #059669; color: white; border: none; border-radius: 5px; cursor: pointer; margin: 10px; }";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>🌡️ MAX31865 RTD Sensor Status</h1>";
    
    // Get current readings
    float temp = grillSensor.readTemperatureF();
    float resistance = grillSensor.readRTD();
   // uint16_t rawValue = grillSensor.readRTDRaw();
    //uint8_t fault = grillSensor.readFault();
    //bool connected = grillSensor.isConnected();
    
    // Temperature reading
    String tempClass = (temp > 32.0 && temp < 600.0) ? "good" : "bad";
    html += "<div class='reading " + tempClass + "'>";
    html += "<h3>🔥 Temperature Reading</h3>";
    html += "<div class='value'>" + String(temp, 1) + " °F</div>";
    html += "<div>" + String((temp - 32.0) * 5.0 / 9.0, 1) + " °C</div>";
    html += "</div>";
    
    // Resistance reading
    String resClass = (resistance > 80.0 && resistance < 200.0) ? "good" : "bad";
    html += "<div class='reading " + resClass + "'>";
    html += "<h3>⚡ RTD Resistance</h3>";
    html += "<div class='value'>" + String(resistance, 2) + " Ω</div>";
    html += "<div>Expected: ~108Ω at 70°F, ~138Ω at 200°F</div>";
    html += "</div>";
  
    
    // Control buttons
    html += "<div style='text-align: center; margin: 30px 0;'>";
    html += "<button class='btn' onclick='runTest()'>🧪 Run Test</button>";
    html += "<button class='btn' onclick='clearFaults()'>🔄 Clear Faults</button>";
    html += "<button class='btn' onclick='location.reload()'>🔄 Refresh</button>";
    html += "</div>";
    
    html += "<a href='/' style='display: block; text-align: center; color: #60a5fa; margin: 20px;'>← Back to Dashboard</a>";
    html += "</div>";
    
    html += "<script>";
    html += "function runTest() {";
    html += "  fetch('/max31865_test')";
    html += "    .then(response => response.text())";
    html += "    .then(data => { alert(data); location.reload(); });";
    html += "}";
    html += "function clearFaults() {";
    html += "  fetch('/max31865_clear')";
    html += "    .then(response => response.text())";
    html += "    .then(data => { alert(data); location.reload(); });";
    html += "}";
    html += "setInterval(() => location.reload(), 10000);";  // Auto-refresh every 10 seconds
    html += "</script>";
    
    html += "</body></html>";
    req->send(200, "text/html", html);
  });

  // Enhanced status endpoint with all 6 temperatures
  server.on("/status_all", HTTP_GET, [](AsyncWebServerRequest *req) {
    double grillTemp = readGrillTemperature();
    double ambientTemp = readAmbientTemperature();
    float meat1 = tempSensor.getFoodTemperature(1);
    float meat2 = tempSensor.getFoodTemperature(2);
    float meat3 = tempSensor.getFoodTemperature(3);
    float meat4 = tempSensor.getFoodTemperature(4);
    
    bool ignOn = digitalRead(RELAY_IGNITER_PIN) == HIGH;
    bool augerOn = digitalRead(RELAY_AUGER_PIN) == HIGH;
    bool hopperOn = digitalRead(RELAY_HOPPER_FAN_PIN) == HIGH;
    bool blowerOn = digitalRead(RELAY_BLOWER_FAN_PIN) == HIGH;
    String status = getStatus(grillTemp);

    String json = "{";
    json += "\"grillTemp\":" + String(grillTemp, 1) + ",";
    json += "\"ambientTemp\":" + String(ambientTemp, 1) + ",";
    json += "\"meat1Temp\":" + String(meat1, 1) + ",";
    json += "\"meat2Temp\":" + String(meat2, 1) + ",";
    json += "\"meat3Temp\":" + String(meat3, 1) + ",";
    json += "\"meat4Temp\":" + String(meat4, 1) + ",";
    json += "\"setpoint\":" + String((int)setpoint) + ",";
    json += "\"status\":\"" + status + "\",";
    json += "\"ignOn\":" + String(ignOn ? "true" : "false") + ",";
    json += "\"augerOn\":" + String(augerOn ? "true" : "false") + ",";
    json += "\"hopperOn\":" + String(hopperOn ? "true" : "false") + ",";
    json += "\"blowerOn\":" + String(blowerOn ? "true" : "false");
    json += "}";
    req->send(200, "application/json", json);
  });

  // Temperature setting endpoint
  server.on("/set_temp", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!req->hasParam("temp")) { 
      req->send(400, "text/plain", "Missing temp parameter"); 
      return; 
    }
    
    int newTemp = req->getParam("temp")->value().toInt();
    if (newTemp < 150 || newTemp > 500) {
      req->send(400, "text/plain", "Temperature out of range (150-500F)");
      return;
    }
    
    setpoint = newTemp;
    save_setpoint();
    req->send(200, "text/plain", "Temperature set to " + String(newTemp) + "F");
  });

  // Control endpoints
  server.on("/start", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (grillRunning) {
      req->send(200, "text/plain", "Grill already running");
      return;
    }
    
    grillRunning = true;
    double currentTemp = readGrillTemperature();
    ignition_start(currentTemp);
    
    req->send(200, "text/plain", "Grill started - ignition sequence initiated");
  });

  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!grillRunning) {
      req->send(200, "text/plain", "Grill already stopped");
      return;
    }
    
    grillRunning = false;
    ignition_stop();
    relay_clear_manual();
    
    req->send(200, "text/plain", "Grill stopped - cooling down");
  });

  // Manual relay control endpoints
  server.on("/control", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!req->hasParam("relay") || !req->hasParam("state")) { 
      req->send(400, "text/plain", "Missing params"); 
      return; 
    }
    
    String relayName = req->getParam("relay")->value();
    String state = req->getParam("state")->value();
    
    RelayRequest manualReq = {RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE, RELAY_NOCHANGE};
    RelayState relayState = (state == "on") ? RELAY_ON : RELAY_OFF;
    
    if (relayName == "hopper") {
      manualReq.hopperFan = relayState;
    } else if (relayName == "auger") {
      manualReq.auger = relayState;
    } else if (relayName == "ignite") {
      manualReq.igniter = relayState;
    } else if (relayName == "blower") {
      manualReq.blowerFan = relayState;
    } else {
      req->send(400, "text/plain", "Invalid relay name");
      return;
    }
    
    relay_request_manual(&manualReq);
    req->send(200, "text/plain", "Manual override: " + relayName + " = " + state);
  });

  server.on("/clear_manual", HTTP_GET, [](AsyncWebServerRequest *req) {
    relay_clear_manual();
    req->send(200, "text/plain", "Manual override cleared");
  });

  server.on("/emergency_stop", HTTP_GET, [](AsyncWebServerRequest *req) {
    relay_emergency_stop();
    grillRunning = false;
    req->send(200, "text/plain", "EMERGENCY STOP activated");
  });

  // PID tuning endpoint
  server.on("/set_pid", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!req->hasParam("kp") || !req->hasParam("ki") || !req->hasParam("kd")) {
      req->send(400, "text/plain", "Missing PID parameters");
      return;
    }
    
    float kp = req->getParam("kp")->value().toFloat();
    float ki = req->getParam("ki")->value().toFloat();
    float kd = req->getParam("kd")->value().toFloat();
    
    setPIDParameters(kp, ki, kd);
    req->send(200, "text/plain", "PID parameters updated");
  });

  // Debug control endpoints
  server.on("/set_individual_debug", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!req->hasParam("sensor") || !req->hasParam("enabled")) {
      req->send(400, "text/plain", "Missing parameters");
      return;
    }
    
    String sensor = req->getParam("sensor")->value();
    bool enabled = (req->getParam("enabled")->value() == "1");
    
    if (sensor == "grill") {
      setGrillDebug(enabled);
    } else if (sensor == "ambient") {
      setAmbientDebug(enabled);
    } else if (sensor == "meat") {
      setMeatProbesDebug(enabled);
    } else if (sensor == "relay") {
      setRelayDebug(enabled);
    } else if (sensor == "system") {
      setSystemDebug(enabled);
    } else {
      req->send(400, "text/plain", "Invalid sensor type");
      return;
    }
    
    req->send(200, "text/plain", sensor + " debug " + (enabled ? "enabled" : "disabled"));
  });

 
 // server.on("/cal_status", HTTP_GET, [](AsyncWebServerRequest *req) {
  //  String status = "MAX31865 RTD Sensor Status:\\n";
 //   status += "Temperature: " + String(grillSensor.readTemperatureF(), 1) + "°F\\n";
  //  status += "Resistance: " + String(grillSensor.readRTD(), 2) + "Ω\\n";
 //   status += "Connected: " + String(grillSensor.isConnected() ? "YES" : "NO") + "\\n";
 //   status += "Status: Professional RTD interface - minimal calibration needed";
    
  //  req->send(200, "text/plain", status);
  //});

  //server.on("/cal_reset", HTTP_POST, [](AsyncWebServerRequest *req) {
   // grillSensor.setCalibration(0.0, 1.0);
   // req->send(200, "text/plain", "MAX31865 calibration reset to defaults");
  //});

  // System diagnostics endpoint
  server.on("/diagnostics", HTTP_GET, [](AsyncWebServerRequest *req) {
    String diag = "System Diagnostics:\\n";
    diag += "Grill Temperature: " + String(readGrillTemperature(), 1) + "F (MAX31865)\\n";
    diag += "Ambient Temperature: " + String(readAmbientTemperature(), 1) + "F\\n";
    //diag += "MAX31865 Status: " + String(grillSensor.isConnected() ? "OK" : "ERROR") + "\\n";
    //if (grillSensor.hasFault()) {
    //  diag += "MAX31865 Fault: " + grillSensor.getFaultString() + "\\n";
   // }
    diag += "Grill Running: " + String(grillRunning ? "YES" : "NO") + "\\n";
    diag += "Free Memory: " + String(ESP.getFreeHeap()) + " bytes\\n";
    diag += "Uptime: " + String(millis() / 1000) + " seconds\\n";
    req->send(200, "text/plain", diag);
  });

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *req) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='utf-8'>";
    html += "<title>Reboot Controller</title>";
    html += "<style>";
    html += "body { background: #1a1a1a; color: #fff; font-family: Arial, sans-serif; padding: 20px; text-align: center; }";
    html += ".btn { padding: 20px 40px; background: #dc2626; color: white; border: none; border-radius: 10px; font-size: 1.2em; cursor: pointer; margin: 20px; }";
    html += "</style></head><body>";
    
    html += "<h1>🔄 Reboot Controller</h1>";
    html += "<p>⚠️ This will restart the ESP32 immediately.</p>";
    html += "<button class='btn' onclick='confirmReboot()'>REBOOT NOW</button>";
    html += "<br><a href='/' style='color: #60a5fa; margin-top: 20px; display: inline-block;'>← Cancel</a>";
    
    html += "<script>";
    html += "function confirmReboot() {";
    html += "  if (confirm('Are you sure you want to reboot?')) {";
    html += "    fetch('/do_reboot', {method: 'POST'});";
    html += "    document.body.innerHTML = '<h1>Rebooting...</h1>';";
    html += "  }";
    html += "}";
    html += "</script></body></html>";
    
    req->send(200, "text/html", html);
  });

  server.on("/do_reboot", HTTP_POST, [](AsyncWebServerRequest *req) {
    req->send(200, "text/plain", "Rebooting...");
    grillRunning = false;
    relay_emergency_stop();
    delay(1000);
    ESP.restart();
  });

  server.onNotFound([](AsyncWebServerRequest *req) {
    req->send(404, "text/plain", "Not Found");
  });

  // Initialize ElegantOTA
  ElegantOTA.begin(&server);
  
  ElegantOTA.onStart([]() {
    Serial.println("OTA update started!");
    grillRunning = false;
    relay_emergency_stop();
  });
  
  ElegantOTA.onEnd([](bool success) {
    if (success) {
      Serial.println("OTA update successful! Rebooting...");
      delay(2000);
      ESP.restart();
    } else {
      Serial.println("OTA update failed!");
    }
  });

// SPI Communication Test endpoint
server.on("/spi_test", HTTP_GET, [](AsyncWebServerRequest *req) {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>MAX31865 SPI Test</title>";
  html += "<style>";
  html += "body { background: #1a1a1a; color: #fff; font-family: 'Courier New', monospace; padding: 20px; }";
  html += ".container { max-width: 800px; margin: 0 auto; }";
  html += "h1 { color: #60a5fa; text-align: center; margin-bottom: 30px; }";
  html += ".test-section { background: rgba(255,255,255,0.1); padding: 20px; margin: 15px 0; border-radius: 10px; }";
  html += ".test-result { font-family: monospace; background: #2a2a2a; padding: 15px; border-radius: 5px; margin: 10px 0; }";
  html += ".success { border-left: 5px solid #059669; }";
  html += ".error { border-left: 5px solid #dc2626; }";
  html += ".warning { border-left: 5px solid #f59e0b; }";
  html += ".btn { padding: 15px 30px; background: #059669; color: white; border: none; border-radius: 5px; cursor: pointer; margin: 10px; }";
  html += ".btn:hover { background: #047857; }";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>🔧 MAX31865 SPI Communication Test</h1>";
  
  // Pin Configuration
  html += "<div class='test-section'>";
  html += "<h3>📍 Current Pin Configuration</h3>";
  html += "<div class='test-result'>";
  //html += "CS Pin: GPIO" + String(MAX31865_CS_PIN) + "<br>";
  //html += "CLK Pin: GPIO" + String(MAX31865_CLK_PIN) + "<br>";
  //html += "MOSI Pin: GPIO" + String(MAX31865_MOSI_PIN) + "<br>";
  //html += "MISO Pin: GPIO" + String(MAX31865_MISO_PIN) + "<br>";
  html += "</div>";
  html += "</div>";
  
  // Basic SPI Test
  html += "<div class='test-section'>";
  html += "<h3>🔌 SPI Communication Test</h3>";
  html += "<div id='spi-results' class='test-result'>Click 'Run Test' to start...</div>";
  html += "<button class='btn' onclick='runSPITest()'>🧪 Run SPI Test</button>";
  html += "</div>";
  
  // Register Dump
  html += "<div class='test-section'>";
  html += "<h3>📊 Register Values</h3>";
  html += "<div id='register-dump' class='test-result'>Click 'Read Registers' to view...</div>";
  html += "<button class='btn' onclick='readRegisters()'>📖 Read Registers</button>";
  html += "</div>";
  
  // Pin Test
  html += "<div class='test-section'>";
  html += "<h3>⚡ Pin Connectivity Test</h3>";
  html += "<div id='pin-test' class='test-result'>Click 'Test Pins' to check...</div>";
  html += "<button class='btn' onclick='testPins()'>🔍 Test Pins</button>";
  html += "</div>";
  
  // Quick Actions
  html += "<div class='test-section'>";
  html += "<h3>⚡ Quick Actions</h3>";
  html += "<button class='btn' onclick='resetSPI()'>🔄 Reset SPI</button>";
  html += "<button class='btn' onclick='clearFaults()'>🧹 Clear Faults</button>";
  html += "<button class='btn' onclick='location.reload()'>🔄 Refresh Page</button>";
  html += "</div>";
  
  html += "<a href='/' style='display: block; text-align: center; color: #60a5fa; margin: 20px;'>← Back to Dashboard</a>";
  html += "</div>";
  
  // JavaScript for testing
  html += "<script>";
  
  html += "function runSPITest() {";
  html += "  document.getElementById('spi-results').innerHTML = 'Running SPI test...';";
  html += "  fetch('/spi_test_run')";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      const resultDiv = document.getElementById('spi-results');";
  html += "      resultDiv.innerHTML = data.replace(/\\n/g, '<br>');";
  html += "      ";
  html += "      if (data.includes('WORKING')) {";
  html += "        resultDiv.className = 'test-result success';";
  html += "      } else if (data.includes('FAILED')) {";
  html += "        resultDiv.className = 'test-result error';";
  html += "      } else {";
  html += "        resultDiv.className = 'test-result warning';";
  html += "      }";
  html += "    })";
  html += "    .catch(err => {";
  html += "      document.getElementById('spi-results').innerHTML = 'Error: ' + err;";
  html += "      document.getElementById('spi-results').className = 'test-result error';";
  html += "    });";
  html += "}";
  
  html += "function readRegisters() {";
  html += "  document.getElementById('register-dump').innerHTML = 'Reading registers...';";
  html += "  fetch('/spi_register_dump')";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      document.getElementById('register-dump').innerHTML = data.replace(/\\n/g, '<br>');";
  html += "      document.getElementById('register-dump').className = 'test-result';";
  html += "    });";
  html += "}";
  
  html += "function testPins() {";
  html += "  document.getElementById('pin-test').innerHTML = 'Testing pin connectivity...';";
  html += "  fetch('/spi_pin_test')";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      document.getElementById('pin-test').innerHTML = data.replace(/\\n/g, '<br>');";
  html += "      document.getElementById('pin-test').className = 'test-result';";
  html += "    });";
  html += "}";
  
  html += "function resetSPI() {";
  html += "  fetch('/spi_reset')";
  html += "    .then(response => response.text())";
  html += "    .then(data => alert(data));";
  html += "}";
  
  html += "function clearFaults() {";
  html += "  fetch('/max31865_clear')";
  html += "    .then(response => response.text())";
  html += "    .then(data => alert(data));";
  html += "}";
  
  html += "</script>";
  html += "</body></html>";
  
  req->send(200, "text/html", html);
});

// SPI Test execution endpoint
//server.on("/spi_test_run", HTTP_GET, [](AsyncWebServerRequest *req) {
//  String result = grillSensor.getSPITestResults();
//  req->send(200, "text/plain", result);
//});

// Register dump endpoint
server.on("/spi_register_dump", HTTP_GET, [](AsyncWebServerRequest *req) {
  String dump = "MAX31865 Register Dump:\\n\\n";
  
  // Read all important registers
  uint8_t registers[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
  String regNames[] = {"Config", "RTD MSB", "RTD LSB", "High Fault MSB", 
                       "High Fault LSB", "Low Fault MSB", "Low Fault LSB", "Fault Status"};
  
  for (int i = 0; i < 8; i++) {
    // Read register
    digitalWrite(MAX31865_CS_PIN, LOW);
    delayMicroseconds(10);
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
    SPI.transfer(registers[i]);
    uint8_t value = SPI.transfer(0x00);
    SPI.endTransaction();
    delayMicroseconds(10);
    digitalWrite(MAX31865_CS_PIN, HIGH);
    
    dump += regNames[i] + " (0x" + String(registers[i], HEX) + "): 0x" + String(value, HEX) + "\\n";
    delay(10);
  }
  
  req->send(200, "text/plain", dump);
});

// Pin test endpoint
server.on("/spi_pin_test", HTTP_GET, [](AsyncWebServerRequest *req) {
  String result = "Pin Connectivity Test:\\n\\n";
  
  // Test CS pin
  result += "Testing CS Pin (GPIO" + String(MAX31865_CS_PIN) + "):\\n";
  pinMode(MAX31865_CS_PIN, OUTPUT);
  digitalWrite(MAX31865_CS_PIN, HIGH);
  delay(10);
  digitalWrite(MAX31865_CS_PIN, LOW);
  delay(10);
  digitalWrite(MAX31865_CS_PIN, HIGH);
  result += "CS pin toggle test: OK\\n\\n";
  
  // Test basic SPI transaction
  result += "Testing SPI Transaction:\\n";
  digitalWrite(MAX31865_CS_PIN, LOW);
  delayMicroseconds(10);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  SPI.transfer(0x00);  // Config register read
  uint8_t response = SPI.transfer(0x00);
  SPI.endTransaction();
  delayMicroseconds(10);
  digitalWrite(MAX31865_CS_PIN, HIGH);
  
  result += "SPI Response: 0x" + String(response, HEX) + "\\n";
  
  if (response == 0x00 || response == 0xFF) {
    result += "Status: NO COMMUNICATION\\n";
    result += "Check: MOSI/MISO wiring, power, CS pin\\n";
  } else {
    result += "Status: COMMUNICATION DETECTED\\n";
  }
  
  req->send(200, "text/plain", result);
});

// SPI reset endpoint
//server.on("/spi_reset", HTTP_GET, [](AsyncWebServerRequest *req) {
  // Reinitialize SPI
 // SPI.end();
 // delay(100);
 // SPI.begin(MAX31865_CLK_PIN, MAX31865_MISO_PIN, MAX31865_MOSI_PIN);
 // delay(100);
  
  // Reset CS pin
 // pinMode(MAX31865_CS_PIN, OUTPUT);
 // digitalWrite(MAX31865_CS_PIN, HIGH);
  
 // req->send(200, "text/plain", "SPI reset complete");
//});

  server.begin();
  Serial.println("Web server started with MAX31865 support");
}