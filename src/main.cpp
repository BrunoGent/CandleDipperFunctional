/**
 * ============================================================================
 * BEE MINE CANDLE DIPPER - MAIN FIRMWARE
 * ============================================================================
 * Platform: M5Stack CoreS3 / Core2 (ESP32-S3 / ESP32)
 * Framework: Arduino / M5Unified
 * 
 * Hardware Subsystems:
 * 1. DisplayManager: 320x240 M5Canvas double-buffered UI, multi-theme engine,
 *    live telemetry dashboards, settings hub, and active dipping animation.
 * 2. MotorManager: Trinamic TMC2209 silent stepper driver over single-wire UART,
 *    supporting dynamic StealthChop2 / SpreadCycle mode switching, 4-stage
 *    homing calibration, and lead-screw motion.
 * 3. SensorManager: Optical / mechanical top limit switch (NC to GND) and
 *    24V NPN optocoupled capacitive wax surface level detector.
 * 4. ScaleManager: HX711 24-bit strain-gauge load cell ADC with 10Hz non-blocking
 *    readings, exponential moving average (EMA) filter, tare, and calibration.
 * 5. Wi-Fi Web Portal: SoftAP captive portal for headless Wi-Fi configuration and
 *    NTP time synchronization.
 * ============================================================================
 */

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
#include "SoundManager.h"


// --- Global Managers & Objects ---
DisplayManager display; // Handles all LCD drawing, touch hit-testing, and UI state
Preferences prefs;      // ESP32 Non-Volatile Storage (NVS) for saving settings across reboots
SensorManager sensorMgr;// Reads and debounces top limit switch and bottom wax capacitive sensor
ScaleManager scaleMgr;  // Polls the HX711 24-bit load cell amplifier at 10Hz
MotorManager motorMgr;  // Controls the TMC2209 stepper driver via STEP/DIR/EN and Serial2 UART
SoundManager soundMgr;  // Drives the M5Stack CoreS3 SE AW88298 I2S speaker for startup/finish/alert tunes

// --- Wi-Fi & Web Portal Variables ---
WebServer webServer(80);// Local HTTP server on port 80 for the Wi-Fi setup form
DNSServer dnsServer;    // Captive portal DNS server that redirects all domains to 192.168.4.1
const byte DNS_PORT = 53;

int lastMinute = -1;              // Tracks the last RTC minute rendered so the top clock only redraws once per minute
float lastPosDisplayed = -999.0f; // Tracks last displayed Z position on Manual screen to avoid unnecessary redraws
float lastWtDisplayed = -999.0f;  // Tracks last displayed weight on Manual screen to avoid unnecessary redraws

// --- Helper Functions ---
static unsigned long g_actionStartTime = 0;

// Records the timestamp when a blocking motion action (Homing or Dip Bot) starts
// so the initial button press on the touchscreen doesn't immediately trigger an emergency stop.
void markActionStarted() {
  g_actionStartTime = millis();
}

// Callback passed into blocking motor loops (Homing, Dip Bot, and Dipping Lower/Raise phases).
// Checks every 40ms if the user touched the screen to emergency-stop motion.
bool checkManualStop() {
  static unsigned long lastCheckTime = 0;
  unsigned long now = millis();
  // Throttle check to once every 40ms to keep microsecond stepper pulse timing smooth and jitter-free
  if (now - lastCheckTime < 40) {
    return false;
  }
  lastCheckTime = now;

  M5.update();
  soundMgr.update();
  DisplayData& data = display.getData();
  data.currentPosition = motorMgr.getCurrentPositionMM();
  data.currentWeight = scaleMgr.getWeightGrams();

  // Ignore touches for initial 400ms so button press that started action doesn't immediately abort it
  if (now - g_actionStartTime < 400) {
    return false;
  }

  if (M5.Touch.getCount() > 0) {
    auto t = M5.Touch.getDetail();
    if (t.wasPressed() || t.isPressed()) {
      data.justStoppedByTouch = true;
      data.isHomingActive = false;
      data.isDipBotActive = false;
      data.isMeltBasesActive = false;
      data.isCleanJigActive = false;
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

// Connects to UK NTP time servers (with GMT/BST daylight saving rules) and updates the M5Stack hardware RTC
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

// Starts an open Wi-Fi Access Point ("BeeMine-Setup") and captive portal web page at http://192.168.4.1
// allowing the user to enter their workshop Wi-Fi SSID and password from a phone.
void startWebPortal() {
  DisplayData& data = display.getData();
  data.portalActive = true;
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAP("BeeMine-Setup");

  // Redirect all DNS queries (*) to the ESP32's SoftAP IP (192.168.4.1)
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  // Serve the HTML Wi-Fi configuration form
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

  // Handle form submission: save new Wi-Fi credentials to NVS and reconnect in Station mode
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
      display.setCurrentPage(PAGE_MACHINE_HUB);
    }
  });

  webServer.begin();
  display.setCurrentPage(PAGE_WIFI_PORTAL);
}

// Shuts down the SoftAP captive portal and reconnects to the saved Wi-Fi network
void stopWebPortal() {
  DisplayData& data = display.getData();
  data.portalActive = false;
  dnsServer.stop();
  webServer.close();
  WiFi.softAPdisconnect(true);
  
  String wpass = prefs.getString("wpass", "");
  if (data.wifiSSID.length() > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(data.wifiSSID.c_str(), wpass.c_str());
  }
  
  display.setCurrentPage(PAGE_MACHINE_HUB);
}

