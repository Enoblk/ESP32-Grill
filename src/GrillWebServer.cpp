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
  // Main dashboard with all 6 temperatures and REAL-TIME relay updates
// REPLACE the main dashboard route in GrillWebServer.cpp with this properly escaped version:

  // Replace the main dashboard route in GrillWebServer.cpp with this version that includes Prime button

server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
  // Get all sensor data
  double grillTemp = readGrillTemperature();
  double ambientTemp = readAmbientTemperature();
  float meat1 = tempSensor.getFoodTemperature(1);
  float meat2 = tempSensor.getFoodTemperature(2);
  float meat3 = tempSensor.getFoodTemperature(3);
  float meat4 = tempSensor.getFoodTemperature(4);
  
  String status = getStatus(grillTemp);
  bool ignOn = digitalRead(RELAY_IGNITER_PIN) == HIGH;
  bool augOn = digitalRead(RELAY_AUGER_PIN) == HIGH;
  bool hopOn = digitalRead(RELAY_HOPPER_FAN_PIN) == HIGH;
  bool bloOn = digitalRead(RELAY_BLOWER_FAN_PIN) == HIGH;
  
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
  html += ".grill-temp { background: rgba(255,255,255,0.15); border-radius: 15px; ";
  html += "padding: 20px; margin-bottom: 20px; text-align: center; border: 2px solid #4ade80; }";
  html += ".grill-temp-main { font-size: 3em; font-weight: bold; margin-bottom: 10px; }";
  html += ".grill-temp-set { font-size: 1.2em; margin-bottom: 10px; }";
  html += ".status { font-size: 1.3em; font-weight: bold; padding: 10px; border-radius: 10px; }";
  html += ".temp-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 15px; margin: 20px 0; }";
  html += ".temp-card { background: rgba(255,255,255,0.1); padding: 15px; border-radius: 10px; text-align: center; }";
  html += ".temp-card.grill { border: 2px solid #4ade80; }";
  html += ".temp-card.ambient { border: 2px solid #60a5fa; }";
  html += ".temp-card.meat { border: 2px solid #f59e0b; }";
  html += ".temp-card h3 { font-size: 0.9em; margin-bottom: 8px; opacity: 0.8; }";
  html += ".temp-value { font-size: 1.8em; font-weight: bold; margin-bottom: 5px; }";
  html += ".temp-type { font-size: 0.8em; opacity: 0.7; }";
  html += ".temp-invalid { color: #ef4444; }";
  html += ".controls { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 10px; margin: 20px 0; }";
  html += ".btn { padding: 15px; font-size: 1.1em; font-weight: bold; border: none; ";
  html += "border-radius: 10px; color: white; cursor: pointer; text-align: center; text-decoration: none; display: block; }";
  html += ".btn:disabled { opacity: 0.6; cursor: not-allowed; }";
  html += ".btn-primary { background: #667eea; }";
  html += ".btn-danger { background: #f093fb; }";
  html += ".btn-success { background: #4facfe; }";
  html += ".btn-warning { background: #fbbf24; }";
  html += ".btn-prime { background: #8b5cf6; }";
  html += ".temp-presets { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; margin: 15px 0; }";
  html += ".temp-preset { padding: 12px; background: rgba(255,255,255,0.1); border: none; ";
  html += "border-radius: 8px; color: white; cursor: pointer; }";
  html += ".relays { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin: 20px 0; }";
  html += ".relay { display: flex; align-items: center; padding: 12px; ";
  html += "background: rgba(255,255,255,0.1); border-radius: 10px; }";
  html += ".relay-dot { width: 12px; height: 12px; border-radius: 50%; margin-right: 10px; transition: all 0.3s ease; }";
  html += ".relay-on { background: #4ade80; box-shadow: 0 0 10px #4ade80; }";
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
  html += "<div>Daniel Boone Controller - PiFire Auger</div>";
  html += "<div>IP: " + WiFi.localIP().toString() + "</div>";
  html += "<div style='margin: 15px 0;'>";
  html += "<a href='/wifi' class='link-btn'>WiFi Settings</a>";
  html += "<a href='/manual' class='link-btn'>Manual Control</a>";
  html += "<a href='/pid' class='link-btn'>PID Tuning</a>";
  html += "<a href='/debug' class='link-btn'>Debug</a>";
  html += "<a href='/update' class='link-btn'>OTA Update</a>";
  html += "<a href='/grill_debug' class='link-btn'>Debug Info</a>";
  html += "</div></div>";
  
  html += "<div style='margin: 10px 0; text-align: center;'>";
  html += "<label style='color: #bbb; font-size: 0.9em;'>Update Speed: ";
  html += "<select id='updateSpeed' onchange='changeUpdateSpeed()' style='background: #333; color: #fff; border: 1px solid #555; border-radius: 3px; padding: 2px;'>";
  html += "<option value='1000'>Fast (1s)</option>";
  html += "<option value='1500' selected>Normal (1.5s)</option>";
  html += "<option value='3000'>Slow (3s)</option>";
  html += "<option value='5000'>Very Slow (5s)</option>";
  html += "<option value='0'>Paused</option>";
  html += "</select></label></div>";

  // Main grill temperature display
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
  
  // Temperature grid
  html += "<div class='temp-grid'>";
  
  // Grill Temperature Card
  html += "<div class='temp-card grill'><h3>GRILL TEMPERATURE</h3>";
  if (isValidTemperature(grillTemp)) {
    html += "<div class='temp-value' id='grill-temp-card'>" + String(grillTemp, 1) + "&deg;F</div>";
  } else {
    html += "<div class='temp-value temp-invalid'>ERROR</div>";
  }
  html += "<div class='temp-type'>MAX31865 RTD</div></div>";
  
  // Ambient Temperature Card
  html += "<div class='temp-card ambient'><h3>AMBIENT</h3>";
  if (ambientTemp > -900) {
    html += "<div class='temp-value' id='ambient-temp'>" + String(ambientTemp, 1) + "&deg;F</div>";
  } else {
    html += "<div class='temp-value temp-invalid'>N/A</div>";
  }
  html += "<div class='temp-type'>10K NTC</div></div>";
  
  // Meat Probe Cards
  html += "<div class='temp-card meat'><h3>MEAT PROBE 1</h3>";
  if (meat1 > -900) {
    html += "<div class='temp-value' id='meat1-temp'>" + String(meat1, 1) + "&deg;F</div>";
  } else {
    html += "<div class='temp-value temp-invalid'>N/A</div>";
  }
  html += "<div class='temp-type'>1K NTC</div></div>";
  
  html += "<div class='temp-card meat'><h3>MEAT PROBE 2</h3>";
  if (meat2 > -900) {
    html += "<div class='temp-value' id='meat2-temp'>" + String(meat2, 1) + "&deg;F</div>";
  } else {
    html += "<div class='temp-value temp-invalid'>N/A</div>";
  }
  html += "<div class='temp-type'>1K NTC</div></div>";
  
  html += "<div class='temp-card meat'><h3>MEAT PROBE 3</h3>";
  if (meat3 > -900) {
    html += "<div class='temp-value' id='meat3-temp'>" + String(meat3, 1) + "&deg;F</div>";
  } else {
    html += "<div class='temp-value temp-invalid'>N/A</div>";
  }
  html += "<div class='temp-type'>1K NTC</div></div>";
  
  html += "<div class='temp-card meat'><h3>MEAT PROBE 4</h3>";
  if (meat4 > -900) {
    html += "<div class='temp-value' id='meat4-temp'>" + String(meat4, 1) + "&deg;F</div>";
  } else {
    html += "<div class='temp-value temp-invalid'>N/A</div>";
  }
  html += "<div class='temp-type'>1K NTC</div></div>";
  html += "</div>"; // End temp-grid
  
  // Temperature presets
  html += "<div class='temp-presets'>";
  html += "<button class='btn temp-preset' onclick='setTemp(225)'>225&deg;F Low</button>";
  html += "<button class='btn temp-preset' onclick='setTemp(275)'>275&deg;F Med</button>";
  html += "<button class='btn temp-preset' onclick='setTemp(325)'>325&deg;F High</button>";
  html += "<button class='btn temp-preset' onclick='setTemp(200)'>200&deg;F Warm</button>";
  html += "<button class='btn temp-preset' onclick='setTemp(250)'>250&deg;F Smoke</button>";
  html += "<button class='btn temp-preset' onclick='setTemp(375)'>375&deg;F Sear</button>";
  html += "</div>";
  
  // Control buttons - NOW WITH 3 COLUMNS INCLUDING PRIME
  html += "<div class='controls' id='controls'>";
  if (grillRunning) {
    html += "<button class='btn btn-danger' onclick='stopGrill()'>STOP Grill</button>";
    html += "<button class='btn btn-primary' onclick='adjustTemp()'>Adjust Temp</button>";
    html += "<button class='btn btn-prime' disabled>PRIME (Grill Running)</button>";
  } else {
    html += "<button class='btn btn-success' onclick='startGrill()'>START Grill</button>";
    html += "<button class='btn btn-primary' onclick='adjustTemp()'>Adjust Temp</button>";
    html += "<button class='btn btn-prime' onclick='primeAuger()'>🌾 PRIME (30s)</button>";
  }
  html += "</div>";
  
  // Relay status
  html += "<div class='relays'>";
  html += "<div class='relay'><div class='relay-dot " + String(ignOn ? "relay-on" : "relay-off") + "' id='igniter-dot'></div><span>Igniter</span></div>";
  html += "<div class='relay'><div class='relay-dot " + String(augOn ? "relay-on" : "relay-off") + "' id='auger-dot'></div><span>Auger</span></div>";
  html += "<div class='relay'><div class='relay-dot " + String(hopOn ? "relay-on" : "relay-off") + "' id='hopper-dot'></div><span>Hopper Fan</span></div>";
  html += "<div class='relay'><div class='relay-dot " + String(bloOn ? "relay-on" : "relay-off") + "' id='blower-dot'></div><span>Blower Fan</span></div>";
  html += "</div>";
  html += "</div>"; // End container

  // JavaScript - properly escaped
  html += "<script>";
  html += "let updateInterval;";
  html += "let isPageVisible = true;";

  html += "document.addEventListener('visibilitychange', function() {";
  html += "  isPageVisible = !document.hidden;";
  html += "  if (isPageVisible) startRealTimeUpdates(); else stopRealTimeUpdates();";
  html += "});";

  html += "function startRealTimeUpdates() {";
  html += "  const speed = parseInt(document.getElementById('updateSpeed').value);";
  html += "  if (speed === 0) return;";
  html += "  if (updateInterval) clearInterval(updateInterval);";
  html += "  updateTemperatures();";
  html += "  updateInterval = setInterval(updateTemperatures, speed);";
  html += "}";

  html += "function stopRealTimeUpdates() {";
  html += "  if (updateInterval) { clearInterval(updateInterval); updateInterval = null; }";
  html += "}";

  html += "function updateTemperatures() {";
  html += "  if (!isPageVisible) return;";
  html += "  fetch('/status_all').then(response => {";
  html += "    if (!response.ok) throw new Error('Network error');";
  html += "    return response.json();";
  html += "  }).then(data => {";
  html += "    const grillTempElement = document.getElementById('grill-temp-main');";
  html += "    const grillTempCardElement = document.getElementById('grill-temp-card');";
  html += "    if (data.grillTemp > 0) {";
  html += "      const newTemp = data.grillTemp.toFixed(1);";
  html += "      grillTempElement.innerHTML = newTemp + '&deg;F';";
  html += "      grillTempCardElement.innerHTML = newTemp + '&deg;F';";
  html += "      grillTempElement.className = 'temp-value';";
  html += "      grillTempCardElement.className = 'temp-value';";
  html += "    } else {";
  html += "      grillTempElement.innerHTML = 'ERROR';";
  html += "      grillTempCardElement.innerHTML = 'ERROR';";
  html += "      grillTempElement.className = 'temp-value temp-invalid';";
  html += "      grillTempCardElement.className = 'temp-value temp-invalid';";
  html += "    }";
  html += "    const ambientTempElement = document.getElementById('ambient-temp');";
  html += "    if (data.ambientTemp > -900) {";
  html += "      ambientTempElement.innerHTML = data.ambientTemp.toFixed(1) + '&deg;F';";
  html += "      ambientTempElement.className = 'temp-value';";
  html += "    } else {";
  html += "      ambientTempElement.innerHTML = 'N/A';";
  html += "      ambientTempElement.className = 'temp-value temp-invalid';";
  html += "    }";
  html += "    ['meat1', 'meat2', 'meat3', 'meat4'].forEach((probe, index) => {";
  html += "      const temp = data[probe + 'Temp'];";
  html += "      const element = document.getElementById(probe + '-temp');";
  html += "      if (temp > -900) {";
  html += "        element.innerHTML = temp.toFixed(1) + '&deg;F';";
  html += "        element.className = 'temp-value';";
  html += "      } else {";
  html += "        element.innerHTML = 'N/A';";
  html += "        element.className = 'temp-value temp-invalid';";
  html += "      }";
  html += "    });";
  html += "    document.getElementById('setpoint').textContent = data.setpoint;";
  html += "    document.getElementById('status').textContent = data.status;";
  html += "    const igniterDot = document.getElementById('igniter-dot');";
  html += "    const augerDot = document.getElementById('auger-dot');"; 
  html += "    const hopperDot = document.getElementById('hopper-dot');";
  html += "    const blowerDot = document.getElementById('blower-dot');";
  html += "    if (igniterDot) igniterDot.className = 'relay-dot ' + (data.ignOn ? 'relay-on' : 'relay-off');";
  html += "    if (augerDot) augerDot.className = 'relay-dot ' + (data.augerOn ? 'relay-on' : 'relay-off');";
  html += "    if (hopperDot) hopperDot.className = 'relay-dot ' + (data.hopperOn ? 'relay-on' : 'relay-off');";
  html += "    if (blowerDot) blowerDot.className = 'relay-dot ' + (data.blowerOn ? 'relay-on' : 'relay-off');";
  html += "    updateControlButtons(data.grillRunning);";
  html += "  }).catch(err => console.log('Update failed:', err));";
  html += "}";

  html += "function updateControlButtons(grillRunning) {";
  html += "  const controlsDiv = document.getElementById('controls');";
  html += "  if (grillRunning) {";
  html += "    controlsDiv.innerHTML = '<button class=\"btn btn-danger\" onclick=\"stopGrill()\">STOP Grill</button><button class=\"btn btn-primary\" onclick=\"adjustTemp()\">Adjust Temp</button><button class=\"btn btn-prime\" disabled>PRIME (Grill Running)</button>';";
  html += "  } else {";
  html += "    controlsDiv.innerHTML = '<button class=\"btn btn-success\" onclick=\"startGrill()\">START Grill</button><button class=\"btn btn-primary\" onclick=\"adjustTemp()\">Adjust Temp</button><button class=\"btn btn-prime\" onclick=\"primeAuger()\">🌾 PRIME (30s)</button>';";
  html += "  }";
  html += "}";

  html += "function changeUpdateSpeed() { stopRealTimeUpdates(); startRealTimeUpdates(); }";

  html += "function setTemp(temp) {";
  html += "  document.getElementById('setpoint').textContent = temp;";
  html += "  fetch('/set_temp?temp=' + temp).then(response => response.text()).then(data => {";
  html += "    console.log('Temperature set');";
  html += "  }).catch(error => alert('Error setting temperature'));";
  html += "}";

  html += "function startGrill() {";
  html += "  const button = event.target;";
  html += "  button.disabled = true;";
  html += "  button.textContent = 'Starting...';";
  html += "  fetch('/start').then(response => {";
  html += "    if (!response.ok) throw new Error('HTTP ' + response.status);";
  html += "    return response.text();";
  html += "  }).then(data => {";
  html += "    alert('Grill Started: ' + data);";
  html += "    updateTemperatures();";
  html += "  }).catch(error => {";
  html += "    alert('Error starting grill: ' + error.message);";
  html += "    button.disabled = false;";
  html += "    button.textContent = 'START Grill';";
  html += "  });";
  html += "}";

  html += "function stopGrill() {";
  html += "  if (!confirm('Stop the grill?')) return;";
  html += "  const button = event.target;";
  html += "  button.disabled = true;";
  html += "  button.textContent = 'Stopping...';";
  html += "  fetch('/stop').then(response => {";
  html += "    if (!response.ok) throw new Error('HTTP ' + response.status);";
  html += "    return response.text();";
  html += "  }).then(data => {";
  html += "    alert('Grill Stopped: ' + data);";
  html += "    updateTemperatures();";
  html += "  }).catch(error => {";
  html += "    alert('Error stopping grill: ' + error.message);";
  html += "    button.disabled = false;";
  html += "    button.textContent = 'STOP Grill';";
  html += "  });";
  html += "}";

  html += "function adjustTemp() {";
  html += "  const currentTemp = document.getElementById('setpoint').textContent;";
  html += "  const newTemp = prompt('Enter target temperature (150-500F):', currentTemp);";
  html += "  if (newTemp && !isNaN(newTemp)) {";
  html += "    const temp = parseInt(newTemp);";
  html += "    if (temp >= 150 && temp <= 500) {";
  html += "      setTemp(temp);";
  html += "    } else {";
  html += "      alert('Temperature must be between 150F and 500F');";
  html += "    }";
  html += "  }";
  html += "}";

  // ADD PRIME FUNCTION
  html += "function primeAuger() {";
  html += "  if (!confirm('Run 30-second PiFire auger prime to fill burn pot?')) return;";
  html += "  const button = event.target;";
  html += "  button.disabled = true;";
  html += "  button.textContent = 'PRIMING... (30s)';";
  html += "  ";
  html += "  fetch('/prime_auger').then(response => {";
  html += "    if (!response.ok) throw new Error('HTTP ' + response.status);";
  html += "    return response.text();";
  html += "  }).then(data => {";
  html += "    alert('Prime Complete: ' + data);";
  html += "    updateTemperatures();";
  html += "  }).catch(error => {";
  html += "    alert('Error priming auger: ' + error.message);";
  html += "  }).finally(() => {";
  html += "    button.disabled = false;";
  html += "    button.textContent = '🌾 PRIME (30s)';";
  html += "  });";
  html += "}";

  html += "document.addEventListener('DOMContentLoaded', function() {";
  html += "  startRealTimeUpdates();";
  html += "});";

  html += "</script></body></html>";
  
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

  // ADD THIS TO YOUR EXISTING GrillWebServer.cpp - REPLACE THE PID PAGE SECTION

  // Enhanced PID Tuning and Pellet Control Page
  server.on("/pid", HTTP_GET, [](AsyncWebServerRequest *req) {
    float kp, ki, kd;
    getPIDParameters(&kp, &ki, &kd);
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>PID Tuning & Pellet Control</title>";
    html += "<style>";
    html += "body { background: #1a1a1a; color: #fff; font-family: Arial, sans-serif; padding: 20px; }";
    html += ".container { max-width: 800px; margin: 0 auto; }";
    html += "h1 { color: #60a5fa; text-align: center; margin-bottom: 30px; }";
    html += "h2 { color: #fbbf24; margin: 30px 0 15px 0; border-bottom: 2px solid #fbbf24; padding-bottom: 5px; }";
    html += ".section { background: rgba(255,255,255,0.1); padding: 20px; margin: 20px 0; border-radius: 10px; }";
    html += ".form-group { margin: 15px 0; display: flex; align-items: center; }";
    html += "label { display: block; margin-bottom: 5px; font-weight: bold; min-width: 200px; }";
    html += "input, select { padding: 8px; font-size: 1em; border-radius: 5px; border: 1px solid #555; background: #333; color: #fff; margin-left: 10px; }";
    html += "input[type='number'] { width: 120px; }";
    html += ".btn { padding: 12px 25px; background: #059669; color: white; border: none; border-radius: 5px; font-size: 1em; cursor: pointer; margin: 10px 5px; }";
    html += ".btn:hover { background: #047857; }";
    html += ".btn-warning { background: #f59e0b; }";
    html += ".btn-warning:hover { background: #d97706; }";
    html += ".current-value { color: #4ade80; font-weight: bold; }";
    html += ".description { font-size: 0.9em; color: #bbb; margin-top: 5px; }";
    html += ".warning { background: #fbbf24; color: #000; padding: 15px; border-radius: 5px; margin: 15px 0; }";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>🎛️ PID Tuning & Pellet Control</h1>";
    
    // PID Section
    html += "<div class='section'>";
    html += "<h2>PID Parameters</h2>";
    html += "<div class='warning'>⚠️ <strong>WARNING:</strong> Incorrect PID values can cause temperature instability or poor performance.</div>";
    
    html += "<form onsubmit='savePID(event)'>";
    html += "<div class='form-group'>";
    html += "<label>Proportional (Kp):</label>";
    html += "<input type='number' id='kp' step='0.1' min='0' max='10' value='" + String(kp, 2) + "'>";
    html += "<div class='description'>Current: <span class='current-value'>" + String(kp, 3) + "</span> - Controls immediate response to temperature error</div>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label>Integral (Ki):</label>";
    html += "<input type='number' id='ki' step='0.001' min='0' max='1' value='" + String(ki, 4) + "'>";
    html += "<div class='description'>Current: <span class='current-value'>" + String(ki, 4) + "</span> - Eliminates steady-state error over time</div>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label>Derivative (Kd):</label>";
    html += "<input type='number' id='kd' step='0.1' min='0' max='5' value='" + String(kd, 2) + "'>";
    html += "<div class='description'>Current: <span class='current-value'>" + String(kd, 3) + "</span> - Prevents overshoot and oscillation</div>";
    html += "</div>";
    
    html += "<button type='submit' class='btn'>💾 Save PID Parameters</button>";
    html += "<button type='button' class='btn btn-warning' onclick='resetPIDDefaults()'>🔄 Reset to Defaults</button>";
    html += "</form>";
    html += "</div>";
    
    // Pellet Feed Parameters Section
    html += "<div class='section'>";
    html += "<h2>🌾 Pellet Feed Parameters</h2>";
    html += "<div class='warning'>🔥 <strong>IGNITION TUNING:</strong> Adjust these values to improve ignition performance. More pellets = better ignition but more smoke.</div>";
    
    // Get current pellet parameters
    unsigned long initialFeed = pellet_get_initial_feed_duration();
    unsigned long lightingFeed = pellet_get_lighting_feed_duration();
    unsigned long normalFeed = pellet_get_normal_feed_duration();
    unsigned long lightingInterval = pellet_get_lighting_feed_interval();
    
    html += "<form onsubmit='savePelletParams(event)'>";
    
    html += "<div class='form-group'>";
    html += "<label>Initial Feed Duration:</label>";
    html += "<input type='number' id='initialFeed' min='10' max='120' value='" + String(initialFeed / 1000) + "'> seconds";
    html += "<div class='description'>Current: <span class='current-value'>" + String(initialFeed / 1000) + "s</span> - First pellet feed when ignition starts (10-120s)</div>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label>Lighting Feed Duration:</label>";
    html += "<input type='number' id='lightingFeed' min='5' max='60' value='" + String(lightingFeed / 1000) + "'> seconds";
    html += "<div class='description'>Current: <span class='current-value'>" + String(lightingFeed / 1000) + "s</span> - Pellet feed during lighting phase (5-60s)</div>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label>Normal Feed Duration:</label>";
    html += "<input type='number' id='normalFeed' min='1' max='30' value='" + String(normalFeed / 1000) + "'> seconds";
    html += "<div class='description'>Current: <span class='current-value'>" + String(normalFeed / 1000) + "s</span> - Normal operation feed time (1-30s)</div>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label>Lighting Feed Interval:</label>";
    html += "<input type='number' id='lightingInterval' min='30' max='180' value='" + String(lightingInterval / 1000) + "'> seconds";
    html += "<div class='description'>Current: <span class='current-value'>" + String(lightingInterval / 1000) + "s</span> - Time between lighting feeds (30-180s)</div>";
    html += "</div>";
    
    html += "<button type='submit' class='btn'>🌾 Save Pellet Parameters</button>";
    html += "<button type='button' class='btn btn-warning' onclick='resetPelletDefaults()'>🔄 Reset Pellet Defaults</button>";
    html += "</form>";
    html += "</div>";
    
    // Current Status Section
    html += "<div class='section'>";
    html += "<h2>📊 Current Status</h2>";
    html += "<div id='status-display'>";
    html += "<p><strong>Grill Running:</strong> " + String(grillRunning ? "YES" : "NO") + "</p>";
    html += "<p><strong>Target Temperature:</strong> " + String(setpoint, 1) + "°F</p>";
    html += "<p><strong>Current Temperature:</strong> " + String(readGrillTemperature(), 1) + "°F</p>";
    html += "<p><strong>Manual Override:</strong> " + String(relay_get_manual_override_status() ? "ACTIVE" : "INACTIVE") + "</p>";
    html += "</div>";
    html += "<button class='btn' onclick='refreshStatus()'>🔄 Refresh Status</button>";
    html += "</div>";
    
    html += "<a href='/' class='btn' style='display: block; text-align: center; margin: 30px 0; text-decoration: none;'>← Back to Dashboard</a>";
    html += "</div>";

    // JavaScript functionality
    html += "<script>";
    
    // Save PID parameters
    html += "function savePID(event) {";
    html += "  event.preventDefault();";
    html += "  const kp = document.getElementById('kp').value;";
    html += "  const ki = document.getElementById('ki').value;";
    html += "  const kd = document.getElementById('kd').value;";
    html += "  ";
    html += "  fetch(`/set_pid?kp=${kp}&ki=${ki}&kd=${kd}`)";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      alert('PID Parameters Saved: ' + data);";
    html += "      setTimeout(() => location.reload(), 1000);";
    html += "    });";
    html += "}";
    
    // Save pellet parameters
    html += "function savePelletParams(event) {";
    html += "  event.preventDefault();";
    html += "  const initialFeed = document.getElementById('initialFeed').value;";
    html += "  const lightingFeed = document.getElementById('lightingFeed').value;";
    html += "  const normalFeed = document.getElementById('normalFeed').value;";
    html += "  const lightingInterval = document.getElementById('lightingInterval').value;";
    html += "  ";
    html += "  fetch(`/set_pellet_params?initial=${initialFeed}&lighting=${lightingFeed}&normal=${normalFeed}&interval=${lightingInterval}`)";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      alert('Pellet Parameters Saved: ' + data);";
    html += "      setTimeout(() => location.reload(), 1000);";
    html += "    });";
    html += "}";
    
    // Reset functions
    html += "function resetPIDDefaults() {";
    html += "  if (confirm('Reset PID to default values? (Kp=1.5, Ki=0.01, Kd=0.5)')) {";
    html += "    fetch('/set_pid?kp=1.5&ki=0.01&kd=0.5')";
    html += "      .then(response => response.text())";
    html += "      .then(data => {";
    html += "        alert('PID Reset to Defaults');";
    html += "        location.reload();";
    html += "      });";
    html += "  }";
    html += "}";
    
    html += "function resetPelletDefaults() {";
    html += "  if (confirm('Reset pellet parameters to defaults?')) {";
    html += "    fetch('/set_pellet_params?initial=45&lighting=20&normal=5&interval=60')";
    html += "      .then(response => response.text())";
    html += "      .then(data => {";
    html += "        alert('Pellet Parameters Reset to Defaults');";
    html += "        location.reload();";
    html += "      });";
    html += "  }";
    html += "}";
    
    // Status refresh
    html += "function refreshStatus() {";
    html += "  location.reload();";
    html += "}";
    
    html += "</script>";
    html += "</body></html>";
    
    req->send(200, "text/html", html);
  });

  // New endpoint for setting pellet parameters
  server.on("/set_pellet_params", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!req->hasParam("initial") || !req->hasParam("lighting") || 
        !req->hasParam("normal") || !req->hasParam("interval")) {
      req->send(400, "text/plain", "Missing pellet parameters");
      return;
    }
    
    unsigned long initial = req->getParam("initial")->value().toInt() * 1000; // Convert to ms
    unsigned long lighting = req->getParam("lighting")->value().toInt() * 1000;
    unsigned long normal = req->getParam("normal")->value().toInt() * 1000;
    unsigned long interval = req->getParam("interval")->value().toInt() * 1000;
    
    // Validate ranges
    if (initial < 10000 || initial > 120000) {
      req->send(400, "text/plain", "Initial feed out of range (10-120s)");
      return;
    }
    if (lighting < 5000 || lighting > 60000) {
      req->send(400, "text/plain", "Lighting feed out of range (5-60s)");
      return;
    }
    if (normal < 1000 || normal > 30000) {
      req->send(400, "text/plain", "Normal feed out of range (1-30s)");
      return;
    }
    if (interval < 30000 || interval > 180000) {
      req->send(400, "text/plain", "Lighting interval out of range (30-180s)");
      return;
    }
    
    // Set the parameters
    pellet_set_initial_feed_duration(initial);
    pellet_set_lighting_feed_duration(lighting);
    pellet_set_normal_feed_duration(normal);
    pellet_set_lighting_feed_interval(interval);
    
    String response = "Pellet parameters updated: ";
    response += "Initial=" + String(initial/1000) + "s, ";
    response += "Lighting=" + String(lighting/1000) + "s, ";
    response += "Normal=" + String(normal/1000) + "s, ";
    response += "Interval=" + String(interval/1000) + "s";
    
    req->send(200, "text/plain", response);
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
  
  // FIXED Control endpoints with proper error handling and debugging
  server.on("/start", HTTP_GET, [](AsyncWebServerRequest *req) {
    Serial.println("🚀 Web request: START grill");
    
    if (grillRunning) {
      Serial.println("   Grill already running");
      req->send(200, "text/plain", "Grill already running");
      return;
    }
    
    // Check for valid temperature reading before starting
    double currentTemp = readGrillTemperature();
    if (!isValidTemperature(currentTemp)) {
      Serial.println("   ERROR: Invalid temperature reading, cannot start");
      req->send(400, "text/plain", "Cannot start: Invalid temperature sensor reading");
      return;
    }
    
    Serial.printf("   Starting grill at %.1f°F, target: %.1f°F\n", currentTemp, setpoint);
    
    // Set grill running flag
    grillRunning = true;
    
    // Clear any manual overrides that might interfere
    relay_clear_manual();
    
    // Start the ignition sequence
    ignition_start(currentTemp);
    
    Serial.println("   ✅ Grill started successfully - ignition sequence initiated");
    req->send(200, "text/plain", "Grill started successfully - ignition sequence initiated");
  });

  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *req) {
    Serial.println("🛑 Web request: STOP grill");
    
    if (!grillRunning) {
      Serial.println("   Grill already stopped");
      req->send(200, "text/plain", "Grill already stopped");
      return;
    }
    
    Serial.println("   Stopping grill and ignition sequence");
    
    // Stop grill operations
    grillRunning = false;
    
    // Stop ignition sequence
    ignition_stop();
    
    // Clear any manual overrides
    relay_clear_manual();
    
    // Turn off all relays safely
    RelayRequest stopReq = {RELAY_OFF, RELAY_OFF, RELAY_ON, RELAY_ON}; // Keep fans on for cooling
    relay_request_auto(&stopReq);
    
    Serial.println("   ✅ Grill stopped successfully - cooling down");
    req->send(200, "text/plain", "Grill stopped successfully - cooling down");
  });

  // Enhanced status endpoint with detailed grill state
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
    json += "\"grillRunning\":" + String(grillRunning ? "true" : "false") + ",";
    json += "\"ignitionState\":\"" + ignition_get_status_string() + "\",";
    json += "\"ignOn\":" + String(ignOn ? "true" : "false") + ",";
    json += "\"augerOn\":" + String(augerOn ? "true" : "false") + ",";
    json += "\"hopperOn\":" + String(hopperOn ? "true" : "false") + ",";
    json += "\"blowerOn\":" + String(blowerOn ? "true" : "false") + ",";
    json += "\"manualOverride\":" + String(relay_get_manual_override_status() ? "true" : "false");
    json += "}";
    
    req->send(200, "application/json", json);
  });

  // Debug endpoint to check grill state
  server.on("/grill_debug", HTTP_GET, [](AsyncWebServerRequest *req) {
    String debug = "Grill Debug Info:\\n";
    debug += "Grill Running: " + String(grillRunning ? "YES" : "NO") + "\\n";
    debug += "Ignition State: " + ignition_get_status_string() + "\\n";
    debug += "Grill Temperature: " + String(readGrillTemperature(), 1) + "°F\\n";
    debug += "Target Temperature: " + String(setpoint, 1) + "°F\\n";
    debug += "Manual Override: " + String(relay_get_manual_override_status() ? "ACTIVE" : "INACTIVE") + "\\n";
    debug += "Free Memory: " + String(ESP.getFreeHeap()) + " bytes\\n";
    debug += "\\nRelay States:\\n";
    debug += "Igniter: " + String(digitalRead(RELAY_IGNITER_PIN) ? "ON" : "OFF") + "\\n";
    debug += "Auger: " + String(digitalRead(RELAY_AUGER_PIN) ? "ON" : "OFF") + "\\n";
    debug += "Hopper Fan: " + String(digitalRead(RELAY_HOPPER_FAN_PIN) ? "ON" : "OFF") + "\\n";
    debug += "Blower Fan: " + String(digitalRead(RELAY_BLOWER_FAN_PIN) ? "ON" : "OFF") + "\\n";
    
    req->send(200, "text/plain", debug);
  });

  // Force start endpoint for troubleshooting
  server.on("/force_start", HTTP_GET, [](AsyncWebServerRequest *req) {
    Serial.println("🔧 FORCE START requested via web");
    
    grillRunning = true;
    relay_clear_manual();
    
    // Get current temperature or use default
    double currentTemp = readGrillTemperature();
    if (!isValidTemperature(currentTemp)) {
      currentTemp = 70.0; // Use room temperature as fallback
      Serial.println("   Using fallback temperature for force start");
    }
    
    ignition_start(currentTemp);
    
    req->send(200, "text/plain", "Force start completed");
  });

  // Force stop endpoint for troubleshooting  
  server.on("/force_stop", HTTP_GET, [](AsyncWebServerRequest *req) {
    Serial.println("🔧 FORCE STOP requested via web");
    
    grillRunning = false;
    ignition_stop();
    relay_emergency_stop();
    
    req->send(200, "text/plain", "Force stop completed");
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