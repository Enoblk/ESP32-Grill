#include "GrillWebServer.h"
#include "Globals.h"
#include "Ignition.h"
#include "Utility.h"
#include <Update.h>
#include <WiFi.h>

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Grill Controller</title>
<style>
body{font-family:Arial,sans-serif;background:#111;color:#f7f7f7;margin:0;padding:16px}
main{max-width:760px;margin:auto}.panel{border:1px solid #333;border-radius:8px;padding:16px;margin:12px 0;background:#1b1b1b}
.temp{font-size:56px;font-weight:700}.muted{color:#aaa}.row{display:flex;gap:10px;flex-wrap:wrap}
button,a{border:0;border-radius:6px;padding:12px 14px;background:#2563eb;color:white;text-decoration:none;font-weight:700}
.danger{background:#dc2626}.ok{background:#059669}.warn{background:#b45309}
input{padding:11px;border-radius:6px;border:1px solid #555;background:#222;color:white;width:90px}
.relay{display:grid;grid-template-columns:repeat(4,minmax(90px,1fr));gap:8px}.on{color:#4ade80}.off{color:#888}
</style></head><body><main>
<h1>Grill Controller</h1>
<section class="panel"><div class="muted">Grill Temp</div><div id="temp" class="temp">--</div>
<div>Target: <b id="target">--</b> F | <span id="status">--</span></div></section>
<section class="panel row">
<button class="ok" onclick="cmd('/start')">Start</button>
<button class="danger" onclick="cmd('/stop')">Stop</button>
<button class="warn" onclick="cmd('/prime_auger')">Prime 30s</button>
<input id="set" type="number" min="150" max="500" step="5" value="225">
<button onclick="cmd('/set_temp?temp='+document.getElementById('set').value)">Set Temp</button>
<a href="/wifi">WiFi</a><a href="/update">OTA</a>
</section>
<section class="panel"><h3>Relays</h3><div class="relay">
<div>Igniter <b id="igniter">--</b></div><div>Auger <b id="auger">--</b></div>
<div>Hopper <b id="hopper">--</b></div><div>Blower <b id="blower">--</b></div>
</div></section>
<section class="panel"><pre id="raw">{}</pre></section>
</main><script>
function yn(v){return v?'<span class=on>ON</span>':'<span class=off>OFF</span>'}
function refresh(){fetch('/status_all').then(r=>r.json()).then(d=>{
temp.textContent=d.grillTempValid?d.grillTemp.toFixed(1)+' F':'SENSOR ERROR';
target.textContent=d.setpoint.toFixed(0); set.value=d.setpoint.toFixed(0); status.textContent=d.status;
igniter.innerHTML=yn(d.ignOn); auger.innerHTML=yn(d.augerOn); hopper.innerHTML=yn(d.hopperOn); blower.innerHTML=yn(d.blowerOn);
raw.textContent=JSON.stringify(d,null,2);
}).catch(e=>status.textContent='offline')}
function cmd(u){fetch(u).then(r=>r.text()).then(t=>{status.textContent=t;setTimeout(refresh,300)}).catch(e=>alert(e))}
refresh(); setInterval(refresh,2000);
</script></body></html>
)HTML";

static void sendStatus() {
  double grillTemp = readGrillTemperature();
  bool valid = isValidTemperature(grillTemp) && grillTemperatureHealthy();
  String json;
  json.reserve(384);
  json += "{";
  json += "\"grillTemp\":" + String(valid ? grillTemp : 0.0, 1) + ",";
  json += "\"grillTempValid\":" + String(valid ? "true" : "false") + ",";
  json += "\"temperatureAgeMs\":" + String(grillTemperatureAgeMs()) + ",";
  json += "\"setpoint\":" + String(setpoint, 1) + ",";
  json += "\"status\":\"" + ignition_get_status_string() + "\",";
  json += "\"grillRunning\":" + String(grillRunning ? "true" : "false") + ",";
  json += "\"primeActive\":" + String(auger_prime_active() ? "true" : "false") + ",";
  json += "\"primeRemainingMs\":" + String(auger_prime_remaining_ms()) + ",";
  json += "\"ignOn\":" + String(digitalRead(RELAY_IGNITER_PIN) == HIGH ? "true" : "false") + ",";
  json += "\"augerOn\":" + String(digitalRead(RELAY_AUGER_PIN) == HIGH ? "true" : "false") + ",";
  json += "\"hopperOn\":" + String(digitalRead(RELAY_HOPPER_FAN_PIN) == HIGH ? "true" : "false") + ",";
  json += "\"blowerOn\":" + String(digitalRead(RELAY_BLOWER_FAN_PIN) == HIGH ? "true" : "false") + ",";
  json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";
  server.send(200, "application/json", json);
}

void setup_grill_server() {
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  server.on("/status_all", HTTP_GET, sendStatus);

  server.on("/start", HTTP_GET, []() {
    double temp = readGrillTemperature();
    ignition_start(temp);
    server.send(ignition_has_failed() ? 503 : 200, "text/plain", ignition_get_status_string());
  });

  server.on("/stop", HTTP_GET, []() {
    ignition_stop();
    server.send(200, "text/plain", "Stopped");
  });

  server.on("/set_temp", HTTP_GET, []() {
    if (!server.hasArg("temp")) {
      server.send(400, "text/plain", "Missing temp");
      return;
    }
    setpoint = server.arg("temp").toDouble();
    clamp_setpoint();
    save_setpoint();
    server.send(200, "text/plain", "Setpoint " + String(setpoint, 0) + " F");
  });

  server.on("/prime_auger", HTTP_GET, []() {
    if (auger_prime_start(30000)) {
      server.send(202, "text/plain", "Prime started");
    } else {
      server.send(409, "text/plain", "Prime unavailable while running or already active");
    }
  });

  server.on("/wifi", HTTP_GET, []() {
    String html = "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<style>body{font-family:Arial;background:#111;color:white;padding:16px}input,button{padding:10px;margin:6px;border-radius:6px}</style></head><body>";
    html += "<h1>WiFi</h1><p>STA: " + String(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "not connected") + "</p>";
    html += "<p>AP: " + WiFi.softAPIP().toString() + "</p>";
    html += "<form method='post' action='/wifi_save'><input name='ssid' placeholder='SSID' required><br><input name='password' placeholder='Password' type='password'><br><button>Save</button></form>";
    html += "<p><a href='/'>Back</a></p></body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/wifi_save", HTTP_POST, []() {
    if (!server.hasArg("ssid")) {
      server.send(400, "text/plain", "Missing SSID");
      return;
    }
    String ssid = server.arg("ssid");
    String password = server.hasArg("password") ? server.arg("password") : "";
    if (!preferences.begin("wifi", false)) {
      server.send(500, "text/plain", "Preferences failed");
      return;
    }
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    server.send(200, "text/plain", "Saved; connecting");
  });

  server.on("/update", HTTP_GET, []() {
    String html = "<!doctype html><html><body><h1>Firmware Update</h1>";
    html += "<form method='POST' action='/update' enctype='multipart/form-data'>";
    html += "<input type='file' name='update'><button>Upload</button></form>";
    html += "<p><a href='/'>Back</a></p></body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/update", HTTP_POST, []() {
    bool ok = !Update.hasError();
    server.send(ok ? 200 : 500, "text/plain", ok ? "Update OK, rebooting" : "Update failed");
    delay(500);
    if (ok) ESP.restart();
  }, []() {
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.println("OTA update starting; stopping grill");
      ignition_stop();
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) Update.printError(Serial);
    } else if (upload.status == UPLOAD_FILE_END) {
      if (!Update.end(true)) Update.printError(Serial);
    }
  });

  server.onNotFound([]() { server.send(404, "text/plain", "Not Found"); });

  server.begin();
  Serial.println("Web server started");
}

void handle_grill_server() {
  server.handleClient();
}