// Arduino setup() runs once on power-up or reset
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  Serial.begin(115200); 

  // Load saved user settings from ESP32 non-volatile flash (namespace "candle_app")
  prefs.begin("candle_app", false);
  DisplayData& data = display.getData();
  data.s_slimWt        = prefs.getInt("s0", 1500);
  data.s_stdWt         = prefs.getInt("s1", 2500);
  data.s_slimDips      = prefs.getInt("s2", 15);
  data.s_stdDips       = prefs.getInt("s3", 25);
  data.s_dip1Time      = prefs.getInt("s4", 10);
  data.s_subDipTime    = prefs.getInt("s5", 4);
  data.s_upSpeed       = prefs.getInt("s6", 60);
  data.s_downSpeed     = prefs.getInt("s7", 50);
  data.s_colLimit      = prefs.getInt("s8", -50); 
  data.s_softLimit     = prefs.getInt("s9", 500);
  data.s_weightDwell   = prefs.getInt("s10", 500);
  data.s_coolTime      = prefs.getInt("s11", 15);
  data.s_tmcMode       = prefs.getInt("s12", 1);  // Default: SpreadCycle (1)
  data.s_tmcThreshold  = prefs.getInt("s13", 30);
  data.s_jigLength     = prefs.getInt("s14", 240);
  data.s_baseMeltTime  = prefs.getInt("s15", 5);
  data.s_jigCleanTime  = prefs.getInt("s16", 30);
  data.s_coolTime1     = prefs.getInt("s17", 0);
  data.s_coolTimeEnd   = prefs.getInt("s18", 0);
  data.s_immerseSpeed  = prefs.getInt("s19", 25);
  data.s_extractSpeed  = prefs.getInt("s20", 20);
  data.s_dripOffset    = prefs.getInt("s21", 15);
  data.s_dripTime      = prefs.getInt("s22", 3);
  data.meltBasesEnabled = prefs.getBool("mb_en", false);
  data.lastWaxLevelMM  = prefs.getFloat("wax_lvl", 0.0f);
  data.s_brightness    = prefs.getInt("bright", 50);
  data.s_theme         = prefs.getInt("theme", 1);
  data.s_volume        = prefs.getInt("vol", 70);
  data.s_startupTune   = prefs.getInt("st_tune", 1);
  data.s_finishTune    = prefs.getInt("fn_tune", 1);
  data.s_alertTune     = prefs.getInt("al_tune", 1);
  data.wifiSSID        = prefs.getString("wssid", "");
  String wifiPass      = prefs.getString("wpass", "");

  // Load 5-entry rotating timing history for the 4 process types (Slim Wt, Std Wt, Slim Dips, Std Dips)
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

  // Initialize Hardware Subsystems
  sensorMgr.begin();
  scaleMgr.begin(&prefs);
  data.scaleFactor = scaleMgr.getScaleFactor();
  motorMgr.begin(&sensorMgr);
  motorMgr.applyConfigMode(data.s_tmcMode, (float)data.s_downSpeed, data.s_tmcThreshold);
  soundMgr.begin(data.s_volume);

  // Initialize double-buffered DisplayManager
  if (!display.begin(&M5.Display)) {
    Serial.println("Display Manager Initialization Failed!");
  }

  // Boot Splash Screen ("BEE MINE DORSET") + Startup Tune
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(M5.Display.color565(255, 235, 130), TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextSize(3.0);
  M5.Display.drawString("BEE MINE", 160, 100);
  M5.Display.setTextSize(2.0);
  M5.Display.drawString("DORSET", 160, 140);

  // Start playing the configured Startup Tune non-blockingly while Wi-Fi connects / splash shows
  soundMgr.playStartupTune(data.s_startupTune, false);

  // Attempt Wi-Fi Connection (up to 3 seconds timeout) if SSID is configured so boot isn't blocked if offline
  if (data.wifiSSID.length() > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(data.wifiSSID.c_str(), wifiPass.c_str());
    unsigned long startWifi = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startWifi < 3000) {
      soundMgr.update();
      delay(20);
    }

    // If connected to Wi-Fi, synchronize the hardware RTC clock via NTP
    if (WiFi.status() == WL_CONNECTED) {
      syncNtpTime();
    }
  }

  unsigned long splashWait = millis();
  while (millis() - splashWait < 1000) {
    soundMgr.update();
    delay(20);
  }
  display.markPageChanged(true);
}

// Saves the 5-entry circular history buffers for all 4 dipping profiles back to NVS flash
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

// States of an automated candle dipping cycle
enum DipPhase {
  DIP_PHASE_IDLE = 0,
  DIP_PHASE_LOWERING,      // Moving carriage down until capacitive wax sensor triggers (monitors collision & low-wax limit)
  DIP_PHASE_HOLDING,       // Dwelling submerged in molten wax (s_dip1Time for dip 1, s_subDipTime for dips 2+)
  DIP_PHASE_RAISING,       // Extracting slowly from wax, optional Drip-Off Pause, fast raise to top switch, backoff, and weighing
  DIP_PHASE_COOLING,       // Waiting at the top position (tapered cool curve) for wax layer to solidify before next dip
  DIP_PHASE_FINAL_COOLING, // Cooling final coat at top before optional Base Melt finishing stage
  DIP_PHASE_MELTING_BASE   // Lowering candle bottoms (lastWaxLevelMM - s_jigLength) to melt & level candle bases
};

