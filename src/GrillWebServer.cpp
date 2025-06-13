#include "GrillWebServer.h"
#include "Globals.h"
#include "Utility.h"
#include "PelletControl.h" 
#include "Ignition.h"
#include <WiFi.h>
#include <Preferences.h>
//#include <ElegantOTA.h>
#include <WiFiClient.h>
#include <ElegantOTA.h>



void setup_grill_server() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
    double temp   = readTemperature();
    String status = getStatus(temp);
    bool ignOn    = digitalRead(RELAY_IGNITER_PIN)     == HIGH;
    bool augerOn  = digitalRead(RELAY_AUGER_PIN)       == HIGH;
    bool hopperOn = digitalRead(RELAY_HOPPER_FAN_PIN)  == HIGH;
    bool blowerOn = digitalRead(RELAY_BLOWER_FAN_PIN)  == HIGH;
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>Grill Controller</title>";
    html += "<style>body{background:#222;color:#eee;font-family:sans-serif;text-align:center;padding:20px;";
    html += "}button{background:#444;color:#eee;border:none;padding:10px 20px;margin:5px;border-radius:4px;cursor:pointer;}";
    html += "button:hover{background:#555;}h1,a{color:#6cf;}";
    html += "a{display:inline-block;margin-top:15px;color:#6cf;text-decoration:none;}";
    html += "p.rel{margin:4px;font-size:1.1em;}span.dot{font-size:1.2em;vertical-align:middle;margin-right:8px;}";
    html += "</style></head><body>";
    html += "<h1>ESP32 Grill Controller</h1>";
    html += "<p>Temp: <strong>" + String(temp,1) + "</strong> °F</p>";
    html += "<p>Set: <strong>" + String((int)setpoint) + "</strong> °F</p>";
    html += "<p>Status: <strong>" + status + "</strong></p>";
    html += "<p class='rel'><span class='dot' style='color:" + relayColor(ignOn)    + "'>&#9679;</span>Igniter</p>";
    html += "<p class='rel'><span class='dot' style='color:" + relayColor(augerOn)  + "'>&#9679;</span>Auger</p>";
    html += "<p class='rel'><span class='dot' style='color:" + relayColor(hopperOn) + "'>&#9679;</span>Hopper Fan</p>";
    html += "<p class='rel'><span class='dot' style='color:" + relayColor(blowerOn) + "'>&#9679;</span>Blower Fan</p>";
    html += "<button onclick=\"fetch('";
    html += (grillRunning ? "/stop" : "/start");
    html += "').then(()=>location.reload())\">";
    html += (grillRunning ? "Stop Grill" : "Start Grill");
    html += "</button><br>";
    html += "<button onclick=\"fetch('/adjust?delta=+5').then(()=>location.reload())\">";
    html += "+5°F</button> ";
    html += "<button onclick=\"fetch('/adjust?delta=-5').then(()=>location.reload())\">";
    html += "-5°F</button><br>";
    html += "<a href='/troubleshoot'>Troubleshooting</a>";
    html += "</body></html>";
    req->send(200, "text/html", html);
  });

  server.on("/adjust", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!req->hasParam("delta")) { req->send(400, "text/plain", "Missing delta"); return; }
    double delta = req->getParam("delta")->value().toFloat();
    setpoint += delta;
    save_setpoint();
    req->send(200, "text/plain", String((int)setpoint));
  });

  server.on("/start", HTTP_GET, [](AsyncWebServerRequest *req) {
    grillRunning = true;
    ignition_start(readTemperature()); // Use modular ignition!
    req->send(200, "text/plain", "Grill started");
});

  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *req) {
    grillRunning = false;
    // DO NOT turn off hopper fan yet
    digitalWrite(RELAY_IGNITER_PIN, LOW);
    digitalWrite(RELAY_AUGER_PIN, LOW);
    digitalWrite(RELAY_BLOWER_FAN_PIN, LOW);
    // Hopper fan stays ON, temp-controlled in main loop
    req->send(200, "text/plain", "Grill stopped");
});


  server.on("/troubleshoot", HTTP_GET, [](AsyncWebServerRequest *req) {
    String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Troubleshoot</title>";
    html += "<style>body{background:#222;color:#eee;font-family:sans-serif;padding:20px;";
    html += "}button{background:#444;color:#eee;border:none;padding:10px;margin:5px;border-radius:4px;cursor:pointer;}";
    html += "button:hover{background:#555;}a{color:#6cf;}";
    html += "</style></head><body><h1>Troubleshooting</h1>";
    html += "<button onclick=\"fetch('/control?relay=hopper&state=on')\">Hopper ON</button>";
    html += "<button onclick=\"fetch('/control?relay=hopper&state=off')\">Hopper OFF</button><br>";
    html += "<button onclick=\"fetch('/control?relay=auger&state=on')\">Auger ON</button>";
    html += "<button onclick=\"fetch('/control?relay=auger&state=off')\">Auger OFF</button><br>";
    html += "<button onclick=\"fetch('/control?relay=ignite&state=on')\">Ignite ON</button>";
    html += "<button onclick=\"fetch('/control?relay=ignite&state=off')\">Ignite OFF</button><br>";
    html += "<button onclick=\"fetch('/control?relay=blower&state=on')\">Blower ON</button>";
    html += "<button onclick=\"fetch('/control?relay=blower&state=off')\">Blower OFF</button><br><br>";
    html += "<button onclick=\"fetch('/ota').then(()=>alert('OTA started! Check serial for progress.'))\">OTA Update</button>";

    req->send(200, "text/html", html);
  });

  server.on("/control", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!req->hasParam("relay") || !req->hasParam("state")) { req->send(400, "text/plain", "Missing params"); return; }
    String r = req->getParam("relay")->value();
    String s = req->getParam("state")->value();
    int pin = -1;
    if (r == "hopper") pin = RELAY_HOPPER_FAN_PIN;
    else if (r == "auger") pin = RELAY_AUGER_PIN;
    else if (r == "ignite") pin = RELAY_IGNITER_PIN;
    else if (r == "blower") pin = RELAY_BLOWER_FAN_PIN;
    if (pin < 0) { req->send(400, "text/plain", "Invalid relay"); return; }
    digitalWrite(pin, (s == "on") ? HIGH : LOW);
    req->send(200, "text/plain", "OK");
  });

  server.onNotFound([](AsyncWebServerRequest *req) {
    req->send(404, "text/plain", "Not Found");
  });
  // Live AJAX status endpoint
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *req) {
    double temp   = readTemperature();
    bool ignOn    = digitalRead(RELAY_IGNITER_PIN) == HIGH;
    bool augerOn  = digitalRead(RELAY_AUGER_PIN) == HIGH;
    bool hopperOn = digitalRead(RELAY_HOPPER_FAN_PIN) == HIGH;
    bool blowerOn = digitalRead(RELAY_BLOWER_FAN_PIN) == HIGH;
    String status = getStatus(temp);

    String json = "{";
    json += "\"temp\":" + String(temp,1) + ",";
    json += "\"setpoint\":" + String((int)setpoint) + ",";
    json += "\"status\":\"" + status + "\",";
    json += "\"ignOn\":" + String(ignOn ? "true" : "false") + ",";
    json += "\"augerOn\":" + String(augerOn ? "true" : "false") + ",";
    json += "\"hopperOn\":" + String(hopperOn ? "true" : "false") + ",";
    json += "\"blowerOn\":" + String(blowerOn ? "true" : "false");
    json += "}";
    req->send(200, "application/json", json);
  });

  server.begin();

  //ElegantOTA.begin(&server); // Enable ElegantOTA at /update
  
ElegantOTA.onStart([]() {
  Serial.println("OTA update started!");
  // Turn off all relays during update for safety
  digitalWrite(RELAY_IGNITER_PIN, LOW);
  digitalWrite(RELAY_AUGER_PIN, LOW);
  digitalWrite(RELAY_BLOWER_FAN_PIN, LOW);
  digitalWrite(RELAY_HOPPER_FAN_PIN, LOW);
  grillRunning = false;
});

ElegantOTA.onProgress([](size_t current, size_t final) {
  Serial.printf("OTA Progress: %u%%\r", (current / (final / 100)));
});

ElegantOTA.onEnd([](bool success) {
  if (success) {
    Serial.println("\nOTA update finished successfully!");
    Serial.println("Rebooting in 2 seconds...");
    delay(2000);
    ESP.restart(); // Force restart
  } else {
    Serial.println("\nOTA update failed!");
  }
});

// Then your existing line:
ElegantOTA.begin(&server);

}


