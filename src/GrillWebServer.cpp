// GrillWebServer.cpp - RESTORE your original full web UI
#include "Globals.h"
#include "Utility.h"
#include "Ignition.h"
#include "MAX31865Sensor.h"
#include <ElegantOTA.h>

// Simple prime function since we can't access the one in Ignition.cpp
void simple_auger_prime() {
  if (!grillRunning) {
    digitalWrite(RELAY_AUGER_PIN, HIGH);
    digitalWrite(RELAY_HOPPER_FAN_PIN, HIGH);
    delay(30000);  // 30 seconds
    digitalWrite(RELAY_AUGER_PIN, LOW);
    digitalWrite(RELAY_HOPPER_FAN_PIN, LOW);
  }
}

void setup_grill_server() {
  // RESTORE your original full dashboard with all features
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
    // Get all sensor data
    double grillTemp = readGrillTemperature();
    double ambientTemp = readAmbientTemperature();
    // For meat probes, we'll return placeholder values since TemperatureSensor is disabled
    float meat1 = -999.0;  // Will show as N/A
    float meat2 = -999.0;
    float meat3 = -999.0;
    float meat4 = -999.0;
    
    String status = ignition_get_status_string();
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
    html += ".status.maintaining { background: #4ecdc4; }";
    html += ".status.off { background: rgba(255,255,255,0.2); }";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<div class='header'>";
    html += "<h1>Green Mountain Grill v1.1</h1>";
    html += "<div>Daniel Boone Controller - Simplified Control</div>";
    html += "<div>IP: " + WiFi.localIP().toString() + " | Build: " + String(__DATE__) + "</div>";
    html += "<div style='margin: 15px 0;'>";
    html += "<a href='/manual' class='link-btn'>Manual Control</a>";
    html += "<a href='/pid' class='link-btn'>Settings</a>";
    html += "<a href='/debug' class='link-btn'>Debug</a>";
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
    if (grillTemp > 0 && grillTemp < 1000) {
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
    if (grillTemp > 0 && grillTemp < 1000) {
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
    
    // Control buttons - WITH 3 COLUMNS INCLUDING PRIME
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

    // JavaScript - Your original full-featured JavaScript
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

    html += "function primeAuger() {";
    html += "  if (!confirm('Run 30-second auger prime to fill burn pot?')) return;";
    html += "  const button = event.target;";
    html += "  button.disabled = true;";
    html += "  button.textContent = 'PRIMING... (30s)';";
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

  // Keep all your original endpoints for the full web interface
  
  // Enhanced status endpoint with all temperatures (same as original)
  server.on("/status_all", HTTP_GET, [](AsyncWebServerRequest *req) {
    double grillTemp = readGrillTemperature();
    double ambientTemp = readAmbientTemperature();
    float meat1 = -999.0;  // Placeholder since TemperatureSensor disabled
    float meat2 = -999.0;
    float meat3 = -999.0;
    float meat4 = -999.0;
    
    bool ignOn = digitalRead(RELAY_IGNITER_PIN) == HIGH;
    bool augerOn = digitalRead(RELAY_AUGER_PIN) == HIGH;
    bool hopperOn = digitalRead(RELAY_HOPPER_FAN_PIN) == HIGH;
    bool blowerOn = digitalRead(RELAY_BLOWER_FAN_PIN) == HIGH;
    String status = ignition_get_status_string();

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

  // Start/Stop endpoints
  server.on("/start", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!grillRunning) {
      double currentTemp = readGrillTemperature();
      ignition_start(currentTemp);
      req->send(200, "text/plain", "Grill started successfully");
    } else {
      req->send(200, "text/plain", "Grill already running");
    }
  });

  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (grillRunning) {
      ignition_stop();
      req->send(200, "text/plain", "Grill stopped successfully");
    } else {
      req->send(200, "text/plain", "Grill already stopped");
    }
  });

  // Prime auger
  server.on("/prime_auger", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (grillRunning) {
      req->send(400, "text/plain", "Cannot prime while grill is running");
      return;
    }
    
    req->send(200, "text/plain", "Auger prime started - 30 seconds");
    simple_auger_prime();
  });

  // Add placeholder endpoints for the links in your original UI
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
    html += ".warning { background: #fbbf24; color: #000; padding: 15px; border-radius: 5px; margin: 20px 0; }";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>Manual Relay Control</h1>";
    
    html += "<div class='warning'>";
    html += "⚠️ <strong>WARNING:</strong> Manual control overrides automatic systems.";
    html += "</div>";
    
    // Igniter Control
    html += "<div class='relay-control'>";
    html += "<div class='relay-status'>";
    html += "<div class='status-dot " + String(ignOn ? "status-on" : "status-off") + "'></div>";
    html += "<h3>Igniter</h3>";
    html += "</div>";
    html += "<button class='btn' onclick='controlRelay(\"igniter\", \"on\")'>Turn ON</button>";
    html += "<button class='btn btn-danger' onclick='controlRelay(\"igniter\", \"off\")'>Turn OFF</button>";
    html += "</div>";
    
    // Auger Control
    html += "<div class='relay-control'>";
    html += "<div class='relay-status'>";
    html += "<div class='status-dot " + String(augerOn ? "status-on" : "status-off") + "'></div>";
    html += "<h3>Auger</h3>";
    html += "</div>";
    html += "<button class='btn' onclick='controlRelay(\"auger\", \"on\")'>Turn ON</button>";
    html += "<button class='btn btn-danger' onclick='controlRelay(\"auger\", \"off\")'>Turn OFF</button>";
    html += "</div>";

    // Hopper Fan Control
    html += "<div class='relay-control'>";
    html += "<div class='relay-status'>";
    html += "<div class='status-dot " + String(hopperOn ? "status-on" : "status-off") + "'></div>";
    html += "<h3>Hopper Fan</h3>";
    html += "</div>";
    html += "<button class='btn' onclick='controlRelay(\"hopper\", \"on\")'>Turn ON</button>";
    html += "<button class='btn btn-danger' onclick='controlRelay(\"hopper\", \"off\")'>Turn OFF</button>";
    html += "</div>";

    // Blower Fan Control
    html += "<div class='relay-control'>";
    html += "<div class='relay-status'>";
    html += "<div class='status-dot " + String(blowerOn ? "status-on" : "status-off") + "'></div>";
    html += "<h3>Blower Fan</h3>";
    html += "</div>";
    html += "<button class='btn' onclick='controlRelay(\"blower\", \"on\")'>Turn ON</button>";
    html += "<button class='btn btn-danger' onclick='controlRelay(\"blower\", \"off\")'>Turn OFF</button>";
    html += "</div>";
    
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

  server.on("/pid", HTTP_GET, [](AsyncWebServerRequest *req) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>Settings - v1.1</title>";
    html += "<style>";
    html += "body { background: linear-gradient(135deg, #1e3c72, #2a5298); color: #fff; font-family: Arial, sans-serif; padding: 20px; }";
    html += ".container { max-width: 800px; margin: 0 auto; }";
    html += "h1 { color: #60a5fa; text-align: center; margin-bottom: 30px; }";
    html += ".section { background: rgba(255,255,255,0.1); padding: 20px; margin: 20px 0; border-radius: 10px; }";
    html += ".btn { padding: 15px 30px; background: #059669; color: white; border: none; border-radius: 5px; cursor: pointer; margin: 10px; text-decoration: none; display: inline-block; text-align: center; }";
    html += ".btn:hover { background: #047857; }";
    html += ".btn-blue { background: #2196F3; }";
    html += ".btn-blue:hover { background: #1976D2; }";
    html += ".btn-orange { background: #ff9800; }";
    html += ".btn-orange:hover { background: #f57c00; }";
    html += ".btn-purple { background: #9c27b0; }";
    html += ".btn-purple:hover { background: #7b1fa2; }";
    html += ".btn-red { background: #f44336; }";
    html += ".btn-red:hover { background: #d32f2f; }";
    html += ".nav-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin: 20px 0; }";
    html += "h3 { color: #fbbf24; margin-bottom: 15px; }";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>⚙️ Settings & Navigation v1.1</h1>";
    
    // System Information Section
    html += "<div class='section'>";
    html += "<h3>📊 System Information</h3>";
    html += "<p><strong>Controller:</strong> v1.1 Simplified Mode with PiFire-style control</p>";
    html += "<p><strong>Current Temperature:</strong> " + String(readGrillTemperature(), 1) + "°F</p>";
    html += "<p><strong>Target Temperature:</strong> " + String(setpoint, 1) + "°F</p>";
    html += "<p><strong>Grill Status:</strong> " + String(grillRunning ? ignition_get_status_string() : "IDLE") + "</p>";
    html += "<p><strong>Free Memory:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
    html += "<p><strong>WiFi Status:</strong> " + String(WiFi.status() == WL_CONNECTED ? "Connected to " + WiFi.SSID() : "AP Mode") + "</p>";
    html += "<p><strong>Build Date:</strong> " + String(__DATE__) + " " + String(__TIME__) + "</p>";
    html += "</div>";
    
    // Navigation Section
    html += "<div class='section'>";
    html += "<h3>🧭 Navigation & Control Pages</h3>";
    html += "<div class='nav-grid'>";
    
    // Main Dashboard
    html += "<a href='/' class='btn'>🏠 Main Dashboard</a>";
    
    // Control Pages
    html += "<a href='/manual' class='btn btn-orange'>🎛️ Manual Relay Control</a>";
    html += "<a href='/wifi' class='btn btn-blue'>📶 WiFi Configuration</a>";
    html += "<a href='/debug' class='btn btn-purple'>🔧 Debug Information</a>";
    
    // Utility Pages
    html += "<a href='/prime_auger' class='btn btn-red'>🌾 Prime Auger (30s)</a>";
    html += "<a href='/status_all' class='btn'>📈 JSON Status API</a>";
    html += "<a href='/update' class='btn btn-purple'>🔄 OTA Update</a>";
    
    html += "</div>";
    html += "</div>";
    
    // Temperature Control Section
    html += "<div class='section'>";
    html += "<h3>🌡️ Temperature Control Info</h3>";
    html += "<p><strong>Control Method:</strong> PiFire-style Error Curve</p>";
    html += "<p><strong>Temperature Error:</strong> " + String(setpoint - readGrillTemperature(), 1) + "°F</p>";
    html += "<p><strong>Control Features:</strong></p>";
    html += "<ul>";
    html += "<li>10-point temperature error curve</li>";
    html += "<li>Automatic auger timing adjustment</li>";
    html += "<li>Ignition override for startup</li>";
    html += "<li>Overshoot protection</li>";
    html += "<li>Emergency temperature cutoff</li>";
    html += "</ul>";
    html += "</div>";
    
    // System Features Section
    html += "<div class='section'>";
    html += "<h3>⚡ Available Features</h3>";
    html += "<div style='display: grid; grid-template-columns: 1fr 1fr; gap: 20px;'>";
    
    html += "<div>";
    html += "<h4>🎯 Grill Control</h4>";
    html += "<ul>";
    html += "<li>3-state ignition system</li>";
    html += "<li>PiFire-style temperature control</li>";
    html += "<li>Manual relay override</li>";
    html += "<li>Temperature presets</li>";
    html += "<li>Emergency shutdown</li>";
    html += "</ul>";
    html += "</div>";
    
    html += "<div>";
    html += "<h4>🌐 Connectivity</h4>";
    html += "<ul>";
    html += "<li>WiFi station + AP mode</li>";
    html += "<li>Real-time web interface</li>";
    html += "<li>OTA firmware updates</li>";
    html += "<li>JSON API endpoints</li>";
    html += "<li>Mobile-responsive design</li>";
    html += "</ul>";
    html += "</div>";
    
    html += "</div>";
    html += "</div>";
    
    // Quick Actions Section
    html += "<div class='section'>";
    html += "<h3>⚡ Quick Actions</h3>";
    html += "<div class='nav-grid'>";
    html += "<a href='/start' class='btn'>🚀 Start Grill</a>";
    html += "<a href='/stop' class='btn btn-red'>🛑 Stop Grill</a>";
    html += "<a href='/set_temp?temp=225' class='btn'>🔥 Set 225°F</a>";
    html += "<a href='/set_temp?temp=275' class='btn'>🔥 Set 275°F</a>";
    html += "</div>";
    html += "</div>";
    
    html += "<a href='/' class='btn' style='width: 100%; margin-top: 30px;'>← Back to Main Dashboard</a>";
    html += "</div></body></html>";
    
    req->send(200, "text/html", html);
  });

  server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *req) {
    // Get current temperature for debug info
    double currentGrillTemp = readGrillTemperature();
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='utf-8'>";
    html += "<title>Debug Information</title>";
    html += "<style>";
    html += "body { background: #1a1a1a; color: #fff; font-family: 'Courier New', monospace; padding: 20px; }";
    html += ".container { max-width: 800px; margin: 0 auto; }";
    html += ".debug-section { background: rgba(255,255,255,0.1); padding: 15px; margin: 15px 0; border-radius: 5px; }";
    html += ".btn { padding: 10px 20px; background: #059669; color: white; border: none; border-radius: 5px; margin: 5px; text-decoration: none; display: inline-block; }";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>Debug Information v1.1</h1>";
    
    html += "<div class='debug-section'>";
    html += "<h3>Version Information</h3>";
    html += "<p>Version: v1.1 Simplified Control</p>";
    html += "<p>Build Date: " + String(__DATE__) + " " + String(__TIME__) + "</p>";
    html += "<p>Features: 3-State Control, WiFi, Manual Relays, OTA</p>";
    html += "</div>";
    
    html += "<div class='debug-section'>";
    html += "<h3>Temperature Debug</h3>";
    html += "<p>Grill Sensor Status: " + String(currentGrillTemp > 0 && currentGrillTemp < 1000 ? "Working" : "Error/Default") + "</p>";
    html += "<p>Last Reading: " + String(currentGrillTemp, 2) + "°F</p>";
    html += "<p>Reading Source: " + String(currentGrillTemp == 70.0 ? "Default/Cached" : "Live Sensor") + "</p>";
    html += "</div>";
    
    html += "<div class='debug-section'>";
    html += "<h3>System Status</h3>";
    html += "<p>Grill Running: " + String(grillRunning ? "YES" : "NO") + "</p>";
    html += "<p>Ignition State: " + ignition_get_status_string() + "</p>";
    html += "<p>Target Temperature: " + String(setpoint, 1) + "°F</p>";
    html += "<p>Free Memory: " + String(ESP.getFreeHeap()) + " bytes</p>";
    html += "<p>Uptime: " + String(millis() / 1000) + " seconds</p>";
    html += "</div>";
    
    html += "<div class='debug-section'>";
    html += "<h3>Relay States</h3>";
    html += "<p>Igniter: " + String(digitalRead(RELAY_IGNITER_PIN) ? "ON" : "OFF") + "</p>";
    html += "<p>Auger: " + String(digitalRead(RELAY_AUGER_PIN) ? "ON" : "OFF") + "</p>";
    html += "<p>Hopper Fan: " + String(digitalRead(RELAY_HOPPER_FAN_PIN) ? "ON" : "OFF") + "</p>";
    html += "<p>Blower Fan: " + String(digitalRead(RELAY_BLOWER_FAN_PIN) ? "ON" : "OFF") + "</p>";
    html += "</div>";
    
    html += "<a href='/' class='btn'>Back to Dashboard</a>";
    html += "</div></body></html>";
    
    req->send(200, "text/html", html);
  });

  // WiFi Configuration Page
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
    html += "input { width: 100%; padding: 10px; font-size: 1em; border-radius: 5px; border: 1px solid #555; background: #333; color: #fff; }";
    html += ".btn { padding: 15px 30px; background: #059669; color: white; border: none; border-radius: 5px; cursor: pointer; margin: 10px 5px; }";
    html += ".status { padding: 15px; margin: 20px 0; border-radius: 5px; background: #333; }";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>WiFi Configuration</h1>";
    
    // Current status
    html += "<div class='status'>";
    html += "<h3>Current Status</h3>";
    if (WiFi.status() == WL_CONNECTED) {
      html += "<p>Status: Connected to " + WiFi.SSID() + "</p>";
      html += "<p>IP Address: " + WiFi.localIP().toString() + "</p>";
      html += "<p>Signal Strength: " + String(WiFi.RSSI()) + " dBm</p>";
    } else {
      html += "<p>Status: AP Mode Active</p>";
      html += "<p>AP IP: " + WiFi.softAPIP().toString() + "</p>";
    }
    html += "</div>";
    
    // WiFi configuration form that actually works
    html += "<form onsubmit='saveWiFi(event)'>";
    html += "<div class='form-group'>";
    html += "<label>Network Name (SSID):</label>";
    html += "<input type='text' id='ssid' placeholder='Enter WiFi network name' required>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label>Password:</label>";
    html += "<input type='password' id='password' placeholder='Enter WiFi password'>";
    html += "</div>";
    
    html += "<button type='submit' class='btn'>Save & Connect</button>";
    html += "<button type='button' class='btn' onclick='scanNetworks()' style='background: #2196F3;'>Scan Networks</button>";
    html += "</form>";
    
    html += "<div id='networks' style='margin: 20px 0;'></div>";
    
    html += "<a href='/' class='btn' style='margin-top: 20px; text-decoration: none;'>Back to Dashboard</a>";
    html += "</div>";

    html += "<script>";
    html += "function saveWiFi(event) {";
    html += "  event.preventDefault();";
    html += "  const ssid = document.getElementById('ssid').value;";
    html += "  const password = document.getElementById('password').value;";
    html += "  ";
    html += "  fetch('/wifi_save', {";
    html += "    method: 'POST',";
    html += "    headers: {'Content-Type': 'application/x-www-form-urlencoded'},";
    html += "    body: 'ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(password)";
    html += "  })";
    html += "  .then(response => response.text())";
    html += "  .then(data => {";
    html += "    alert('WiFi settings saved: ' + data);";
    html += "    setTimeout(() => location.reload(), 3000);";
    html += "  })";
    html += "  .catch(error => alert('Error: ' + error));";
    html += "}";
    
    html += "function scanNetworks() {";
    html += "  document.getElementById('networks').innerHTML = 'Scanning...';";
    html += "  fetch('/wifi_scan')";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      document.getElementById('networks').innerHTML = data;";
    html += "    })";
    html += "    .catch(error => {";
    html += "      document.getElementById('networks').innerHTML = 'Scan failed';";
    html += "    });";
    html += "}";
    
    html += "function selectNetwork(ssid) {";
    html += "  document.getElementById('ssid').value = ssid;";
    html += "}";
    html += "</script>";
    
    html += "</body></html>";
    req->send(200, "text/html", html);
  });

  // WiFi save endpoint
  server.on("/wifi_save", HTTP_POST, [](AsyncWebServerRequest *req) {
    if (!req->hasParam("ssid", true)) {
      req->send(400, "text/plain", "Missing SSID");
      return;
    }
    
    String ssid = req->getParam("ssid", true)->value();
    String password = "";
    
    if (req->hasParam("password", true)) {
      password = req->getParam("password", true)->value();
    }
    
    // Save to preferences
    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    
    // Try to connect
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    req->send(200, "text/plain", "Connecting to " + ssid + "...");
  });

  // WiFi scan endpoint
  server.on("/wifi_scan", HTTP_GET, [](AsyncWebServerRequest *req) {
    int n = WiFi.scanNetworks();
    String html = "";
    
    if (n == 0) {
      html = "<p>No networks found</p>";
    } else {
      html += "<h3>Available Networks:</h3>";
      html += "<div style='background: rgba(255,255,255,0.1); padding: 10px; border-radius: 5px;'>";
      for (int i = 0; i < n; i++) {
        html += "<div style='padding: 8px; margin: 5px 0; background: rgba(255,255,255,0.1); border-radius: 3px; cursor: pointer;' onclick='selectNetwork(\"" + WiFi.SSID(i) + "\")'>";
        html += WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm)";
        if (WiFi.encryptionType(i) != WIFI_AUTH_OPEN) {
          html += " 🔒";
        }
        html += "</div>";
      }
      html += "</div>";
    }
    
    req->send(200, "text/html", html);
  });

  // Manual relay control endpoints
  server.on("/control", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!req->hasParam("relay") || !req->hasParam("state")) { 
      req->send(400, "text/plain", "Missing params"); 
      return; 
    }
    
    String relayName = req->getParam("relay")->value();
    String state = req->getParam("state")->value();
    bool relayState = (state == "on");
    
    if (grillRunning) {
      req->send(400, "text/plain", "Cannot use manual control while grill is running");
      return;
    }
    
    if (relayName == "igniter") {
      digitalWrite(RELAY_IGNITER_PIN, relayState ? HIGH : LOW);
    } else if (relayName == "auger") {
      digitalWrite(RELAY_AUGER_PIN, relayState ? HIGH : LOW);
    } else if (relayName == "hopper") {
      digitalWrite(RELAY_HOPPER_FAN_PIN, relayState ? HIGH : LOW);
    } else if (relayName == "blower") {
      digitalWrite(RELAY_BLOWER_FAN_PIN, relayState ? HIGH : LOW);
    } else {
      req->send(400, "text/plain", "Invalid relay name");
      return;
    }
    
    req->send(200, "text/plain", "Manual control: " + relayName + " = " + state);
  });

  server.onNotFound([](AsyncWebServerRequest *req) {
    req->send(404, "text/plain", "Not Found");
  });

  // Initialize ElegantOTA
  ElegantOTA.begin(&server);
  
  ElegantOTA.onStart([]() {
    Serial.println("OTA update started!");
    grillRunning = false;
    ignition_stop();
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

  server.begin();
  Serial.println("Full web server started with original interface");
}