static DipPhase currentDipPhase = DIP_PHASE_IDLE;
static unsigned long phaseStartTime = 0;
static unsigned long holdDurationMs = 0;

// Computes the cooling rest time (in seconds) for the current dip using a linear curve
// between s_coolTime1 (1st dip) and s_coolTimeEnd (last dip). If either is 0, s_coolTime is used.
int computeCurrentCoolTimeSec(const DisplayData& data, bool isFinalCool = false) {
  int startCool = (data.s_coolTime1 > 0) ? data.s_coolTime1 : data.s_coolTime;
  int endCool   = (data.s_coolTimeEnd > 0) ? data.s_coolTimeEnd : data.s_coolTime;
  if (isFinalCool) return endCool;
  if (startCool == endCool) return startCool;

  float progress = 0.0f;
  if (data.isActiveWeightBased) {
    float targetWt = (float)max(1, data.isActiveSlimProfile ? data.s_slimWt : data.s_stdWt);
    progress = constrain(data.currentWeight / targetWt, 0.0f, 1.0f);
  } else {
    int totalDips = max(1, data.isActiveSlimProfile ? data.s_slimDips : data.s_stdDips);
    if (totalDips <= 1) progress = 1.0f;
    else progress = constrain((float)(data.currentDipCount - 1) / (float)(totalDips - 1), 0.0f, 1.0f);
  }
  return (int)roundf((float)startCool + progress * (float)(endCool - startCool));
}

// Helper to complete the automated dipping process and play completion melody
void finishDippingWithMelody() {
  DisplayData& data = display.getData();
  currentDipPhase = DIP_PHASE_IDLE;
  int finalElapsedSec = (int)((millis() - data.dipStartTime) / 1000);
  display.endDippingProcess(false, finalElapsedSec);
  saveProcessHistoryToPrefs();
  soundMgr.playFinishTune(data.s_finishTune, false);
}

