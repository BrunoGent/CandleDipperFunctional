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
#include "logo.h"

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
  DisplayData& data = display.getData();
  data.currentPosition = motorMgr.getCurrentPositionMM();
  data.currentWeight = scaleMgr.getWeightGrams();

  if (M5.Touch.getCount() > 0) {
    auto t = M5.Touch.getDetail();
    if (t.wasPressed() || t.isPressed()) {
      data.justStoppedByTouch = true;
      data.isHomingActive = false;
      data.isDipBotActive = false;
      if (data.currentPage == PAGE_ACTIVE_DIP) {
        data.currentPage = PAGE_STOP_CONFIRM;
        data.stopConfirmEnterTime = millis();
      }
      motorMgr.stopMotor();
      display.markPageChanged(true);
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
  data.s_upSpeed    = prefs.getInt("s6", 60);
  data.s_downSpeed  = prefs.getInt("s7", 50);
  data.s_colLimit   = prefs.getInt("s8", -50); 
  data.s_softLimit  = prefs.getInt("s9", 500);
  data.s_weightDwell = prefs.getInt("s10", 500);
  data.s_softRamp   = prefs.getInt("s11", 50);
  data.s_tmcMode    = prefs.getInt("s12", 1);  // Default: SpreadCycle (1)
  data.s_tmcThreshold = prefs.getInt("s13", 30);
  data.s_irun       = prefs.getInt("s14", 16); // Default IRUN = 16 (~0.6A RMS)
  data.s_ihold      = prefs.getInt("s15", 4);  // Default IHOLD = 4 (~0.15A RMS) -> prevents overheating!
  data.s_iholddelay = prefs.getInt("s16", 6);  // Default IHOLDDELAY = 6
  data.s_brightness = prefs.getInt("bright", 50);
  data.s_theme      = prefs.getInt("theme", 1);
  data.wifiSSID     = prefs.getString("wssid", "Trooli_BB00");
  String wifiPass   = prefs.getString("wpass", "4_j4p4mM3d");

  // Load 5-entry rotating timing history for 4 process types
  for (int t = 0; t < 4; t++) {
    char kHead[16], kCount[16];
    snprintf(kHead, sizeof(kHead), "hh_%d", t);
    snprintf(kCount, sizeof(kCount), "hc_%d", t);
    data.historyHead[t] = prefs.getInt(kHead, 0);
    data.historyCount[t] = prefs.getInt(kCount, 0);
    for (int i = 0; i < 5; i++) {
      char kHist[16];
      snprintf(kHist, sizeof(kHist), "h_%d_%d", t, i);
      data.processHistory[t][i] = prefs.getInt(kHist, 0);
    }
  }

  // Initialize Hardware Managers
  sensorMgr.begin();
  scaleMgr.begin(&prefs);
  motorMgr.begin(&sensorMgr);
  motorMgr.setTMCCurrents(data.s_irun, data.s_ihold, data.s_iholddelay);

  // Initialize display manager
  if (!display.begin(&M5.Display)) {
    Serial.println("Display Manager Initialization Failed!");
  }

  // Attempt Wi-Fi Connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(data.wifiSSID.c_str(), wifiPass.c_str());
  unsigned long startWifi = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startWifi < 1500) {
    delay(50);
  }

  if (WiFi.status() == WL_CONNECTED) {
    syncNtpTime();
  }

  display.markPageChanged(true);
}

void saveProcessHistoryToPrefs() {
  DisplayData& data = display.getData();
  for (int t = 0; t < 4; t++) {
    char kHead[16], kCount[16];
    snprintf(kHead, sizeof(kHead), "hh_%d", t);
    snprintf(kCount, sizeof(kCount), "hc_%d", t);
    prefs.putInt(kHead, data.historyHead[t]);
    prefs.putInt(kCount, data.historyCount[t]);
    for (int i = 0; i < 5; i++) {
      char kHist[16];
      snprintf(kHist, sizeof(kHist), "h_%d_%d", t, i);
      prefs.putInt(kHist, data.processHistory[t][i]);
    }
  }
}

enum DipPhase {
  DIP_PHASE_IDLE = 0,
  DIP_PHASE_PRESTART_COUNTDOWN,
  DIP_PHASE_LOWERING,
  DIP_PHASE_HOLDING,
  DIP_PHASE_RAISING,
  DIP_PHASE_COOLING
};

