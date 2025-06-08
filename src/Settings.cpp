#include "Settings.h"
#include "Globals.h"
#include <WiFi.h>
#include <Preferences.h>
#include <ESPmDNS.h>

void load_settings() {
  prefs.begin("grill", true);
  String ssd = prefs.getString("ssid", "");
  String pwp = prefs.getString("pass", "");
  useDHCP    = prefs.getBool("dhcp", true);
  ssd.toCharArray(ssid, sizeof(ssid));
  pwp.toCharArray(password, sizeof(password));
  prefs.end();
}

void setup_wifi_and_mdns() {
  bool connected = false;
  if (strlen(ssid) > 0) {
    if (!useDHCP) WiFi.config(IPAddress(192,168,1,200), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
    WiFi.begin(ssid, password);
    unsigned long t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < 10000) delay(200);
    connected = (WiFi.status() == WL_CONNECTED);
  }
  if (connected) {
    MDNS.begin("grill");
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    WiFi.softAP("GrillSetupAP", "grill1234");
    Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
  }
}