// Executes the automated dipping state machine when on PAGE_ACTIVE_DIP
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

  // If on confirmation modal, ensure the carriage is safely raised to top rest (never stuck in molten wax!)
  if (data.currentPage == PAGE_STOP_CONFIRM) {
    if (data.currentPosition > 0.5f || !sensorMgr.readRawTopLimit()) {
      motorMgr.retractToTopRest((float)data.s_upSpeed);
      data.currentPosition = 0.0f;
      if (currentDipPhase == DIP_PHASE_HOLDING || currentDipPhase == DIP_PHASE_RAISING) {
        currentDipPhase = DIP_PHASE_LOWERING;
      }
      display.markPageChanged(true);
    } else {
      motorMgr.stopMotor();
    }
    return;
  }

  // Initialize active dipping phase if entering from IDLE
  if (currentDipPhase == DIP_PHASE_IDLE) {
    currentDipPhase = DIP_PHASE_LOWERING;
    data.currentPhaseText = "Dip " + String(data.currentDipCount) + ": Lowering...";
    display.markPageChanged(true);
  }

  char pTxt[64];

  switch (currentDipPhase) {
    // PHASE 1: Lower carriage into wax pot until capacitive sensor detects wax surface
    // Uses fast Down Speed (s_downSpeed) in open air, then slows to Immerse Speed (s_immerseSpeed)
    // 15mm before the bottom of the candle jig touches the wax surface (lastWaxLevelMM - s_jigLength).
    case DIP_PHASE_LOWERING: {
      float fastDownSpeed = (data.s_downSpeed > 0) ? (float)data.s_downSpeed : 50.0f;
      float immerseSpeed  = (data.s_immerseSpeed > 0) ? (float)data.s_immerseSpeed : fastDownSpeed;

      // Z position where the bottom of the candles touches the wax surface
      bool waxKnown = (data.lastWaxLevelMM > (float)data.s_jigLength);
      float baseTouchMM = waxKnown ? (data.lastWaxLevelMM - (float)data.s_jigLength) : 0.0f;
      // Start slowing down 15mm before the candle base touches the wax
      float slowStartMM = waxKnown ? max(0.0f, baseTouchMM - 15.0f) : 0.0f;

      bool isImmersingSlow = (data.currentPosition >= slowStartMM);
      float activeSpeed = isImmersingSlow ? immerseSpeed : fastDownSpeed;
      motorMgr.applyConfigMode(data.s_tmcMode, activeSpeed, data.s_tmcThreshold);

      digitalWrite(PIN_MOTOR_DIR, LOW); // DOWN direction
      unsigned long delayUs = (unsigned long)(1000000.0f / (activeSpeed * MotorManager::STEPS_PER_MM));
      if (delayUs < 20) delayUs = 20;

      unsigned long startTimeout = millis();
      float stepDistMM = 1.0f / MotorManager::STEPS_PER_MM;

      // Capture baseline weight at the start of this downward stroke so collision detection works
      // accurately on any dip (even when candles already weigh +300g..+2500g from previous dips)
      float baselineWeight = scaleMgr.getWeightGrams();
      int collisionHits = 0;

      // Step downward until wax sensor triggers, soft limit is reached, collision occurs, or user touches screen
      while (!sensorMgr.readRawCapSensor()) {
        if (checkManualStop()) {
          // Immediately retract carriage to top rest position so candles are never left hanging near/in wax
          motorMgr.retractToTopRest((float)data.s_upSpeed);
          data.currentPosition = 0.0f;
          return;
        }

        // Switch from fast open-air Down Speed to slower Immerse Speed just before candle base touches wax
        if (!isImmersingSlow && data.currentPosition >= slowStartMM) {
          isImmersingSlow = true;
          activeSpeed = immerseSpeed;
          motorMgr.applyConfigMode(data.s_tmcMode, activeSpeed, data.s_tmcThreshold);
          delayUs = (unsigned long)(1000000.0f / (activeSpeed * MotorManager::STEPS_PER_MM));
          if (delayUs < 20) delayUs = 20;
          if (immerseSpeed != fastDownSpeed) {
            snprintf(pTxt, sizeof(pTxt), "Dip %d: Immersing...", data.currentDipCount);
            data.currentPhaseText = String(pTxt);
            display.markPageChanged(true);
            display.renderCurrentPage();
          }
        }

        // Poll load cell non-blockingly (10Hz) to check for upward collision against unmelted/solid wax
        if (scaleMgr.update()) {
          data.currentWeight = scaleMgr.getWeightGrams();
          if (data.s_colLimit < 0) {
            float weightDelta = data.currentWeight - baselineWeight;
            if (weightDelta <= (float)data.s_colLimit) {
              collisionHits++;
              // Require 2 consecutive 10Hz samples below threshold to reject stepper start vibration
              // and thick liquid wax buoyancy spikes
              if (collisionHits >= 2) {
                Serial.printf("Wax Collision Detected! Delta: %.1fg <= Limit: %dg. Retracting to top rest!\n", weightDelta, data.s_colLimit);
                motorMgr.stopMotor();
                soundMgr.playBeep(400, 250);
                data.currentPhaseText = "Collision! Raising to top...";
                display.markPageChanged(true);
                display.renderCurrentPage();

                // Retract carriage all the way up to top rest position (0.0mm) so jig isn't stuck in wax
                motorMgr.retractToTopRest((float)data.s_upSpeed);
                data.currentPosition = 0.0f;

                data.collisionTriggered = true;
                data.lowWaxTriggered = false;
                data.currentPage = PAGE_STOP_CONFIRM;
                data.stopConfirmEnterTime = millis();
                soundMgr.playAlertTune(data.s_alertTune, false);
                display.markPageChanged(true);
                return;
              }
            } else {
              collisionHits = 0;
            }
          }
        }

        // Low Wax Depletion / Soft Limit Cutoff: if carriage reaches s_softLimit without capacitive sensor triggering,
        // the wax level has dropped below the reach of the carriage -> retract to top rest and alert user!
        if (data.s_softLimit > 0 && data.currentPosition >= (float)data.s_softLimit) {
          Serial.println("Low Wax Alert: Soft Limit reached without sensing wax! Retracting to top rest.");
          motorMgr.stopMotor();
          soundMgr.playBeep(350, 250);
          data.currentPhaseText = "Low Wax! Raising to top...";
          display.markPageChanged(true);
          display.renderCurrentPage();

          motorMgr.retractToTopRest((float)data.s_upSpeed);
          data.currentPosition = 0.0f;

          data.lowWaxTriggered = true;
          data.collisionTriggered = false;
          data.currentPage = PAGE_STOP_CONFIRM;
          data.stopConfirmEnterTime = millis();
          soundMgr.playAlertTune(data.s_alertTune, false);
          display.markPageChanged(true);
          return;
        }

        digitalWrite(PIN_MOTOR_STEP, HIGH);
        delayMicroseconds(3);
        digitalWrite(PIN_MOTOR_STEP, LOW);

        data.currentPosition += stepDistMM;
        motorMgr.setCurrentPositionMM(data.currentPosition);

        delayMicroseconds(delayUs);

        if (millis() - startTimeout > 35000) break; // 35-second safety timeout guard
      }

      motorMgr.stopMotor();

      // Record the Z-position of the wax surface whenever the capacitive sensor triggers
      if (sensorMgr.readRawCapSensor()) {
        data.lastWaxLevelMM = data.currentPosition;
        prefs.putFloat("wax_lvl", data.lastWaxLevelMM);
      }

      currentDipPhase = DIP_PHASE_HOLDING;
      phaseStartTime = millis();
      // First dip uses s_dip1Time (longer soak for dry wicks); subsequent dips use s_subDipTime
      int holdSec = (data.currentDipCount == 1) ? data.s_dip1Time : data.s_subDipTime;
      holdDurationMs = (unsigned long)holdSec * 1000;

      snprintf(pTxt, sizeof(pTxt), "Dip %d: Holding (%ds)", data.currentDipCount, holdSec);
      data.currentPhaseText = String(pTxt);
      display.markPageChanged(true);
      break;
    }

    // PHASE 2: Hold stationary in the wax for the configured dwell duration (non-blocking countdown)
    case DIP_PHASE_HOLDING:
      motorMgr.stopMotor();
      {
        unsigned long elapsed = millis() - phaseStartTime;
        if (elapsed >= holdDurationMs) {
          currentDipPhase = DIP_PHASE_RAISING;
          snprintf(pTxt, sizeof(pTxt), "Dip %d: Extracting...", data.currentDipCount);
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

    // PHASE 3: Extract slowly from wax (s_extractSpeed), optional Drip-Off Pause (s_dripOffset / s_dripTime),
    // accelerate to Up Speed (s_upSpeed) once candle base is out of wax, relieve top switch, and weigh
    case DIP_PHASE_RAISING: {
      display.renderCurrentPage();

      float fastUpSpeed  = (data.s_upSpeed > 0) ? (float)data.s_upSpeed : 60.0f;
      float extractSpeed = (data.s_extractSpeed > 0) ? (float)data.s_extractSpeed : fastUpSpeed;

      // Z position where the bottom of the candle jig clears the wax surface
      bool waxKnown = (data.lastWaxLevelMM > (float)data.s_jigLength);
      float baseClearMM = waxKnown ? max(0.0f, data.lastWaxLevelMM - (float)data.s_jigLength) : 0.0f;

      // Drip-Off Pause position relative to wax surface (+s_dripOffset = above wax, -s_dripOffset = below wax)
      bool dripEnabled = (waxKnown && data.s_dripOffset != 0 && data.s_dripTime > 0);
      float dripPauseMM = dripEnabled ? constrain(baseClearMM - (float)data.s_dripOffset, 0.0f, data.lastWaxLevelMM) : -1.0f;
      bool dripPauseDone = !dripEnabled;

      bool isExtractingSlow = (waxKnown && data.currentPosition > baseClearMM);
      float activeSpeed = isExtractingSlow ? extractSpeed : fastUpSpeed;
      motorMgr.applyConfigMode(data.s_tmcMode, activeSpeed, data.s_tmcThreshold);

      digitalWrite(PIN_MOTOR_DIR, HIGH); // UP direction
      unsigned long delayUs = (unsigned long)(1000000.0f / (activeSpeed * MotorManager::STEPS_PER_MM));
      if (delayUs < 20) delayUs = 20;

      unsigned long startTimeout = millis();
      float stepDistMM = 1.0f / MotorManager::STEPS_PER_MM;

      // Step upward until top mechanical limit switch is hit or user touches screen
      while (!sensorMgr.readRawTopLimit()) {
        if (checkManualStop()) {
          // Finish raising to top rest before entering pause confirmation
          motorMgr.retractToTopRest((float)data.s_upSpeed);
          data.currentPosition = 0.0f;
          currentDipPhase = DIP_PHASE_LOWERING;
          return;
        }

        // Check if carriage has reached the Drip-Off Pause position
        if (!dripPauseDone && data.currentPosition <= dripPauseMM) {
          dripPauseDone = true;
          motorMgr.stopMotor();

          unsigned long dripStart = millis();
          unsigned long dripDurMs = (unsigned long)data.s_dripTime * 1000UL;
          while (millis() - dripStart < dripDurMs) {
            if (checkManualStop()) {
              motorMgr.retractToTopRest((float)data.s_upSpeed);
              data.currentPosition = 0.0f;
              currentDipPhase = DIP_PHASE_LOWERING;
              return;
            }
            int remDrip = (int)((dripDurMs - (millis() - dripStart) + 999) / 1000);
            snprintf(pTxt, sizeof(pTxt), "Dip %d: Drip Pause (%ds)", data.currentDipCount, remDrip);
            if (data.currentPhaseText != String(pTxt)) {
              data.currentPhaseText = String(pTxt);
              display.markPageChanged(true);
              display.renderCurrentPage();
            }
            delay(10);
          }

          // Resume upward motion after drip pause
          digitalWrite(PIN_MOTOR_DIR, HIGH);
          startTimeout = millis();
          snprintf(pTxt, sizeof(pTxt), "Dip %d: Raising...", data.currentDipCount);
          data.currentPhaseText = String(pTxt);
          display.markPageChanged(true);
          display.renderCurrentPage();
        }

        // Once the bottom of the candle jig is clear of the wax (and drip pause is done), speed up to Up Speed
        if (isExtractingSlow && data.currentPosition <= baseClearMM && dripPauseDone) {
          isExtractingSlow = false;
          activeSpeed = fastUpSpeed;
          motorMgr.applyConfigMode(data.s_tmcMode, activeSpeed, data.s_tmcThreshold);
          delayUs = (unsigned long)(1000000.0f / (activeSpeed * MotorManager::STEPS_PER_MM));
          if (delayUs < 20) delayUs = 20;
          snprintf(pTxt, sizeof(pTxt), "Dip %d: Raising...", data.currentDipCount);
          if (data.currentPhaseText != String(pTxt)) {
            data.currentPhaseText = String(pTxt);
            display.markPageChanged(true);
            display.renderCurrentPage();
          }
        }

        digitalWrite(PIN_MOTOR_STEP, HIGH);
        delayMicroseconds(3);
        digitalWrite(PIN_MOTOR_STEP, LOW);

        data.currentPosition -= stepDistMM;
        if (data.currentPosition < 0.0f) data.currentPosition = 0.0f;
        motorMgr.setCurrentPositionMM(data.currentPosition);

        delayMicroseconds(delayUs);

        if (millis() - startTimeout > 35000) break; // 35-second safety timeout guard
      }

      motorMgr.stopMotor();

      // Back off 2.0mm from the top limit switch so mechanical switch spring pressure does not skew the load cell
      motorMgr.backoffFromTopLimit(2.0f);
      data.currentPosition = 0.0f;

      // Dwell time before scale reading so carriage vibration and dripping wax settle
      if (data.s_weightDwell > 0) {
        delay(data.s_weightDwell);
      }

      // Take 4 settled HX711 readings directly so EMA lag does not under-report the new dip's weight
      data.currentWeight = scaleMgr.readSettledWeight(4);

      // Check if dipping process finished (by target weight or target dip count)
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
        // Check if optional "Melt Candle Bases" final stage is enabled and wax level is known
        if (data.meltBasesEnabled && data.s_baseMeltTime > 0 && data.lastWaxLevelMM > (float)data.s_jigLength) {
          int finalCoolSec = computeCurrentCoolTimeSec(data, true);
          currentDipPhase = DIP_PHASE_FINAL_COOLING;
          phaseStartTime = millis();
          holdDurationMs = (unsigned long)finalCoolSec * 1000;
          snprintf(pTxt, sizeof(pTxt), "Pre-Melt Cool (%ds)", finalCoolSec);
          data.currentPhaseText = String(pTxt);
          display.markPageChanged(true);
        } else {
          finishDippingWithMelody();
        }
      } else {
        // More dips needed -> enter cooling phase at top of stroke using tapered cooling curve
        int coolSec = computeCurrentCoolTimeSec(data, false);
        currentDipPhase = DIP_PHASE_COOLING;
        phaseStartTime = millis();
        holdDurationMs = (unsigned long)coolSec * 1000;
        snprintf(pTxt, sizeof(pTxt), "Dip %d: Cooling (%ds)", data.currentDipCount, coolSec);
        data.currentPhaseText = String(pTxt);
        display.markPageChanged(true);
      }
      break;
    }

    // PHASE 4: Cool at top of stroke before starting the next dip cycle
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

    // PHASE 5: Cool final coat at top before lowering candle bases to melt & level bottoms
    case DIP_PHASE_FINAL_COOLING:
      motorMgr.stopMotor();
      {
        unsigned long elapsed = millis() - phaseStartTime;
        if (elapsed >= holdDurationMs) {
          currentDipPhase = DIP_PHASE_MELTING_BASE;
          snprintf(pTxt, sizeof(pTxt), "Melting Bases (%ds)...", data.s_baseMeltTime);
          data.currentPhaseText = String(pTxt);
          display.markPageChanged(true);
        } else {
          int remSec = (int)((holdDurationMs - elapsed + 999) / 1000);
          snprintf(pTxt, sizeof(pTxt), "Pre-Melt Cool (%ds)", remSec);
          if (data.currentPhaseText != String(pTxt)) {
            data.currentPhaseText = String(pTxt);
            display.markPageChanged(true);
          }
        }
      }
      break;

    // PHASE 6: Lower candle bases to (lastWaxLevelMM - s_jigLength), hold for s_baseMeltTime, and retract to top rest
    case DIP_PHASE_MELTING_BASE: {
      float targetBaseMM = data.lastWaxLevelMM - (float)data.s_jigLength;
      if (targetBaseMM <= 0.0f) {
        finishDippingWithMelody();
        break;
      }
      display.renderCurrentPage();
      markActionStarted();
      motorMgr.performMeltBases(targetBaseMM, (float)data.s_downSpeed, (float)data.s_upSpeed, data.s_baseMeltTime, checkManualStop);
      data.currentPosition = motorMgr.getCurrentPositionMM();

      // If user did not pause/stop during base melt, complete the dipping process
      if (data.currentPage == PAGE_ACTIVE_DIP) {
        finishDippingWithMelody();
      }
      break;
    }

    default:
      break;
  }
}