static DipPhase currentDipPhase = DIP_PHASE_IDLE;
static unsigned long phaseStartTime = 0;
static unsigned long holdDurationMs = 0;

void processActiveDipping() {
  DisplayData& data = display.getData();

  // If page changed away from active dipping or stop confirm, reset phase
  if (data.currentPage != PAGE_ACTIVE_DIP && data.currentPage != PAGE_STOP_CONFIRM) {
    if (currentDipPhase != DIP_PHASE_IDLE) {
      currentDipPhase = DIP_PHASE_IDLE;
      motorMgr.stopMotor();
    }
    return;
  }

  // If on confirmation modal, pause motor motion
  if (data.currentPage == PAGE_STOP_CONFIRM) {
    motorMgr.stopMotor();
    return;
  }

  // Initialize active dipping phase
  if (currentDipPhase == DIP_PHASE_IDLE) {
    currentDipPhase = DIP_PHASE_LOWERING;
    data.currentDipCount = 1;
    data.dipStartTime = millis();
    char pTxt[32];
    snprintf(pTxt, sizeof(pTxt), "Dip 1: Lowering...");
    data.currentPhaseText = String(pTxt);
    display.markPageChanged(true);
  }

  char pTxt[64];

  switch (currentDipPhase) {
    case DIP_PHASE_LOWERING: {
      float speed = (float)data.s_downSpeed;
      if (speed <= 0.0f) speed = 20.0f;
      motorMgr.applyConfigMode(data.s_tmcMode, speed, data.s_tmcThreshold);

      digitalWrite(PIN_MOTOR_DIR, LOW); // DOWN direction
      float stepsPerSec = speed * MotorManager::STEPS_PER_MM;
      unsigned long delayUs = (unsigned long)(1000000.0f / stepsPerSec);
      if (delayUs < 20) delayUs = 20;

      unsigned long startTimeout = millis();
      float stepDistMM = 1.0f / MotorManager::STEPS_PER_MM;

      while (!sensorMgr.readRawCapSensor()) {
        if (checkManualStop()) {
          motorMgr.stopMotor();
          return;
        }

        if (data.s_softLimit > 0 && data.currentPosition >= (float)data.s_softLimit) {
          Serial.println("Soft Limit Guard reached!");
          break;
        }

        digitalWrite(PIN_MOTOR_STEP, HIGH);
        delayMicroseconds(3);
        digitalWrite(PIN_MOTOR_STEP, LOW);

        data.currentPosition += stepDistMM;
        motorMgr.setCurrentPositionMM(data.currentPosition);

        delayMicroseconds(delayUs);

        if (millis() - startTimeout > 35000) break; // Timeout guard
      }

      motorMgr.stopMotor();
      currentDipPhase = DIP_PHASE_HOLDING;
      phaseStartTime = millis();
      int holdSec = (data.currentDipCount == 1) ? data.s_dip1Time : data.s_subDipTime;
      holdDurationMs = (unsigned long)holdSec * 1000;

      snprintf(pTxt, sizeof(pTxt), "Dip %d: Holding (%ds)", data.currentDipCount, holdSec);
      data.currentPhaseText = String(pTxt);
      display.markPageChanged(true);
      break;
    }

    case DIP_PHASE_HOLDING:
      motorMgr.stopMotor();
      {
        unsigned long elapsed = millis() - phaseStartTime;
        if (elapsed >= holdDurationMs) {
          currentDipPhase = DIP_PHASE_RAISING;
          snprintf(pTxt, sizeof(pTxt), "Dip %d: Raising...", data.currentDipCount);
          data.currentPhaseText = String(pTxt);
          display.markPageChanged(true);
        } else {
          int remSec = (int)((holdDurationMs - elapsed + 999) / 1000);
          snprintf(pTxt, sizeof(pTxt), "Dip %d: Holding (%ds)", data.currentDipCount, remSec);
          if (data.currentPhaseText != String(pTxt)) {
            data.currentPhaseText = String(pTxt);
            display.markPageChanged(true);
          }
        }
      }
      break;

    case DIP_PHASE_RAISING: {
      float speed = (float)data.s_upSpeed;
      if (speed <= 0.0f) speed = 30.0f;
      motorMgr.applyConfigMode(data.s_tmcMode, speed, data.s_tmcThreshold);

      digitalWrite(PIN_MOTOR_DIR, HIGH); // UP direction
      float stepsPerSec = speed * MotorManager::STEPS_PER_MM;
      unsigned long delayUs = (unsigned long)(1000000.0f / stepsPerSec);
      if (delayUs < 20) delayUs = 20;

      unsigned long startTimeout = millis();
      float stepDistMM = 1.0f / MotorManager::STEPS_PER_MM;

      while (!sensorMgr.readRawTopLimit() && data.currentPosition > 0.0f) {
        if (checkManualStop()) {
          motorMgr.stopMotor();
          return;
        }

        digitalWrite(PIN_MOTOR_STEP, HIGH);
        delayMicroseconds(3);
        digitalWrite(PIN_MOTOR_STEP, LOW);

        data.currentPosition -= stepDistMM;
        if (data.currentPosition < 0.0f) data.currentPosition = 0.0f;
        motorMgr.setCurrentPositionMM(data.currentPosition);

        delayMicroseconds(delayUs);

        if (millis() - startTimeout > 35000) break; // Timeout guard
      }

      if (sensorMgr.readRawTopLimit()) {
        data.currentPosition = 0.0f;
        motorMgr.setCurrentPositionMM(0.0f);
      }

      motorMgr.stopMotor();

      // Dwell time before scale reading
      if (data.s_weightDwell > 0) {
        delay(data.s_weightDwell);
      }
      scaleMgr.update();
      data.currentWeight = scaleMgr.getWeightGrams();

      // Check if dipping process finished
      bool isSlim = data.isActiveSlimProfile;
      bool isWeight = data.isActiveWeightBased;
      int targetWeight = isSlim ? data.s_slimWt : data.s_stdWt;
      int targetDips = isSlim ? data.s_slimDips : data.s_stdDips;

      bool processFinished = false;
      if (isWeight) {
        if (data.currentWeight >= (float)targetWeight) {
          processFinished = true;
        }
      } else {
        if (data.currentDipCount >= targetDips) {
          processFinished = true;
        }
      }

      if (processFinished) {
        currentDipPhase = DIP_PHASE_IDLE;
        int finalElapsedSec = (int)((millis() - data.dipStartTime) / 1000);
        display.endDippingProcess(false, finalElapsedSec);
        saveProcessHistoryToPrefs();
        M5.Speaker.tone(1000, 300);
        delay(100);
        M5.Speaker.tone(1500, 500);
      } else {
        currentDipPhase = DIP_PHASE_COOLING;
        phaseStartTime = millis();
        holdDurationMs = (unsigned long)data.s_subDipTime * 1000;
        snprintf(pTxt, sizeof(pTxt), "Dip %d: Cooling (%ds)", data.currentDipCount, data.s_subDipTime);
        data.currentPhaseText = String(pTxt);
        display.markPageChanged(true);
      }
      break;
    }

    case DIP_PHASE_COOLING:
      motorMgr.stopMotor();
      {
        unsigned long elapsed = millis() - phaseStartTime;
        if (elapsed >= holdDurationMs) {
          data.currentDipCount++;
          currentDipPhase = DIP_PHASE_LOWERING;
          snprintf(pTxt, sizeof(pTxt), "Dip %d: Lowering...", data.currentDipCount);
          data.currentPhaseText = String(pTxt);
          display.markPageChanged(true);
        } else {
          int remSec = (int)((holdDurationMs - elapsed + 999) / 1000);
          snprintf(pTxt, sizeof(pTxt), "Dip %d: Cooling (%ds)", data.currentDipCount, remSec);
          if (data.currentPhaseText != String(pTxt)) {
            data.currentPhaseText = String(pTxt);
            display.markPageChanged(true);
          }
        }
      }
      break;

    default:
      break;
  }
}

