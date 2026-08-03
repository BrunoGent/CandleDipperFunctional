#include <M5Unified.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <time.h>

#include "DisplayManager.h"
#include "PinConfig.h"
#include "SensorManager.h"
#include "ScaleManager.h"
#include "MotorManager.h"

// --- Global Managers & Objects ---
DisplayManager display;
Preferences prefs;
SensorManager sensorMgr;
ScaleManager scaleMgr;
MotorManager motorMgr;

// --- Wi-Fi & Web Portal Variables ---
WebServer webServer(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

int lastMinute = -1;
float lastPosDisplayed = -999.0f;
float lastWtDisplayed = -999.0f;

// --- Helper Functions ---
bool checkManualStop() {
  M5.update();
  if (M5.Touch.getCount() > 0) {
    auto t = M5.Touch.getDetail();
    if (t.wasPressed() || t.isPressed()) {
      return true; // Screen touched -> Stop motion immediately
    }
  }
  return false;
}

void syncNtpTime() {
  if (WiFi.status() == WL_CONNECTED) {
    configTzTime("GMT0BST,M3.5.0/1,M10.5.0/2", "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5000)) {
      m5::rtc_datetime_t dt;
      dt.date.year = timeinfo.tm_year + 1900;
      dt.date.month = timeinfo.tm_mon + 1;
      dt.date.date = timeinfo.tm_mday;
      dt.date.weekDay = timeinfo.tm_wday;
      dt.time.hours = timeinfo.tm_hour;
      dt.time.minutes = timeinfo.tm_min;
      dt.time.seconds = timeinfo.tm_sec;
      M5.Rtc.setDateTime(dt);
      Serial.println("RTC updated via NTP!");
    }
  }
}

void startWebPortal() {
  DisplayData& data = display.getData();
  data.portalActive = true;
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAP("BeeMine-Setup");

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  webServer.on("/", []() {
    DisplayData& d = display.getData();
    String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"
                  "<style>body{font-family:sans-serif;background:#232323;color:#fff;padding:20px;text-align:center;}"
                  "h2{color:#FFEB82;} input,button{width:100%;padding:12px;margin:8px 0;box-sizing:border-box;border-radius:6px;border:none;}"
                  "input{background:#333;color:#fff;} button{background:#FFEB82;color:#000;font-weight:bold;cursor:pointer;}</style></head><body>"
                  "<h2>Bee Mine Wi-Fi Setup</h2>"
                  "<form action='/save' method='POST'>"
                  "<input type='text' name='ssid' placeholder='Wi-Fi Name' required value='" + d.wifiSSID + "'><br>"
                  "<input type='password' name='pass' placeholder='Password' required><br>"
                  "<button type='submit'>Save & Connect</button>"
                  "</form></body></html>";
    webServer.send(200, "text/html", html);
  });

  webServer.on("/save", []() {
    if (webServer.hasArg("ssid") && webServer.hasArg("pass")) {
      DisplayData& d = display.getData();
      d.wifiSSID = webServer.arg("ssid");
      String wifiPass = webServer.arg("pass");
      
      prefs.putString("wssid", d.wifiSSID);
      prefs.putString("wpass", wifiPass);

      webServer.send(200, "text/html", "<html><body style='background:#232323;color:#FFEB82;text-align:center;padding:30px;'><h2>Credentials Saved!</h2><p>Reconnecting device to Wi-Fi...</p></body></html>");
      delay(1500);
      d.portalActive = false;
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
      WiFi.begin(d.wifiSSID.c_str(), wifiPass.c_str());
      display.setCurrentPage(PAGE_SETTINGS_HUB);
    }
  });

  webServer.begin();
  display.setCurrentPage(PAGE_WIFI_PORTAL);
}

void stopWebPortal() {
  DisplayData& data = display.getData();
  data.portalActive = false;
  dnsServer.stop();
  webServer.close();
  WiFi.softAPdisconnect(true);
  
  String wpass = prefs.getString("wpass", "4_j4p4mM3d");
  WiFi.mode(WIFI_STA);
  WiFi.begin(data.wifiSSID.c_str(), wpass.c_str());
  
  display.setCurrentPage(PAGE_SETTINGS_HUB);
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  Serial.begin(115200); 

  // Load preferences
  prefs.begin("candle_app", false);
  DisplayData& data = display.getData();
  data.s_slimWt     = prefs.getInt("s0", 1500);
  data.s_stdWt      = prefs.getInt("s1", 2500);
  data.s_slimDips   = prefs.getInt("s2", 15);
  data.s_stdDips    = prefs.getInt("s3", 25);
  data.s_dip1Time   = prefs.getInt("s4", 10);
  data.s_subDipTime = prefs.getInt("s5", 4);
  data.s_downSpeed  = prefs.getInt("s6", 50);
  data.s_upSpeed    = prefs.getInt("s7", 60);
  data.s_colLimit   = prefs.getInt("s8", -50); 
  data.s_brightness = prefs.getInt("bright", 50);
  data.s_theme      = prefs.getInt("theme", 1);
  data.wifiSSID     = prefs.getString("wssid", "Trooli_BB00");
  String wifiPass   = prefs.getString("wpass", "4_j4p4mM3d");

  // Initialize Hardware Managers
  sensorMgr.begin();
  scaleMgr.begin(&prefs);
  motorMgr.begin(&sensorMgr);

  // Initialize display manager
  if (!display.begin(&M5.Display)) {
    Serial.println("Display Manager Initialization Failed!");
  }

  // Boot Splash Screen
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(M5.Display.color565(255, 235, 130), TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextSize(3.0);
  M5.Display.drawString("BEE MINE", 160, 100);
  M5.Display.setTextSize(2.0);
  M5.Display.drawString("DORSET", 160, 140);

  // Attempt Wi-Fi Connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(data.wifiSSID.c_str(), wifiPass.c_str());
  unsigned long startWifi = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startWifi < 3000) {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    syncNtpTime();
  }

  delay(1000);
  display.markPageChanged(true);
}

void loop() {
  M5.update();
  DisplayData& data = display.getData();

  // Update hardware sensors & scale polling
  sensorMgr.update();
  scaleMgr.update();

  // Sync current position and load cell weight into DisplayData container
  data.currentPosition = motorMgr.getCurrentPositionMM();
  data.currentWeight = scaleMgr.getWeightGrams();
  data.limitSwitchOn = sensorMgr.isTopLimitHit();
  data.capSensorOn = sensorMgr.isCapSensorTriggered();

  if (data.portalActive) {
    dnsServer.processNextRequest();
    webServer.handleClient();
  }

  // Handle continuous movement on Manual Page
  if (data.currentPage == PAGE_MANUAL) {
    if (data.isUpPressed) {
      // Move UP at manualSpeed mm/s smoothly
      motorMgr.stepMotorBurst(true, (float)data.manualSpeed, 15);
    } else if (data.isDownPressed) {
      // Move DOWN at manualSpeed mm/s smoothly
      motorMgr.stepMotorBurst(false, (float)data.manualSpeed, 15);
    } else {
      motorMgr.stopMotor();
    }

    // Refresh display if position or weight changed noticeably
    if (abs(data.currentPosition - lastPosDisplayed) >= 0.5f || abs(data.currentWeight - lastWtDisplayed) >= 0.5f) {
      lastPosDisplayed = data.currentPosition;
      lastWtDisplayed = data.currentWeight;
      display.markPageChanged(true);
    }
  }

  // Periodic RTC clock update on top banner
  m5::rtc_datetime_t dt;
  if (M5.Rtc.getDateTime(&dt)) {
    if (dt.time.minutes != lastMinute) {
       lastMinute = dt.time.minutes;
       if (data.currentPage < PAGE_ACTIVE_DIP && !data.showNumpad) {
         display.markPageChanged(true);
       }
    }
  }

  // Periodic ticker during active dipping
  if (data.currentPage == PAGE_ACTIVE_DIP && (millis() - data.lastUiTick >= 1000)) {
    data.lastUiTick = millis(); 
    display.markPageChanged(true); 
  }

  // 1. Process Touch Events & User Interactions
  UiEvent event = display.updateTouch();

  // 2. Handle Event Callbacks
  switch (event) {
    case UI_EVENT_MANUAL_HOME:
      Serial.println("Executing Homing Sequence...");
      data.isHomingActive = true;
      display.markPageChanged(true);
      display.renderCurrentPage();

      motorMgr.performHoming((float)data.s_upSpeed, checkManualStop);

      data.isHomingActive = false;
      display.markPageChanged(true);
      break;

    case UI_EVENT_MANUAL_DIP_BOT:
      Serial.println("Executing Dip Bot Sequence...");
      data.isDipBotActive = true;
      display.markPageChanged(true);
      display.renderCurrentPage();

      motorMgr.performDipBot((float)data.s_downSpeed, (float)data.s_upSpeed, data.s_subDipTime, checkManualStop);

      data.isDipBotActive = false;
      display.markPageChanged(true);
      break;

    case UI_EVENT_MANUAL_STOP:
      Serial.println("UI Event: STOP requested!");
      motorMgr.stopMotor();
      data.isHomingActive = false;
      data.isDipBotActive = false;
      display.markPageChanged(true);
      break;

    case UI_EVENT_MANUAL_TARE:
      Serial.println("Taring Scale (10 samples)...");
      scaleMgr.tare(10);
      display.markPageChanged(true);
      break;

    case UI_EVENT_START_WIFI_PORTAL:
      startWebPortal();
      break;

    case UI_EVENT_STOP_WIFI_PORTAL:
      stopWebPortal();
      break;

    case UI_EVENT_SETTING_UPDATED:
      // Persist modified setting to Preferences
      if (data.activeSetting >= 0 && data.activeSetting <= 8) {
        char keyStr[8];
        snprintf(keyStr, sizeof(keyStr), "s%d", data.activeSetting);
        prefs.putInt(keyStr, display.getSettingValue(data.activeSetting));
      }
      break;

    case UI_EVENT_THEME_CHANGED:
      prefs.putInt("theme", data.s_theme);
      break;

    case UI_EVENT_BRIGHTNESS_CHANGED:
      prefs.putInt("bright", data.s_brightness);
      break;

    case UI_EVENT_START_DIP:
      Serial.println("UI Event: Starting dipping process...");
      break;

    case UI_EVENT_ABORT_DIP:
      Serial.println("UI Event: Aborting dipping process...");
      M5.Speaker.tone(300, 500);
      break;

    case UI_EVENT_FINISH_DIP:
      Serial.println("UI Event: Dipping process finished");
      M5.Speaker.tone(800, 500);
      break;

    default:
      break;
  }

  // 3. Render Double-Buffered Screen Canvas
  display.renderCurrentPage();
}