// Main non-blocking firmware loop
void loop() {
  M5.update();
  soundMgr.update();
  DisplayData& data = display.getData();

  // Update hardware sensors (100Hz debounce) & scale polling (10Hz EMA)
  sensorMgr.update();
  scaleMgr.update();

  // Sync current soft limit setting to motor manager
  motorMgr.setMaxSoftLimitMM((float)data.s_softLimit);

  // Sync current position, load cell weight, calibration factor, and sensor flags into DisplayData container
  data.currentPosition = motorMgr.getCurrentPositionMM();
  data.currentWeight = scaleMgr.getWeightGrams();
  data.scaleFactor = scaleMgr.getScaleFactor();
  data.limitSwitchOn = sensorMgr.isTopLimitHit();
  data.capSensorOn = sensorMgr.isCapSensorTriggered();

  // Handle captive portal DNS & HTTP requests when Wi-Fi setup portal is active
  if (data.portalActive) {
    dnsServer.processNextRequest();
    webServer.handleClient();
  }

  // Live screen refresh & manual jogging on Manual Control Page
  if (data.currentPage == PAGE_MANUAL) {
    if (data.isUpPressed) {
      // Stop upward jog if top limit switch is hit or position is already at 0.0mm
      if (data.limitSwitchOn || data.currentPosition <= 0.0f) {
        data.isUpPressed = false;
        motorMgr.stopMotor();
        display.markPageChanged(true);
      } else {
        motorMgr.applyConfigMode(data.s_tmcMode, (float)data.manualSpeed, data.s_tmcThreshold);
        motorMgr.stepMotorBurst(true, (float)data.manualSpeed, 50);
        data.currentPosition = motorMgr.getCurrentPositionMM();
        static unsigned long lastPosUpdate = 0;
        if (millis() - lastPosUpdate > 250) {
          lastPosUpdate = millis();
          display.markPageChanged(true);
        }
      }
    } else if (data.isDownPressed) {
      // Stop downward jog if wax capacitive sensor triggers or soft limit is reached
      if (data.capSensorOn || (data.s_softLimit > 0 && data.currentPosition >= (float)data.s_softLimit)) {
        data.isDownPressed = false;
        motorMgr.stopMotor();
        if (data.capSensorOn && data.currentPosition > 0.0f) {
          data.lastWaxLevelMM = data.currentPosition;
          prefs.putFloat("wax_lvl", data.lastWaxLevelMM);
        }
        display.markPageChanged(true);
      } else {
        motorMgr.applyConfigMode(data.s_tmcMode, (float)data.manualSpeed, data.s_tmcThreshold);
        motorMgr.stepMotorBurst(false, (float)data.manualSpeed, 50);
        data.currentPosition = motorMgr.getCurrentPositionMM();
        static unsigned long lastPosUpdate = 0;
        if (millis() - lastPosUpdate > 250) {
          lastPosUpdate = millis();
          display.markPageChanged(true);
        }
      }
    } else if (!data.isHomingActive && !data.isDipBotActive) {
      // When idle on Manual page, refresh telemetry at 10Hz if weight, position, or sensor states change
      motorMgr.stopMotor();
      static unsigned long lastScaleRefresh = 0;
      static bool lastLimitDisplayed = false;
      static bool lastCapDisplayed = false;
      if (millis() - lastScaleRefresh >= 100) {
        lastScaleRefresh = millis();
        if (abs(data.currentWeight - lastWtDisplayed) >= 0.2f || 
            abs(data.currentPosition - lastPosDisplayed) >= 0.1f ||
            data.limitSwitchOn != lastLimitDisplayed ||
            data.capSensorOn != lastCapDisplayed) {
          lastWtDisplayed = data.currentWeight;
          lastPosDisplayed = data.currentPosition;
          lastLimitDisplayed = data.limitSwitchOn;
          lastCapDisplayed = data.capSensorOn;
          display.markPageChanged(true);
        }
      }
    }
  } else if (data.currentPage == PAGE_SCALE_CAL && !data.showNumpad) {
    // Refresh live load cell readout at 4Hz on the Scale Calibration Wizard page
    static unsigned long lastCalRefresh = 0;
    if (millis() - lastCalRefresh >= 250) {
      lastCalRefresh = millis();
      if (abs(data.currentWeight - lastWtDisplayed) >= 0.1f) {
        lastWtDisplayed = data.currentWeight;
        display.markPageChanged(true);
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

  // Periodic RTC clock update on top banner (redraws once per minute when minute rolls over)
  m5::rtc_datetime_t dt;
  if (M5.Rtc.getDateTime(&dt)) {
    if (dt.time.minutes != lastMinute) {
       lastMinute = dt.time.minutes;
       if (data.currentPage < PAGE_ACTIVE_DIP && !data.showNumpad) {
         display.markPageChanged(true);
       }
    }
  }

  // Periodic 1Hz ticker during active dipping to keep the elapsed time counter ticking on screen
  if (data.currentPage == PAGE_ACTIVE_DIP && (millis() - data.lastUiTick >= 1000)) {
    data.lastUiTick = millis(); 
    display.markPageChanged(true); 
  }

  // 1. Process Touch Events & User Interactions
  UiEvent event = display.updateTouch();

  // 2. Handle Event Callbacks from UI
  switch (event) {
    case UI_EVENT_MANUAL_HOME:
      Serial.println("Executing Homing Sequence...");
      markActionStarted();
      data.isHomingActive = true;
      display.markPageChanged(true);
      display.renderCurrentPage(); // Render "STOP" button before entering blocking homing routine

      motorMgr.performHoming((float)data.s_upSpeed, checkManualStop);

      data.isHomingActive = false;
      display.markPageChanged(true);
      break;

    case UI_EVENT_MANUAL_DIP_BOT:
      Serial.println("Executing Dip Bot Sequence...");
      markActionStarted();
      data.isDipBotActive = true;
      display.markPageChanged(true);
      display.renderCurrentPage(); // Render "STOP" button before entering blocking Dip Bot routine

      motorMgr.performDipBot((float)data.s_downSpeed, (float)data.s_upSpeed, data.s_subDipTime, &data.lastWaxLevelMM, checkManualStop);
      if (data.lastWaxLevelMM > 0.0f) {
        prefs.putFloat("wax_lvl", data.lastWaxLevelMM);
      }

      data.isDipBotActive = false;
      display.markPageChanged(true);
      break;

    case UI_EVENT_TOOL_MELT_BASES: {
      float targetDepth = data.lastWaxLevelMM - (float)data.s_jigLength;
      if (data.lastWaxLevelMM <= 0.0f || targetDepth <= 0.0f) {
        Serial.println("Cannot Melt Bases: Wax level not sensed yet or <= Jig Length!");
        soundMgr.playAlertTune(data.s_alertTune, false);
        display.markPageChanged(true);
        break;
      }
      Serial.printf("Executing Melt Candle Bases at Z=%.1fmm for %ds...\n", targetDepth, data.s_baseMeltTime);
      markActionStarted();
      data.isMeltBasesActive = true;
      display.markPageChanged(true);
      display.renderCurrentPage();

      motorMgr.performMeltBases(targetDepth, (float)data.s_downSpeed, (float)data.s_upSpeed, data.s_baseMeltTime, checkManualStop);

      data.isMeltBasesActive = false;
      display.markPageChanged(true);
      break;
    }

    case UI_EVENT_TOOL_CLEAN_JIG:
      Serial.printf("Executing Clean Whole Jig (full sink to sensor) for %ds...\n", data.s_jigCleanTime);
      markActionStarted();
      data.isCleanJigActive = true;
      display.markPageChanged(true);
      display.renderCurrentPage();

      motorMgr.performCleanJig((float)data.s_downSpeed, (float)data.s_upSpeed, data.s_jigCleanTime, &data.lastWaxLevelMM, checkManualStop);
      if (data.lastWaxLevelMM > 0.0f) {
        prefs.putFloat("wax_lvl", data.lastWaxLevelMM);
      }

      data.isCleanJigActive = false;
      display.markPageChanged(true);
      break;

    case UI_EVENT_MELT_TOGGLED:
      prefs.putBool("mb_en", data.meltBasesEnabled);
      soundMgr.playBeep(data.meltBasesEnabled ? 1400 : 900, 80);
      break;

    case UI_EVENT_MANUAL_STOP:
      Serial.println("UI Event: STOP requested!");
      motorMgr.stopMotor();
      data.isHomingActive = false;
      data.isDipBotActive = false;
      data.isMeltBasesActive = false;
      data.isCleanJigActive = false;
      currentDipPhase = DIP_PHASE_IDLE;
      display.markPageChanged(true);
      break;

    case UI_EVENT_MANUAL_TARE:
      Serial.println("Taring Scale in RAM (10 fresh HX711 samples)...");
      scaleMgr.tare(10);
      data.currentWeight = scaleMgr.getWeightGrams();
      data.calStatusText = "Zeroed (0.0g)! Hang known weight & tap 2";
      soundMgr.playBeep(1200, 150);
      display.markPageChanged(true);
      break;

    case UI_EVENT_CALIBRATE_SCALE:
      Serial.printf("Calibrating Scale with %dg reference weight...\n", data.calibrationRefGrams);
      scaleMgr.calibrate((float)data.calibrationRefGrams, 10);
      data.scaleFactor = scaleMgr.getScaleFactor();
      data.currentWeight = scaleMgr.getWeightGrams();
      data.calStatusText = "Saved! Calibrated to " + String(data.calibrationRefGrams) + "g";
      soundMgr.playBeep(1500, 250);
      display.markPageChanged(true);
      break;

    case UI_EVENT_START_WIFI_PORTAL:
      startWebPortal();
      break;

    case UI_EVENT_STOP_WIFI_PORTAL:
      stopWebPortal();
      break;

    case UI_EVENT_SETTING_UPDATED:
      // Persist modified parameter (s0..s22) to ESP32 NVS Preferences
      if (data.activeSetting >= 0 && data.activeSetting <= 22) {
        char keyStr[8];
        snprintf(keyStr, sizeof(keyStr), "s%d", data.activeSetting);
        prefs.putInt(keyStr, display.getSettingValue(data.activeSetting));
      }
      motorMgr.applyConfigMode(data.s_tmcMode, (float)data.s_downSpeed, data.s_tmcThreshold);
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

    case UI_EVENT_VOLUME_CHANGED:
      prefs.putInt("vol", data.s_volume);
      soundMgr.setVolume(data.s_volume);
      if (data.s_volume > 0) {
        soundMgr.playBeep(1047, 60); // Short C6 feedback chirp at the newly selected volume
      }
      break;

    case UI_EVENT_STARTUP_TUNE_CHANGED:
      prefs.putInt("st_tune", data.s_startupTune);
      soundMgr.playStartupTune(data.s_startupTune, false); // Live preview when selected!
      break;

    case UI_EVENT_FINISH_TUNE_CHANGED:
      prefs.putInt("fn_tune", data.s_finishTune);
      soundMgr.playFinishTune(data.s_finishTune, false);   // Live preview when selected!
      break;

    case UI_EVENT_ALERT_TUNE_CHANGED:
      prefs.putInt("al_tune", data.s_alertTune);
      soundMgr.playAlertTune(data.s_alertTune, false);     // Live preview when selected!
      break;

    case UI_EVENT_START_DIP:
      Serial.println("UI Event: Starting dipping process (RAM auto-tare)...");
      motorMgr.stopMotor();
      scaleMgr.tare(10); // Auto-tare scale in RAM (10 fresh conversions) so current jig weight is zeroed out before Dip 1
      data.currentWeight = 0.0f;
      phaseStartTime = millis();
      currentDipPhase = DIP_PHASE_LOWERING;
      display.markPageChanged(true);
      soundMgr.playBeep(1800, 200);
      break;

    case UI_EVENT_ABORT_DIP:
      Serial.println("UI Event: Aborting dipping process...");
      currentDipPhase = DIP_PHASE_IDLE;
      motorMgr.stopMotor();
      soundMgr.playAlertTune(data.s_alertTune, false);
      break;

    case UI_EVENT_FINISH_DIP:
      Serial.println("UI Event: Dipping process finished");
      currentDipPhase = DIP_PHASE_IDLE;
      motorMgr.stopMotor();
      soundMgr.playFinishTune(data.s_finishTune, false);
      break;

    default:
      break;
  }

  // 3. Render Double-Buffered Screen Canvas (no-op if data.pageChanged is false)
  display.renderCurrentPage();
}