void loop() {
  M5.update();
  DisplayData& data = display.getData();

  // Update hardware sensors & scale polling
  sensorMgr.update();
  scaleMgr.update();

  // Sync current soft limit setting to motor manager
  motorMgr.setMaxSoftLimitMM((float)data.s_softLimit);

  // Sync current position and load cell weight into DisplayData container
  data.currentPosition = motorMgr.getCurrentPositionMM();
  data.currentWeight = scaleMgr.getWeightGrams();
  data.limitSwitchOn = sensorMgr.isTopLimitHit();
  data.capSensorOn = sensorMgr.isCapSensorTriggered();

  if (data.portalActive) {
    dnsServer.processNextRequest();
    webServer.handleClient();
  }

  // Live screen refresh on Manual Page when scale weight, position, or manual jog occurs
  if (data.currentPage == PAGE_MANUAL) {
    if (data.isUpPressed || data.isDownPressed) {
      bool dirUp = data.isUpPressed;
      unsigned long startMs = millis();
      while (millis() - startMs < 50) {
        sensorMgr.update();
        float currentPos = motorMgr.getCurrentPositionMM();
        if (dirUp) {
          if (sensorMgr.isTopLimitHit() || currentPos <= 0.0f) {
            data.isUpPressed = false;
            motorMgr.stopMotor();
            display.markPageChanged(true);
            break;
          }
        } else {
          if (sensorMgr.isCapSensorTriggered() || (data.s_softLimit > 0 && currentPos >= (float)data.s_softLimit)) {
            data.isDownPressed = false;
            motorMgr.stopMotor();
            display.markPageChanged(true);
            break;
          }
        }
        motorMgr.stepMotor(dirUp, (float)data.manualSpeed);
        delayMicroseconds(5);
      }
      data.currentPosition = motorMgr.getCurrentPositionMM();
    } else if (!data.isHomingActive && !data.isDipBotActive) {
      motorMgr.stopMotor();
      static unsigned long lastScaleRefresh = 0;
      if (millis() - lastScaleRefresh >= 250) {
        lastScaleRefresh = millis();
        if (abs(data.currentWeight - lastWtDisplayed) >= 0.2f || abs(data.currentPosition - lastPosDisplayed) >= 0.1f) {
          lastWtDisplayed = data.currentWeight;
          lastPosDisplayed = data.currentPosition;
          display.markPageChanged(true);
        }
      }
    }
  }

  // Active dipping process state machine execution
  if (data.currentPage == PAGE_ACTIVE_DIP || data.currentPage == PAGE_STOP_CONFIRM) {
    processActiveDipping();
  } else {
    if (currentDipPhase != DIP_PHASE_IDLE) {
      currentDipPhase = DIP_PHASE_IDLE;
      motorMgr.stopMotor();
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
      currentDipPhase = DIP_PHASE_IDLE;
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
      if (data.activeSetting >= 0 && data.activeSetting <= 16) {
        char keyStr[8];
        snprintf(keyStr, sizeof(keyStr), "s%d", data.activeSetting);
        prefs.putInt(keyStr, display.getSettingValue(data.activeSetting));
      }
      motorMgr.setTMCCurrents(data.s_irun, data.s_ihold, data.s_iholddelay);
      motorMgr.applyConfigMode(data.s_tmcMode, 0, data.s_tmcThreshold);
      break;

    case UI_EVENT_MOTOR_ENABLE_TOGGLE:
      motorMgr.setMotorEnable(data.isMotorEnabled);
      break;

    case UI_EVENT_THEME_CHANGED:
      prefs.putInt("theme", data.s_theme);
      break;

    case UI_EVENT_BRIGHTNESS_CHANGED:
      prefs.putInt("bright", data.s_brightness);
      break;

    case UI_EVENT_START_DIP:
      Serial.println("UI Event: Auto-homing to 0mm before dipping process...");
      data.currentPage = PAGE_ACTIVE_DIP;
      data.currentPhaseText = "Auto-Homing to 0mm...";
      display.markPageChanged(true);
      display.renderCurrentPage();

      // Perform homing motion to top limit switch first so 0mm is accurately calibrated
      motorMgr.performHoming((float)data.s_upSpeed);

      motorMgr.stopMotor();
      scaleMgr.tare(10);
      scaleMgr.update();
      data.currentWeight = 0.0f;
      data.currentDipCount = 1;
      data.dipStartTime = millis();
      phaseStartTime = millis();
      currentDipPhase = DIP_PHASE_LOWERING;
      data.currentPhaseText = "Dip 1: Lowering...";
      display.markPageChanged(true);
      M5.Speaker.tone(1800, 200);
      saveProcessHistoryToPrefs();
      break;

    case UI_EVENT_ABORT_DIP:
      Serial.println("UI Event: Aborting dipping process...");
      currentDipPhase = DIP_PHASE_IDLE;
      motorMgr.stopMotor();
      M5.Speaker.tone(300, 500);
      break;

    case UI_EVENT_FINISH_DIP:
      Serial.println("UI Event: Dipping process finished");
      currentDipPhase = DIP_PHASE_IDLE;
      motorMgr.stopMotor();
      M5.Speaker.tone(800, 500);
      break;

    default:
      break;
  }

  // 3. Render Double-Buffered Screen Canvas
  display.renderCurrentPage();
}
