// GrillWebServer.cpp - Complete version with all 6 temperatures and all endpoints
#include "Globals.h"
#include "Utility.h"
#include "PelletControl.h" 
#include "Ignition.h"
#include "RelayControl.h"
#include "OLEDDisplay.h"
#include "WiFiManager.h"
#include "TemperatureSensor.h"
#include <WiFi.h>
#include <Preferences.h>
#include <WiFiClient.h>

void setup_grill_server() {
  // Main dashboard with all 6 temperatures
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
    // Get grill temperature (PT100)
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
    html += "<div>Daniel Boone Controller</div>";
    html += "<div>IP: " + WiFi.localIP().toString() + "</div>";
    
    // Navigation links
    html += "<div style='margin: 15px 0;'>";
    html += "<a href='/wifi' class='link-btn'>WiFi Settings</a>";
    html += "<a href='/manual' class='link-btn'>Manual Control</a>";
    html += "<a href='/pid' class='link-btn'>PID Tuning</a>";
    html += "</div>";
    html += "</div>";
    
    // Main grill temperature display (prominent)
    html += "<div class='grill-temp'>";
    if (grillTemp > 0) {
      html += "<div class='grill-temp-main' id='grill-temp'>" + String(grillTemp, 1) + "&deg;F</div>";
    } else {
      html += "<div class='grill-temp-main temp-invalid' id='grill-temp'>SENSOR ERROR</div>";
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
    if (grillTemp > 0) {
      html += "<div class='temp-value' id='grill-temp-card'>" + String(grillTemp, 1) + "&deg;F</div>";
    } else {
      html += "<div class='temp-value temp-invalid'>ERROR</div>";
    }
    html += "<div class='temp-type'>PT100 RTD</div>";
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

    // JavaScript for functionality and auto-updates
    html += "<script>";
    html += "function setTemp(temp) {";
    html += "  fetch('/set_temp?temp=' + temp)";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      document.getElementById('setpoint').textContent = temp;";
    html += "      alert('Set to ' + temp + ' degrees F');";
    html += "    });";
    html += "}";
    
    html += "function startGrill() {";
    html += "  fetch('/start')";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      alert(data);";
    html += "      setTimeout(() => location.reload(), 1000);";
    html += "    });";
    html += "}";
    
    html += "function stopGrill() {";
    html += "  fetch('/stop')";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      alert(data);";
    html += "      setTimeout(() => location.reload(), 1000);";
    html += "    });";
    html += "}";
    
    html += "function adjustTemp() {";
    html += "  const newTemp = prompt('Enter target temperature (150-500F):', document.getElementById('setpoint').textContent);";
    html += "  if (newTemp && newTemp >= 150 && newTemp <= 500) {";
    html += "    setTemp(parseInt(newTemp));";
    html += "  }";
    html += "}";
    
    // Auto-update all temperatures every 5 seconds
    html += "setInterval(() => {";
    html += "  fetch('/status_all')";
    html += "    .then(response => response.json())";
    html += "    .then(data => {";
    // Update grill temperature
    html += "      if (data.grillTemp > 0) {";
    html += "        document.getElementById('grill-temp').innerHTML = data.grillTemp.toFixed(1) + '&deg;F';";
    html += "        document.getElementById('grill-temp-card').innerHTML = data.grillTemp.toFixed(1) + '&deg;F';";
    html += "        document.getElementById('grill-temp').className = 'grill-temp-main';";
    html += "      } else {";
    html += "        document.getElementById('grill-temp').innerHTML = 'SENSOR ERROR';";
    html += "        document.getElementById('grill-temp-card').innerHTML = 'ERROR';";
    html += "        document.getElementById('grill-temp').className = 'grill-temp-main temp-invalid';";
    html += "      }";
    
    // Update ambient temperature
    html += "      if (data.ambientTemp > -900) {";
    html += "        document.getElementById('ambient-temp').innerHTML = data.ambientTemp.toFixed(1) + '&deg;F';";
    html += "        document.getElementById('ambient-temp').className = 'temp-value';";
    html += "      } else {";
    html += "        document.getElementById('ambient-temp').innerHTML = 'N/A';";
    html += "        document.getElementById('ambient-temp').className = 'temp-value temp-invalid';";
    html += "      }";
    
    // Update meat probe temperatures
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
    
    // Update other status elements
    html += "      document.getElementById('setpoint').textContent = data.setpoint;";
    html += "      document.getElementById('status').textContent = data.status;";
    html += "      document.getElementById('ign-dot').className = 'relay-dot ' + (data.ignOn ? 'relay-on' : 'relay-off');";
    html += "      document.getElementById('aug-dot').className = 'relay-dot ' + (data.augerOn ? 'relay-on' : 'relay-off');";
    html += "      document.getElementById('hop-dot').className = 'relay-dot ' + (data.hopperOn ? 'relay-on' : 'relay-off');";
    html += "      document.getElementById('blo-dot').className = 'relay-dot ' + (data.blowerOn ? 'relay-on' : 'relay-off');";
    html += "    })";
    html += "    .catch(err => console.log('Update failed:', err));";
    html += "}, 5000);";
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
    html += "⚠️ <strong>WARNING:</strong> Manual control overrides automatic safety systems. ";
    html += "Use only for testing. Safety interlocks remain active.";
    html += "</div>";
    
    // Igniter Control
    html += "<div class='relay-control'>";
    html += "<div class='relay-status'>";
    html += "<div class='status-dot " + String(ignOn ? "status-on" : "status-off") + "' id='ign-status'></div>";
    html += "<h3>Igniter (Pin " + String(RELAY_IGNITER_PIN) + ")</h3>";
    html += "</div>";
    html += "<button class='btn' onclick='controlRelay(\"ignite\", \"on\")'>Turn ON</button>";
    html += "<button class='btn btn-danger' onclick='controlRelay(\"ignite\", \"off\")'>Turn OFF</button>";
    html += "</div>";
    
    // Auger Control
    html += "<div class='relay-control'>";
    html += "<div class='relay-status'>";
    html += "<div class='status-dot " + String(augerOn ? "status-on" : "status-off") + "' id='aug-status'></div>";
    html += "<h3>Auger Motor (Pin " + String(RELAY_AUGER_PIN) + ")</h3>";
    html += "</div>";
    html += "<button class='btn' onclick='controlRelay(\"auger\", \"on\")'>Turn ON</button>";
    html += "<button class='btn btn-danger' onclick='controlRelay(\"auger\", \"off\")'>Turn OFF</button>";
    html += "</div>";
    
    // Hopper Fan Control
    html += "<div class='relay-control'>";
    html += "<div class='relay-status'>";
    html += "<div class='status-dot " + String(hopperOn ? "status-on" : "status-off") + "' id='hop-status'></div>";
    html += "<h3>Hopper Fan (Pin " + String(RELAY_HOPPER_FAN_PIN) + ")</h3>";
    html += "</div>";
    html += "<button class='btn' onclick='controlRelay(\"hopper\", \"on\")'>Turn ON</button>";
    html += "<button class='btn btn-danger' onclick='controlRelay(\"hopper\", \"off\")'>Turn OFF</button>";
    html += "</div>";
    
    // Blower Fan Control
    html += "<div class='relay-control'>";
    html += "<div class='relay-status'>";
    html += "<div class='status-dot " + String(blowerOn ? "status-on" : "status-off") + "' id='blo-status'></div>";
    html += "<h3>Blower Fan (Pin " + String(RELAY_BLOWER_FAN_PIN) + ")</h3>";
    html += "</div>";
    html += "<button class='btn' onclick='controlRelay(\"blower\", \"on\")'>Turn ON</button>";
    html += "<button class='btn btn-danger' onclick='controlRelay(\"blower\", \"off\")'>Turn OFF</button>";
    html += "</div>";
    
    // Emergency Controls
    html += "<div style='text-align: center; margin: 30px 0;'>";
    html += "<button class='btn btn-warning' onclick='clearManual()'>Clear All Manual Overrides</button>";
    html += "<button class='btn btn-danger' onclick='emergencyStop()'>EMERGENCY STOP</button>";
    html += "</div>";
    
    html += "<a href='/' class='btn' style='display: block; text-align: center; margin: 20px 0; text-decoration: none;'>Back to Dashboard</a>";
    html += "</div>";

    // JavaScript
    html += "<script>";
    html += "function controlRelay(relay, state) {";
    html += "  fetch('/control?relay=' + relay + '&state=' + state)";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      alert(data);";
    html += "      setTimeout(() => location.reload(), 1000);";
    html += "    })";
    html += "    .catch(err => alert('Error: ' + err));";
    html += "}";
    
    html += "function clearManual() {";
    html += "  fetch('/clear_manual')";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      alert(data);";
    html += "      setTimeout(() => location.reload(), 1000);";
    html += "    });";
    html += "}";
    
    html += "function emergencyStop() {";
    html += "  if (confirm('EMERGENCY STOP - Are you sure?')) {";
    html += "    fetch('/emergency_stop')";
    html += "      .then(response => response.text())";
    html += "      .then(data => {";
    html += "        alert(data);";
    html += "        setTimeout(() => location.reload(), 1000);";
    html += "      });";
    html += "  }";
    html += "}";
    
    // Auto-update relay status
    html += "setInterval(() => {";
    html += "  fetch('/status_all')";
    html += "    .then(response => response.json())";
    html += "    .then(data => {";
    html += "      document.getElementById('ign-status').className = 'status-dot ' + (data.ignOn ? 'status-on' : 'status-off');";
    html += "      document.getElementById('aug-status').className = 'status-dot ' + (data.augerOn ? 'status-on' : 'status-off');";
    html += "      document.getElementById('hop-status').className = 'status-dot ' + (data.hopperOn ? 'status-on' : 'status-off');";
    html += "      document.getElementById('blo-status').className = 'status-dot ' + (data.blowerOn ? 'status-on' : 'status-off');";
    html += "    });";
    html += "}, 2000);";
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
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>PID Tuning - Grill Controller</title>";
    html += "<style>";
    html += "body { background: #1a1a1a; color: #fff; font-family: Arial, sans-serif; padding: 20px; }";
    html += ".container { max-width: 600px; margin: 0 auto; }";
    html += "h1 { color: #60a5fa; text-align: center; margin-bottom: 30px; }";
    html += ".form-group { margin: 20px 0; }";
    html += "label { display: block; margin-bottom: 5px; font-weight: bold; }";
    html += "input { width: 100%; padding: 10px; font-size: 1em; border-radius: 5px; border: 1px solid #555; background: #333; color: #fff; }";
    html += ".btn { padding: 15px 30px; background: #059669; color: white; border: none; border-radius: 5px; font-size: 1.1em; cursor: pointer; margin: 10px 5px; }";
    html += ".btn-danger { background: #dc2626; }";
    html += ".info-box { background: rgba(255,255,255,0.1); padding: 15px; border-radius: 5px; margin: 15px 0; }";
    html += ".current-values { background: #059669; padding: 15px; border-radius: 5px; margin: 15px 0; }";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>PID Tuning</h1>";
    
    html += "<div class='current-values'>";
    html += "<h3>Current PID Values</h3>";
    html += "<p><strong>Kp (Proportional):</strong> " + String(kp, 3) + "</p>";
    html += "<p><strong>Ki (Integral):</strong> " + String(ki, 4) + "</p>";
    html += "<p><strong>Kd (Derivative):</strong> " + String(kd, 3) + "</p>";
    html += "<p><strong>Pellet Status:</strong> " + pellet_get_status() + "</p>";
    html += "</div>";
    
    html += "<form onsubmit='updatePID(event)'>";
    html += "<div class='form-group'>";
    html += "<label>Kp (Proportional Gain):</label>";
    html += "<input type='number' id='kp' step='0.001' value='" + String(kp, 3) + "' required>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label>Ki (Integral Gain):</label>";
    html += "<input type='number' id='ki' step='0.0001' value='" + String(ki, 4) + "' required>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label>Kd (Derivative Gain):</label>";
    html += "<input type='number' id='kd' step='0.001' value='" + String(kd, 3) + "' required>";
    html += "</div>";
    
    html += "<button type='submit' class='btn'>Update PID Parameters</button>";
    html += "<button type='button' class='btn btn-danger' onclick='resetPID()'>Reset to Defaults</button>";
    html += "</form>";
    
    html += "<div class='info-box'>";
    html += "<h3>PID Tuning Guide</h3>";
    html += "<p><strong>Kp:</strong> Higher values make the system respond faster but can cause oscillation.</p>";
    html += "<p><strong>Ki:</strong> Eliminates steady-state error but can cause instability if too high.</p>";
    html += "<p><strong>Kd:</strong> Reduces overshoot and helps stability but makes system sensitive to noise.</p>";
    html += "<p><strong>Start with:</strong> Kp=1.5, Ki=0.01, Kd=0.5 and adjust gradually.</p>";
    html += "</div>";
    
    html += "<a href='/' class='btn' style='display: block; text-align: center; margin: 20px 0; text-decoration: none;'>Back to Dashboard</a>";
    html += "</div>";

    // JavaScript
    html += "<script>";
    html += "function updatePID(event) {";
    html += "  event.preventDefault();";
    html += "  const kp = document.getElementById('kp').value;";
    html += "  const ki = document.getElementById('ki').value;";
    html += "  const kd = document.getElementById('kd').value;";
    html += "  ";
    html += "  fetch('/set_pid?kp=' + kp + '&ki=' + ki + '&kd=' + kd)";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      alert(data);";
    html += "      setTimeout(() => location.reload(), 1000);";
    html += "    });";
    html += "}";
    
    html += "function resetPID() {";
    html += "  if (confirm('Reset PID to default values? (Kp=1.5, Ki=0.01, Kd=0.5)')) {";
    html += "    fetch('/set_pid?kp=1.5&ki=0.01&kd=0.5')";
    html += "      .then(response => response.text())";
    html += "      .then(data => {";
    html += "        alert(data);";
    html += "        setTimeout(() => location.reload(), 1000);";
    html += "      });";
    html += "  }";
    html += "}";
    html += "</script>";
    
    html += "</body></html>";
    
    req->send(200, "text/html", html);
  });

  // Enhanced status endpoint with all 6 temperatures
  server.on("/status_all", HTTP_GET, [](AsyncWebServerRequest *req) {
    // Get all temperatures
    double grillTemp = readGrillTemperature();
    double ambientTemp = readAmbientTemperature();
    float meat1 = tempSensor.getFoodTemperature(1);
    float meat2 = tempSensor.getFoodTemperature(2);
    float meat3 = tempSensor.getFoodTemperature(3);
    float meat4 = tempSensor.getFoodTemperature(4);
    
    // Get relay states
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

  // Control endpoints - integrated with complex systems
  server.on("/start", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (grillRunning) {
      req->send(200, "text/plain", "Grill already running");
      return;
    }
    
    grillRunning = true;
    double currentTemp = readGrillTemperature();
    
    // Start the intelligent ignition sequence
    ignition_start(currentTemp);
    
    Serial.printf("Grill started - ignition sequence initiated at %.1f°F\n", currentTemp);
    req->send(200, "text/plain", "Grill started - ignition sequence initiated");
  });

  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!grillRunning) {
      req->send(200, "text/plain", "Grill already stopped");
      return;
    }
    
    grillRunning = false;
    
    // Stop ignition sequence
    ignition_stop();
    
    // Clear any manual overrides and stop heating
    relay_clear_manual();
    
    // Keep fans on briefly for cooling
    RelayRequest cooldownReq = {RELAY_OFF, RELAY_OFF, RELAY_ON, RELAY_ON};
    relay_request_auto(&cooldownReq);
    
    Serial.println("Grill stopped - cooling down");
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
    Serial.printf("Manual override: %s = %s\n", relayName.c_str(), state.c_str());
    req->send(200, "text/plain", "Manual override: " + relayName + " = " + state);
  });

  server.on("/clear_manual", HTTP_GET, [](AsyncWebServerRequest *req) {
    relay_clear_manual();
    Serial.println("Manual override cleared");
    req->send(200, "text/plain", "Manual override cleared");
  });

  server.on("/emergency_stop", HTTP_GET, [](AsyncWebServerRequest *req) {
    relay_emergency_stop();
    grillRunning = false;
    Serial.println("EMERGENCY STOP activated via web interface");
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
    req->send(200, "text/plain", "PID parameters updated: Kp=" + String(kp, 3) + ", Ki=" + String(ki, 4) + ", Kd=" + String(kd, 3));
  });

  // System diagnostics endpoint
  server.on("/diagnostics", HTTP_GET, [](AsyncWebServerRequest *req) {
    String diag = "System Diagnostics:\\n";
    diag += "Grill Temperature: " + String(readGrillTemperature(), 1) + "F\\n";
    diag += "Ambient Temperature: " + String(readAmbientTemperature(), 1) + "F\\n";
    diag += "Meat Probe 1: " + String(tempSensor.getFoodTemperature(1), 1) + "F\\n";
    diag += "Meat Probe 2: " + String(tempSensor.getFoodTemperature(2), 1) + "F\\n";
    diag += "Meat Probe 3: " + String(tempSensor.getFoodTemperature(3), 1) + "F\\n";
    diag += "Meat Probe 4: " + String(tempSensor.getFoodTemperature(4), 1) + "F\\n";
    diag += "Grill Running: " + String(grillRunning ? "YES" : "NO") + "\\n";
    diag += "Ignition: " + ignition_get_status_string() + "\\n";
    diag += "Pellet Control: " + pellet_get_status() + "\\n";
    diag += "Relay Safety: " + String(relay_is_safe_state() ? "SAFE" : "WARNING") + "\\n";
    diag += "Free Memory: " + String(ESP.getFreeHeap()) + " bytes\\n";
    diag += "Uptime: " + String(millis() / 1000) + " seconds\\n";
    req->send(200, "text/plain", diag);
  });

  // Direct relay test endpoint (bypasses all safety)
  server.on("/relay_test", HTTP_GET, [](AsyncWebServerRequest *req) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='utf-8'>";
    html += "<title>Relay Direct Test</title>";
    html += "<style>body{background:#000;color:#fff;font-family:Arial;padding:20px;}";
    html += ".btn{padding:15px;margin:10px;background:#4CAF50;color:white;border:none;border-radius:5px;cursor:pointer;}";
    html += ".btn-off{background:#f44336;}</style></head><body>";
    
    html += "<h1>Direct Relay Test</h1>";
    html += "<p>This bypasses all safety systems - USE ONLY FOR TESTING!</p>";
    
    html += "<h3>Igniter (Pin " + String(RELAY_IGNITER_PIN) + ")</h3>";
    html += "<button class='btn' onclick='directRelay(" + String(RELAY_IGNITER_PIN) + ", 1)'>Force ON</button>";
    html += "<button class='btn btn-off' onclick='directRelay(" + String(RELAY_IGNITER_PIN) + ", 0)'>Force OFF</button>";
    html += "<span id='ign-status'>Status: " + String(digitalRead(RELAY_IGNITER_PIN) ? "ON" : "OFF") + "</span><br><br>";
    
    html += "<h3>Auger (Pin " + String(RELAY_AUGER_PIN) + ")</h3>";
    html += "<button class='btn' onclick='directRelay(" + String(RELAY_AUGER_PIN) + ", 1)'>Force ON</button>";
    html += "<button class='btn btn-off' onclick='directRelay(" + String(RELAY_AUGER_PIN) + ", 0)'>Force OFF</button>";
    html += "<span id='aug-status'>Status: " + String(digitalRead(RELAY_AUGER_PIN) ? "ON" : "OFF") + "</span><br><br>";
    
    html += "<h3>Hopper Fan (Pin " + String(RELAY_HOPPER_FAN_PIN) + ")</h3>";
    html += "<button class='btn' onclick='directRelay(" + String(RELAY_HOPPER_FAN_PIN) + ", 1)'>Force ON</button>";
    html += "<button class='btn btn-off' onclick='directRelay(" + String(RELAY_HOPPER_FAN_PIN) + ", 0)'>Force OFF</button>";
    html += "<span id='hop-status'>Status: " + String(digitalRead(RELAY_HOPPER_FAN_PIN) ? "ON" : "OFF") + "</span><br><br>";
    
    html += "<h3>Blower Fan (Pin " + String(RELAY_BLOWER_FAN_PIN) + ")</h3>";
    html += "<button class='btn' onclick='directRelay(" + String(RELAY_BLOWER_FAN_PIN) + ", 1)'>Force ON</button>";
    html += "<button class='btn btn-off' onclick='directRelay(" + String(RELAY_BLOWER_FAN_PIN) + ", 0)'>Force OFF</button>";
    html += "<span id='blo-status'>Status: " + String(digitalRead(RELAY_BLOWER_FAN_PIN) ? "ON" : "OFF") + "</span><br><br>";
    
    html += "<a href='/' style='color: white;'>Back to Dashboard</a>";
    
    html += "<script>";
    html += "function directRelay(pin, state) {";
    html += "  fetch('/direct_relay?pin=' + pin + '&state=' + state)";
    html += "    .then(response => response.text())";
    html += "    .then(data => { alert(data); setTimeout(() => location.reload(), 500); });";
    html += "}";
    html += "</script></body></html>";
    
    req->send(200, "text/html", html);
  });

  // Direct relay control endpoint (bypasses all safety)
  server.on("/direct_relay", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!req->hasParam("pin") || !req->hasParam("state")) {
      req->send(400, "text/plain", "Missing pin or state parameter");
      return;
    }
    
    int pin = req->getParam("pin")->value().toInt();
    int state = req->getParam("state")->value().toInt();
    
    // Direct pin control - bypasses all systems
    digitalWrite(pin, state ? HIGH : LOW);
    
    Serial.printf("DIRECT RELAY TEST: Pin %d set to %s\n", pin, state ? "HIGH" : "LOW");
    Serial.printf("Pin readback: %s\n", digitalRead(pin) ? "HIGH" : "LOW");
    
    req->send(200, "text/plain", "Pin " + String(pin) + " set to " + String(state ? "HIGH" : "LOW"));
  });

  // Legacy status endpoint (for compatibility)
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *req) {
    double temp = readGrillTemperature();
    bool ignOn = digitalRead(RELAY_IGNITER_PIN) == HIGH;
    bool augerOn = digitalRead(RELAY_AUGER_PIN) == HIGH;
    bool hopperOn = digitalRead(RELAY_HOPPER_FAN_PIN) == HIGH;
    bool blowerOn = digitalRead(RELAY_BLOWER_FAN_PIN) == HIGH;
    String status = getStatus(temp);

    String json = "{";
    json += "\"temp\":" + String(temp, 1) + ",";
    json += "\"setpoint\":" + String((int)setpoint) + ",";
    json += "\"status\":\"" + status + "\",";
    json += "\"ignOn\":" + String(ignOn ? "true" : "false") + ",";
    json += "\"augerOn\":" + String(augerOn ? "true" : "false") + ",";
    json += "\"hopperOn\":" + String(hopperOn ? "true" : "false") + ",";
    json += "\"blowerOn\":" + String(blowerOn ? "true" : "false");
    json += "}";
    req->send(200, "application/json", json);
  });

  server.onNotFound([](AsyncWebServerRequest *req) {
    req->send(404, "text/plain", "Not Found");
  });

  server.begin();
  Serial.println("Web server started with all endpoints");
}