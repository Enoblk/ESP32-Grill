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
    html += "<a href='/debug' class='link-btn'>Debug</a>";
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

    // JavaScript for functionality and REAL-TIME auto-updates
    html += "<script>";
    html += "let updateInterval;";
    html += "let isPageVisible = true;";

    // Visibility API to pause updates when tab is not active
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

    // Update main grill temperature with smooth transitions
    html += "      const grillTempElement = document.getElementById('grill-temp');";
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
    html += "        grillTempElement.className = 'grill-temp-main';";
    html += "        grillTempCardElement.className = 'temp-value';";
    html += "      } else {";
    html += "        grillTempElement.innerHTML = 'SENSOR ERROR';";
    html += "        grillTempCardElement.innerHTML = 'ERROR';";
    html += "        grillTempElement.className = 'grill-temp-main temp-invalid';";
    html += "        grillTempCardElement.className = 'temp-value temp-invalid';";
    html += "      }";

    // Update ambient temperature with change detection
    html += "      const ambientTempElement = document.getElementById('ambient-temp');";
    html += "      if (data.ambientTemp > -900) {";
    html += "        const newAmbientTemp = data.ambientTemp.toFixed(1);";
    html += "        if (ambientTempElement.innerHTML !== newAmbientTemp + '&deg;F') {";
    html += "          ambientTempElement.innerHTML = newAmbientTemp + '&deg;F';";
    html += "          ambientTempElement.style.transition = 'background-color 0.3s ease';";
    html += "          ambientTempElement.style.backgroundColor = 'rgba(96, 165, 250, 0.3)';";
    html += "          setTimeout(() => { ambientTempElement.style.backgroundColor = ''; }, 300);";
    html += "        }";
    html += "        ambientTempElement.className = 'temp-value';";
    html += "      } else {";
    html += "        ambientTempElement.innerHTML = 'N/A';";
    html += "        ambientTempElement.className = 'temp-value temp-invalid';";
    html += "      }";

    // Update meat probe temperatures with individual change detection
    html += "      ['meat1', 'meat2', 'meat3', 'meat4'].forEach((probe, index) => {";
    html += "        const temp = data[probe + 'Temp'];";
    html += "        const element = document.getElementById(probe + '-temp');";
    html += "        ";
    html += "        if (temp > -900) {";
    html += "          const newTemp = temp.toFixed(1);";
    html += "          if (element.innerHTML !== newTemp + '&deg;F') {";
    html += "            element.innerHTML = newTemp + '&deg;F';";
    html += "            element.style.transition = 'all 0.4s ease';";
    html += "            element.style.backgroundColor = 'rgba(245, 158, 11, 0.4)';";
    html += "            element.style.transform = 'scale(1.05)';";
    html += "            setTimeout(() => {";
    html += "              element.style.backgroundColor = '';";
    html += "              element.style.transform = '';";
    html += "            }, 400);";
    html += "          }";
    html += "          element.className = 'temp-value';";
    html += "        } else {";
    html += "          element.innerHTML = 'N/A';";
    html += "          element.className = 'temp-value temp-invalid';";
    html += "        }";
    html += "      });";

    // Update setpoint and status
    html += "      document.getElementById('setpoint').textContent = data.setpoint;";
    html += "      ";
    html += "      const statusElement = document.getElementById('status');";
    html += "      if (statusElement.textContent !== data.status) {";
    html += "        statusElement.textContent = data.status;";
    html += "        statusElement.style.transition = 'box-shadow 0.5s ease';";
    html += "        statusElement.style.boxShadow = '0 0 15px rgba(255, 255, 255, 0.5)';";
    html += "        setTimeout(() => { statusElement.style.boxShadow = ''; }, 500);";
    html += "      }";

    // Update relay indicators with smooth transitions
    html += "      const relays = [";
    html += "        {id: 'ign-dot', state: data.ignOn, name: 'igniter'},";
    html += "        {id: 'aug-dot', state: data.augerOn, name: 'auger'},";
    html += "        {id: 'hop-dot', state: data.hopperOn, name: 'hopper'},";
    html += "        {id: 'blo-dot', state: data.blowerOn, name: 'blower'}";
    html += "      ];";
    html += "      ";
    html += "      relays.forEach(relay => {";
    html += "        const element = document.getElementById(relay.id);";
    html += "        const newClass = 'relay-dot ' + (relay.state ? 'relay-on' : 'relay-off');";
    html += "        if (element.className !== newClass) {";
    html += "          element.className = newClass;";
    html += "          element.style.transition = 'all 0.3s ease';";
    html += "          element.style.transform = 'scale(1.3)';";
    html += "          setTimeout(() => { element.style.transform = 'scale(1)'; }, 300);";
    html += "        }";
    html += "      });";

    // Add connection status indicator
    html += "      const now = new Date();";
    html += "      const timeString = now.toLocaleTimeString();";
    html += "      let statusIndicator = document.getElementById('connection-status');";
    html += "      if (!statusIndicator) {";
    html += "        statusIndicator = document.createElement('div');";
    html += "        statusIndicator.id = 'connection-status';";
    html += "        statusIndicator.style.cssText = 'position: fixed; top: 10px; right: 10px; background: rgba(0,255,0,0.8); color: black; padding: 5px 10px; border-radius: 5px; font-size: 12px; z-index: 1000;';";
    html += "        document.body.appendChild(statusIndicator);";
    html += "      }";
    html += "      statusIndicator.textContent = '🟢 Live: ' + timeString;";
    html += "      statusIndicator.style.backgroundColor = 'rgba(0,255,0,0.8)';";

    html += "    })";
    html += "    .catch(err => {";
    html += "      console.log('Update failed:', err);";
    html += "      let statusIndicator = document.getElementById('connection-status');";
    html += "      if (statusIndicator) {";
    html += "        statusIndicator.textContent = '🔴 Connection Error';";
    html += "        statusIndicator.style.backgroundColor = 'rgba(255,0,0,0.8)';";
    html += "      }";
    html += "    });";
    html += "}";

    // Speed control functions
    html += "function changeUpdateSpeed() {";
    html += "  const speed = parseInt(document.getElementById('updateSpeed').value);";
    html += "  stopRealTimeUpdates();";
    html += "  if (speed === 0) {";
    html += "    showNotification('Updates paused', 'info');";
    html += "    return;";
    html += "  }";
    html += "  if (updateInterval) clearInterval(updateInterval);";
    html += "  updateTemperatures();";
    html += "  updateInterval = setInterval(updateTemperatures, speed);";
    html += "  const speedText = speed < 2000 ? 'Fast' : speed < 4000 ? 'Normal' : 'Slow';";
    html += "  showNotification(`Updates: ${speedText} (${speed/1000}s)`, 'info');";
    html += "}";

    // Enhanced control functions with immediate feedback
    html += "function setTemp(temp) {";
    html += "  document.getElementById('setpoint').textContent = temp;";
    html += "  ";
    html += "  fetch('/set_temp?temp=' + temp)";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      showNotification('Set to ' + temp + '°F', 'success');";
    html += "    })";
    html += "    .catch(err => {";
    html += "      showNotification('Failed to set temperature', 'error');";
    html += "    });";
    html += "}";

    html += "function startGrill() {";
    html += "  showNotification('Starting grill...', 'info');";
    html += "  fetch('/start')";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      showNotification(data, 'success');";
    html += "    })";
    html += "    .catch(err => {";
    html += "      showNotification('Failed to start grill', 'error');";
    html += "    });";
    html += "}";

    html += "function stopGrill() {";
    html += "  showNotification('Stopping grill...', 'info');";
    html += "  fetch('/stop')";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      showNotification(data, 'success');";
    html += "    })";
    html += "    .catch(err => {";
    html += "      showNotification('Failed to stop grill', 'error');";
    html += "    });";
    html += "}";

    html += "function adjustTemp() {";
    html += "  const newTemp = prompt('Enter target temperature (150-500F):', document.getElementById('setpoint').textContent);";
    html += "  if (newTemp && newTemp >= 150 && newTemp <= 500) {";
    html += "    setTemp(parseInt(newTemp));";
    html += "  }";
    html += "}";

    // Notification system
    html += "function showNotification(message, type) {";
    html += "  const notification = document.createElement('div');";
    html += "  notification.style.cssText = `";
    html += "    position: fixed; top: 70px; right: 10px; padding: 15px 20px;";
    html += "    border-radius: 5px; color: white; font-weight: bold;";
    html += "    z-index: 1001; max-width: 300px; word-wrap: break-word;";
    html += "    transition: all 0.3s ease; transform: translateX(100%);";
    html += "  `;";
    html += "  ";
    html += "  switch(type) {";
    html += "    case 'success': notification.style.backgroundColor = '#059669'; break;";
    html += "    case 'error': notification.style.backgroundColor = '#dc2626'; break;";
    html += "    case 'info': notification.style.backgroundColor = '#2563eb'; break;";
    html += "    default: notification.style.backgroundColor = '#6b7280';";
    html += "  }";
    html += "  ";
    html += "  notification.textContent = message;";
    html += "  document.body.appendChild(notification);";
    html += "  ";
    html += "  setTimeout(() => { notification.style.transform = 'translateX(0)'; }, 10);";
    html += "  ";
    html += "  setTimeout(() => {";
    html += "    notification.style.transform = 'translateX(100%)';";
    html += "    setTimeout(() => { document.body.removeChild(notification); }, 300);";
    html += "  }, 3000);";
    html += "}";

    // Start real-time updates when page loads
    html += "document.addEventListener('DOMContentLoaded', function() {";
    html += "  startRealTimeUpdates();";
    html += "});";

    html += "startRealTimeUpdates();";
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

  // Debug Control Page
  server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *req) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>Debug Control - Grill Controller</title>";
    html += "<style>";
    html += "body { background: #1a1a1a; color: #fff; font-family: Arial, sans-serif; padding: 20px; }";
    html += ".container { max-width: 600px; margin: 0 auto; }";
    html += "h1 { color: #60a5fa; text-align: center; margin-bottom: 30px; }";
    html += ".debug-section { background: rgba(255,255,255,0.1); padding: 20px; margin: 15px 0; border-radius: 10px; }";
    html += ".toggle-row { display: flex; align-items: center; justify-content: space-between; margin: 10px 0; }";
    html += ".toggle { position: relative; display: inline-block; width: 60px; height: 34px; }";
    html += ".toggle input { opacity: 0; width: 0; height: 0; }";
    html += ".slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; ";
    html += "background-color: #ccc; transition: .4s; border-radius: 34px; }";
    html += ".slider:before { position: absolute; content: ''; height: 26px; width: 26px; left: 4px; ";
    html += "bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; }";
    html += "input:checked + .slider { background-color: #4CAF50; }";
    html += "input:checked + .slider:before { transform: translateX(26px); }";
    html += ".btn { padding: 12px 20px; margin: 10px 5px; background: #059669; color: white; border: none; border-radius: 5px; cursor: pointer; }";
    html += ".btn-danger { background: #dc2626; }";
    html += ".sensor-info { font-size: 0.9em; color: #bbb; margin-top: 5px; }";
    html += ".status-indicator { width: 12px; height: 12px; border-radius: 50%; display: inline-block; margin-right: 8px; }";
    html += ".status-on { background: #4CAF50; }";
    html += ".status-off { background: #666; }";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>Debug Control Center</h1>";
    
    html += "<div class='debug-section'>";
    html += "<h3>🔥 Grill Temperature Sensor</h3>";
    html += "<div class='toggle-row'>";
    html += "<div>";
    html += "<span class='status-indicator' id='grill-status'></span>";
    html += "PT100 RTD Debug";
    html += "<div class='sensor-info'>Shows: ADC, Voltage, Resistance, Temperature, Alpha</div>";
    html += "</div>";
    html += "<label class='toggle'>";
    html += "<input type='checkbox' id='grillDebug' onchange='toggleDebug(\"grill\", this.checked)'>";
    html += "<span class='slider'></span>";
    html += "</label>";
    html += "</div>";
    html += "</div>";
    
    html += "<div class='debug-section'>";
    html += "<h3>🌡️ Ambient Temperature Sensor</h3>";
    html += "<div class='toggle-row'>";
    html += "<div>";
    html += "<span class='status-indicator' id='ambient-status'></span>";
    html += "100kΩ NTC Debug";
    html += "<div class='sensor-info'>Shows: ADC, Voltage, Resistance, Temperature, Beta</div>";
    html += "</div>";
    html += "<label class='toggle'>";
    html += "<input type='checkbox' id='ambientDebug' onchange='toggleDebug(\"ambient\", this.checked)'>";
    html += "<span class='slider'></span>";
    html += "</label>";
    html += "</div>";
    html += "</div>";
    
    html += "<div class='debug-section'>";
    html += "<h3>🥩 Meat Probe Sensors</h3>";
    html += "<div class='toggle-row'>";
    html += "<div>";
    html += "<span class='status-indicator' id='meat-status'></span>";
    html += "1kΩ NTC ADS1115 Debug";
    html += "<div class='sensor-info'>Shows: ADS ADC, Voltage, Resistance, Temperature</div>";
    html += "</div>";
    html += "<label class='toggle'>";
    html += "<input type='checkbox' id='meatDebug' onchange='toggleDebug(\"meat\", this.checked)'>";
    html += "<span class='slider'></span>";
    html += "</label>";
    html += "</div>";
    html += "</div>";
    
    html += "<div class='debug-section'>";
    html += "<h3>⚡ Relay Control</h3>";
    html += "<div class='toggle-row'>";
    html += "<div>";
    html += "<span class='status-indicator' id='relay-status'></span>";
    html += "Relay State Changes";
    html += "<div class='sensor-info'>Shows: Pin states, Manual overrides, Safety checks</div>";
    html += "</div>";
    html += "<label class='toggle'>";
    html += "<input type='checkbox' id='relayDebug' onchange='toggleDebug(\"relay\", this.checked)'>";
    html += "<span class='slider'></span>";
    html += "</label>";
    html += "</div>";
    html += "</div>";
    
    html += "<div class='debug-section'>";
    html += "<h3>🔧 System Debug</h3>";
    html += "<div class='toggle-row'>";
    html += "<div>";
    html += "<span class='status-indicator' id='system-status'></span>";
    html += "General System Info";
    html += "<div class='sensor-info'>Shows: Memory, Timing, Status changes</div>";
    html += "</div>";
    html += "<label class='toggle'>";
    html += "<input type='checkbox' id='systemDebug' onchange='toggleDebug(\"system\", this.checked)'>";
    html += "<span class='slider'></span>";
    html += "</label>";
    html += "</div>";
    html += "</div>";
    
    // Quick action buttons
    html += "<div style='text-align: center; margin: 30px 0;'>";
    html += "<button class='btn' onclick='setAllDebug(true)'>Enable All</button>";
    html += "<button class='btn btn-danger' onclick='setAllDebug(false)'>Disable All</button>";
    html += "</div>";
    
    html += "<a href='/' class='btn' style='display: block; text-align: center; margin: 20px 0; text-decoration: none;'>Back to Grill Control</a>";
    html += "</div>";

    // JavaScript
    html += "<script>";
    html += "function toggleDebug(sensor, enabled) {";
    html += "  fetch('/set_individual_debug?sensor=' + sensor + '&enabled=' + (enabled ? '1' : '0'))";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      console.log(data);";
    html += "      updateStatusIndicators();";
    html += "    });";
    html += "}";
    
    html += "function setAllDebug(enabled) {";
    html += "  fetch('/set_all_debug?enabled=' + (enabled ? '1' : '0'))";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      console.log(data);";
    html += "      updateAllToggles();";
    html += "    });";
    html += "}";
    
    html += "function updateStatusIndicators() {";
    html += "  fetch('/get_debug_status_all')";
    html += "    .then(response => response.json())";
    html += "    .then(data => {";
    html += "      document.getElementById('grill-status').className = 'status-indicator ' + (data.grill ? 'status-on' : 'status-off');";
    html += "      document.getElementById('ambient-status').className = 'status-indicator ' + (data.ambient ? 'status-on' : 'status-off');";
    html += "      document.getElementById('meat-status').className = 'status-indicator ' + (data.meat ? 'status-on' : 'status-off');";
    html += "      document.getElementById('relay-status').className = 'status-indicator ' + (data.relay ? 'status-on' : 'status-off');";
    html += "      document.getElementById('system-status').className = 'status-indicator ' + (data.system ? 'status-on' : 'status-off');";
    html += "    });";
    html += "}";
    
    html += "function updateAllToggles() {";
    html += "  fetch('/get_debug_status_all')";
    html += "    .then(response => response.json())";
    html += "    .then(data => {";
    html += "      document.getElementById('grillDebug').checked = data.grill;";
    html += "      document.getElementById('ambientDebug').checked = data.ambient;";
    html += "      document.getElementById('meatDebug').checked = data.meat;";
    html += "      document.getElementById('relayDebug').checked = data.relay;";
    html += "      document.getElementById('systemDebug').checked = data.system;";
    html += "      updateStatusIndicators();";
    html += "    });";
    html += "}";
    
    // Initialize on page load
    html += "updateAllToggles();";
    html += "setInterval(updateStatusIndicators, 5000);";
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

  // Fast status endpoint - optimized for real-time updates
  server.on("/status_fast", HTTP_GET, [](AsyncWebServerRequest *req) {
    // Cache temperature readings to avoid multiple sensor reads
    static double lastGrillTemp = -999.0;
    static double lastAmbientTemp = -999.0;
    static float lastMeat1 = -999.0, lastMeat2 = -999.0, lastMeat3 = -999.0, lastMeat4 = -999.0;
    static unsigned long lastUpdate = 0;
    
    unsigned long now = millis();
    
    // Only read sensors every 500ms to avoid overwhelming them
    if (now - lastUpdate > 500) {
      lastGrillTemp = readGrillTemperature();
      lastAmbientTemp = readAmbientTemperature();
      lastMeat1 = tempSensor.getFoodTemperature(1);
      lastMeat2 = tempSensor.getFoodTemperature(2);
      lastMeat3 = tempSensor.getFoodTemperature(3);
      lastMeat4 = tempSensor.getFoodTemperature(4);
      lastUpdate = now;
    }
    
    // Get relay states (these are fast to read)
    bool ignOn = digitalRead(RELAY_IGNITER_PIN) == HIGH;
    bool augerOn = digitalRead(RELAY_AUGER_PIN) == HIGH;
    bool hopperOn = digitalRead(RELAY_HOPPER_FAN_PIN) == HIGH;
    bool blowerOn = digitalRead(RELAY_BLOWER_FAN_PIN) == HIGH;
    String status = getStatus(lastGrillTemp);

    // Build compact JSON response
    String json = "{";
    json += "\"grillTemp\":" + String(lastGrillTemp, 1) + ",";
    json += "\"ambientTemp\":" + String(lastAmbientTemp, 1) + ",";
    json += "\"meat1Temp\":" + String(lastMeat1, 1) + ",";
    json += "\"meat2Temp\":" + String(lastMeat2, 1) + ",";
    json += "\"meat3Temp\":" + String(lastMeat3, 1) + ",";
    json += "\"meat4Temp\":" + String(lastMeat4, 1) + ",";
    json += "\"setpoint\":" + String((int)setpoint) + ",";
    json += "\"status\":\"" + status + "\",";
    json += "\"ignOn\":" + String(ignOn ? "true" : "false") + ",";
    json += "\"augerOn\":" + String(augerOn ? "true" : "false") + ",";
    json += "\"hopperOn\":" + String(hopperOn ? "true" : "false") + ",";
    json += "\"blowerOn\":" + String(blowerOn ? "true" : "false") + ",";
    json += "\"timestamp\":" + String(now);
    json += "}";
    
    // Set headers for real-time streaming
    AsyncWebServerResponse *response = req->beginResponse(200, "application/json", json);
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
    req->send(response);
  });

  // Lightweight endpoint for just temperature data (even faster)
  server.on("/temps_only", HTTP_GET, [](AsyncWebServerRequest *req) {
    // Get just the essential temperature data
    double grillTemp = readGrillTemperature();
    double ambientTemp = readAmbientTemperature();
    
    String json = "{";
    json += "\"grill\":" + String(grillTemp, 1) + ",";
    json += "\"ambient\":" + String(ambientTemp, 1) + ",";
    json += "\"target\":" + String((int)setpoint);
    json += "}";
    
    AsyncWebServerResponse *response = req->beginResponse(200, "application/json", json);
    response->addHeader("Cache-Control", "no-cache");
    req->send(response);
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

  // Individual debug control endpoints
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

  server.on("/set_all_debug", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!req->hasParam("enabled")) {
      req->send(400, "text/plain", "Missing enabled parameter");
      return;
    }
    
    bool enabled = (req->getParam("enabled")->value() == "1");
    setAllDebug(enabled);
    
    req->send(200, "text/plain", "All debug modes " + String(enabled ? "enabled" : "disabled"));
  });

  server.on("/get_debug_status_all", HTTP_GET, [](AsyncWebServerRequest *req) {
    String json = "{";
    json += "\"grill\":" + String(getGrillDebug() ? "true" : "false") + ",";
    json += "\"ambient\":" + String(getAmbientDebug() ? "true" : "false") + ",";
    json += "\"meat\":" + String(getMeatProbesDebug() ? "true" : "false") + ",";
    json += "\"relay\":" + String(getRelayDebug() ? "true" : "false") + ",";
    json += "\"system\":" + String(getSystemDebug() ? "true" : "false");
    json += "}";
    
    req->send(200, "application/json", json);
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