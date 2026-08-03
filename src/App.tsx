import React, { useState, useEffect, useRef } from 'react';
import { 
  Code2, 
  Layers, 
  Cpu, 
  CheckCircle2, 
  Copy, 
  Check, 
  Eye, 
  Sparkles, 
  Maximize2,
  Sliders,
  RotateCcw,
  Palette,
  Wifi,
  Scale,
  Settings
} from 'lucide-react';

const DISPLAY_MANAGER_H_CODE = `#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <M5Unified.h>

// --- Application States ---
enum AppState {
  PAGE_WEIGHT_DIP,
  PAGE_DIPS_DIP,
  PAGE_MANUAL,
  PAGE_SETTINGS_HUB,  
  PAGE_SETTING_DIPPING,
  PAGE_SETTING_MOTION,
  PAGE_SETTING_SENSOR,
  PAGE_SETTING_SCREEN,
  PAGE_WIFI_PORTAL,
  PAGE_ACTIVE_DIP,
  PAGE_STOP_CONFIRM,
  PAGE_DIP_DONE
};

enum SubState {
  SUB_MAIN,
  SUB_CONFIRM_SLIM,
  SUB_CONFIRM_STD
};

// --- Action Events Triggered by UI Touch Interactions ---
enum UiEvent {
  UI_EVENT_NONE,
  UI_EVENT_START_DIP,
  UI_EVENT_ABORT_DIP,
  UI_EVENT_FINISH_DIP,
  UI_EVENT_START_WIFI_PORTAL,
  UI_EVENT_STOP_WIFI_PORTAL,
  UI_EVENT_SETTING_UPDATED,
  UI_EVENT_THEME_CHANGED,
  UI_EVENT_BRIGHTNESS_CHANGED,
  UI_EVENT_MANUAL_TARE
};

// --- Display Data Container ---
struct DisplayData {
  // Navigation & Page State
  AppState currentPage = PAGE_WEIGHT_DIP;
  SubState currentSubState = SUB_MAIN;
  bool pageChanged = true;

  // Manual Control State
  bool limitSwitchOn = false;
  bool capSensorOn = false;
  bool isUpPressed = false;     
  bool isDownPressed = false;   
  int manualSpeed = 50;
  float currentPosition = 0.0;
  float currentWeight = 0.0;

  // Settings Configuration Values
  int s_slimWt = 1500;
  int s_stdWt = 2500;
  int s_slimDips = 15;
  int s_stdDips = 25;
  int s_dip1Time = 10;
  int s_subDipTime = 4;
  int s_downSpeed = 50;
  int s_upSpeed = 60;
  int s_colLimit = -50;
  int s_brightness = 50; // 1 to 99%
  int s_theme = 1;       // 0: Original, 1: Bee Mine, 2: Dark Mode

  // Numpad Editor State
  bool showNumpad = false;
  int activeSetting = -1;
  String numpadStr = "";

  // Active Dipping State
  unsigned long dipStartTime = 0;
  unsigned long lastUiTick = 0;
  int estimatedTotalSeconds = 0;
  bool isActiveWeightBased = true;
  bool isActiveSlimProfile = true;
  bool dipWasAborted = false;
  int currentDipCount = 0;
  String currentPhaseText = "Starting...";

  // Network & Portal State
  String wifiSSID = "Trooli_BB00";
  bool portalActive = false;
};

class DisplayManager {
public:
  DisplayManager();

  // Initialization & Setup
  bool begin(M5GFX* display = &M5.Display);

  // Theme & Screen Control
  void applyTheme(int themeId);
  void setBrightness(int percent);

  // Core Render & Touch Execution
  void renderCurrentPage();
  UiEvent updateTouch();

  // Process Controls
  void startDippingProcess(bool isWeight, bool isSlim);
  void endDippingProcess(bool aborted);

  // Data Accessors & Mutators
  DisplayData& getData() { return data; }
  const DisplayData& getData() const { return data; }

  AppState getCurrentPage() const { return data.currentPage; }
  void setCurrentPage(AppState page) { data.currentPage = page; data.pageChanged = true; }

  SubState getSubState() const { return data.currentSubState; }
  void setSubState(SubState sub) { data.currentSubState = sub; data.pageChanged = true; }

  bool isPageChanged() const { return data.pageChanged; }
  void markPageChanged(bool changed = true) { data.pageChanged = changed; }

  int getSettingValue(int index) const;
  void setSettingValue(int index, int val);

private:
  M5GFX* _display;
  M5Canvas canvas;
  DisplayData data;

  // Theme Colors
  uint16_t c_bg;
  uint16_t c_banner;
  uint16_t c_bannerTxt;
  uint16_t c_btn1;
  uint16_t c_btn2;
  uint16_t c_btnTxt;
  uint16_t c_active;
  uint16_t c_outline;

  // Internal Drawing Helpers
  void drawTopBanner();
  void drawBottomBanner();
  void drawButton(int x, int y, int w, int h, const char* label, uint32_t color, uint32_t txtColor);

  void drawPage1_2();
  void drawManualPage();
  void drawSettingsHub();
  void drawSettingsList(int startIndex, int count);
  void drawScreenDashboard();
  void drawWifiPortalPage();
  void drawNumpad();
  void drawActiveDipPage();
  void drawStopConfirmPage();
  void drawDipDonePage();

  // Text & Label Utilities
  const char* getPageTitle() const;
  void formatTime(int totalSeconds, char* buffer, int bufferSize) const;
};

#endif // DISPLAY_MANAGER_H`;

const DISPLAY_MANAGER_CPP_HIGHLIGHT = `// Excerpt from DisplayManager.cpp
#include "DisplayManager.h"

DisplayManager::DisplayManager() : _display(&M5.Display), canvas(&M5.Display) {}

bool DisplayManager::begin(M5GFX* display) {
  if (display) _display = display;
  if (!canvas.createSprite(_display->width(), _display->height())) return false;
  _display->setBrightness(map(data.s_brightness, 1, 99, 10, 255));
  applyTheme(data.s_theme);
  data.pageChanged = true;
  return true;
}

void DisplayManager::applyTheme(int themeId) {
  data.s_theme = themeId;
  if (data.s_theme == 0) { // Original
    c_bg = TFT_BLACK; c_banner = TFT_DARKCYAN; c_bannerTxt = TFT_WHITE;
    c_btn1 = TFT_NAVY; c_btn2 = TFT_MAROON; c_btnTxt = TFT_WHITE;
    c_active = TFT_DARKGREEN; c_outline = TFT_DARKGREY;
  } else if (data.s_theme == 1) { // Bee Mine Pastel
    c_bg = canvas.color565(35, 35, 35);
    c_banner = canvas.color565(255, 235, 130); c_bannerTxt = TFT_BLACK;
    c_btn1 = canvas.color565(255, 235, 130); c_btn2 = canvas.color565(140, 210, 180);
    c_btnTxt = TFT_BLACK; c_active = canvas.color565(130, 230, 130); c_outline = canvas.color565(80, 80, 80);
  } else { // Dark Mode
    c_bg = TFT_BLACK; c_banner = canvas.color565(40, 40, 40); c_bannerTxt = TFT_LIGHTGREY;
    c_btn1 = canvas.color565(60, 60, 60); c_btn2 = canvas.color565(60, 60, 60); c_btnTxt = TFT_WHITE;
    c_active = canvas.color565(80, 160, 80); c_outline = canvas.color565(100, 100, 100);
  }
}

void DisplayManager::renderCurrentPage() {
  if (!data.pageChanged) return;
  switch (data.currentPage) {
    case PAGE_WEIGHT_DIP: case PAGE_DIPS_DIP: drawPage1_2(); break;
    case PAGE_MANUAL: drawManualPage(); break;
    case PAGE_SETTINGS_HUB: drawSettingsHub(); break;
    case PAGE_SETTING_DIPPING: if (data.showNumpad) drawNumpad(); else drawSettingsList(0, 4); break;
    case PAGE_SETTING_MOTION:  if (data.showNumpad) drawNumpad(); else drawSettingsList(4, 4); break;
    case PAGE_SETTING_SENSOR:  if (data.showNumpad) drawNumpad(); else drawSettingsList(8, 1); break;
    case PAGE_SETTING_SCREEN:  drawScreenDashboard(); break;
    case PAGE_WIFI_PORTAL:     drawWifiPortalPage(); break;
    case PAGE_ACTIVE_DIP:      drawActiveDipPage(); break;
    case PAGE_STOP_CONFIRM:    drawStopConfirmPage(); break;
    case PAGE_DIP_DONE:        drawDipDonePage(); break;
  }
  canvas.pushSprite(0, 0);
  data.pageChanged = false;
}`;

const PIN_CONFIG_H_CODE = `#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// =========================================================
// BEE MINE CANDLE DIPPER - HARDWARE PIN DEFINITIONS
// All connections routed via M5Stack Proto Module (M-Bus)
// =========================================================

// --- TMC2209 Stepper Driver ---
#define PIN_MOTOR_STEP        8   // Hardware PWM pulse line
#define PIN_MOTOR_DIR         9   // Motor rotation direction
#define PIN_MOTOR_ENABLE     17   // Active LOW enable line
#define PIN_MOTOR_UART       18   // Single-Wire UART (1k Ohm inline resistor)

// --- Frame & Arm Sensors ---
#define PIN_TOP_LIMIT_SW      7   // Mechanical home switch (NC config)
#define PIN_WAX_LEVEL_SENS    6   // Arm Capacitive sensor (via 24V -> 3.3V Optocoupler)

// --- HX711 Load Cell Amplifier (Drag Chain) ---
#define PIN_HX711_DT          5   // Scale Serial Data line
#define PIN_HX711_SCK        13   // Scale Serial Clock line

#endif // PIN_CONFIG_H`;

const MOTOR_MANAGER_H_CODE = `#ifndef MOTOR_MANAGER_H
#define MOTOR_MANAGER_H

#include <Arduino.h>
#include "PinConfig.h"
#include "SensorManager.h"

enum TMCMode {
  MODE_STEALTHCHOP = 0, // Silent operation for low/medium speed
  MODE_SPREADCYCLE = 1  // High torque / rapid travel
};

class MotorManager {
public:
  MotorManager();
  void begin(SensorManager* sensorMgr);
  
  void stepMotor(bool directionUp, float speedMMps);
  void stepMotorBurst(bool directionUp, float speedMMps, uint32_t burstMs = 20);
  void stopMotor();

  void setTMCMode(TMCMode mode);
  TMCMode getTMCMode() const { return _currentMode; }

  bool performHoming(float speedMMps, bool (*stopCheck)() = nullptr);
  bool performDipBot(float downSpeedMMps, float upSpeedMMps, int holdTimeSec, bool (*stopCheck)() = nullptr);

  float getCurrentPositionMM() const { return _positionMM; }
  void setCurrentPositionMM(float pos) { _positionMM = pos; }

  static const float STEPS_PER_MM;
private:
  SensorManager* _sensors;
  TMCMode _currentMode;
  bool _enabled;
  float _positionMM;
  unsigned long _lastStepMicros;

  void sendUARTCommand(uint8_t reg, uint32_t val);
};

#endif // MOTOR_MANAGER_H`;

const SCALE_MANAGER_H_CODE = `#ifndef SCALE_MANAGER_H
#define SCALE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "PinConfig.h"

class ScaleManager {
public:
  ScaleManager();
  void begin(Preferences* prefs = nullptr);
  void update();

  void tare(uint8_t samples = 10);
  void calibrate(float knownWeightGrams, uint8_t samples = 10);

  float getWeightGrams() const { return _currentWeightGrams; }
  long getRawValue() const { return _rawReadout; }
  void setScaleFactor(float factor);
  float getScaleFactor() const { return _scaleFactor; }
  long getZeroOffset() const { return _zeroOffset; }

  long readRawBitbang();

private:
  Preferences* _prefs;
  long _zeroOffset;
  float _scaleFactor;
  float _currentWeightGrams;
  long _rawReadout;
  unsigned long _lastPollMs;
};

#endif // SCALE_MANAGER_H`;

const SENSOR_MANAGER_H_CODE = `#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include "PinConfig.h"

class SensorManager {
public:
  SensorManager();
  void begin();
  void update();

  bool isTopLimitHit() const { return _topLimitHit; }
  bool isCapSensorTriggered() const { return _capSensorTriggered; }

  bool readRawTopLimit(); // Returns HIGH when NC mechanical switch is pressed
  bool readRawCapSensor();

private:
  bool _topLimitHit;
  bool _capSensorTriggered;
  unsigned long _lastDebounceTime;
};

#endif // SENSOR_MANAGER_H`;

const MAIN_CPP_CODE = `#include <M5Unified.h>
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

DisplayManager display;
Preferences prefs;
SensorManager sensorMgr;
ScaleManager scaleMgr;
MotorManager motorMgr;

bool checkManualStop() {
  M5.update();
  if (M5.Touch.getCount() > 0) {
    auto t = M5.Touch.getDetail();
    if (t.wasPressed() || t.isPressed()) return true;
  }
  return false;
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  
  sensorMgr.begin();
  scaleMgr.begin(&prefs);
  motorMgr.begin(&sensorMgr);
  display.begin(&M5.Display);
}

void loop() {
  M5.update();
  DisplayData& data = display.getData();

  sensorMgr.update();
  scaleMgr.update();

  data.currentPosition = motorMgr.getCurrentPositionMM();
  data.currentWeight = scaleMgr.getWeightGrams();
  data.limitSwitchOn = sensorMgr.isTopLimitHit();
  data.capSensorOn = sensorMgr.isCapSensorTriggered();

  if (data.currentPage == PAGE_MANUAL) {
    if (data.isUpPressed) {
      motorMgr.stepMotorBurst(true, (float)data.manualSpeed, 15);
    } else if (data.isDownPressed) {
      motorMgr.stepMotorBurst(false, (float)data.manualSpeed, 15);
    } else {
      motorMgr.stopMotor();
    }
  }

  UiEvent event = display.updateTouch();
  switch (event) {
    case UI_EVENT_MANUAL_HOME:
      data.isHomingActive = true;
      display.markPageChanged(true);
      display.renderCurrentPage();
      motorMgr.performHoming((float)data.s_upSpeed, checkManualStop);
      data.isHomingActive = false;
      display.markPageChanged(true);
      break;

    case UI_EVENT_MANUAL_DIP_BOT:
      data.isDipBotActive = true;
      display.markPageChanged(true);
      display.renderCurrentPage();
      motorMgr.performDipBot((float)data.s_downSpeed, (float)data.s_upSpeed, data.s_subDipTime, checkManualStop);
      data.isDipBotActive = false;
      display.markPageChanged(true);
      break;

    case UI_EVENT_MANUAL_STOP:
      motorMgr.stopMotor();
      data.isHomingActive = false;
      data.isDipBotActive = false;
      display.markPageChanged(true);
      break;

    case UI_EVENT_MANUAL_TARE:
      scaleMgr.tare(10);
      break;
  }

  display.renderCurrentPage();
}`;

export default function App() {
  const [activeTab, setActiveTab] = useState<'preview' | 'main' | 'motor' | 'scale' | 'sensor' | 'header' | 'cpp' | 'pinconfig' | 'architecture'>('preview');
  const [copiedFile, setCopiedFile] = useState<string | null>(null);

  // M5Stack CoreS3 SE Interactive State
  const [page, setPage] = useState<number>(0); // 0: WEIGHT, 1: DIPS, 2: MANUAL, 3: SETTINGS_HUB, 4: S_DIP, 5: S_MOTION, 6: S_SENSOR, 7: S_SCREEN, 8: WIFI, 9: ACTIVE, 10: CONFIRM, 11: DONE
  const [subState, setSubState] = useState<number>(0); // 0: MAIN, 1: CONFIRM_SLIM, 2: CONFIRM_STD
  const [theme, setTheme] = useState<number>(1); // 0: Original, 1: Bee Mine, 2: Dark
  const [brightness, setBrightness] = useState<number>(50);
  const [limitSwitch, setLimitSwitch] = useState<boolean>(false);
  const [capSensor, setCapSensor] = useState<boolean>(false);
  const [weight, setWeight] = useState<number>(0);
  const [speed, setSpeed] = useState<number>(50);
  const [posMM, setPosMM] = useState<number>(0.0);
  const [isHoming, setIsHoming] = useState<boolean>(false);
  const [isDipBot, setIsDipBot] = useState<boolean>(false);
  const [showNumpad, setShowNumpad] = useState<boolean>(false);
  const [numpadVal, setNumpadVal] = useState<string>('');
  const [activeSettingIdx, setActiveSettingIdx] = useState<number>(0);
  const [lastEvent, setLastEvent] = useState<string>('UI Initialized (Hardware Managers Loaded)');

  const homingTimerRef = useRef<any[]>([]);
  const dipBotTimerRef = useRef<any[]>([]);
  const holdIntervalRef = useRef<any>(null);

  const stopHoming = () => {
    homingTimerRef.current.forEach(clearTimeout);
    homingTimerRef.current = [];
    setIsHoming(false);
    setLastEvent('Homing motion STOPPED by user');
  };

  const stopDipBot = () => {
    dipBotTimerRef.current.forEach(clearTimeout);
    dipBotTimerRef.current = [];
    setIsDipBot(false);
    setLastEvent('Dip Bot motion STOPPED by user');
  };

  const startManualMove = (isUp: boolean) => {
    if (holdIntervalRef.current) clearInterval(holdIntervalRef.current);
    const stepMove = () => {
      setPosMM((prevPos) => {
        if (isUp) {
          if (prevPos <= 0) {
            setLimitSwitch(true);
            return 0;
          }
          const next = Math.max(0, prevPos - (speed * 0.05));
          if (next === 0) setLimitSwitch(true);
          return next;
        } else {
          setLimitSwitch(false);
          return prevPos + (speed * 0.05);
        }
      });
    };
    stepMove();
    holdIntervalRef.current = setInterval(stepMove, 50);
  };

  const stopManualMove = () => {
    if (holdIntervalRef.current) {
      clearInterval(holdIntervalRef.current);
      holdIntervalRef.current = null;
    }
  };

  const settingNames = ["Slim Weight", "Std Weight", "Slim Dips", "Std Dips", "1st Dip Time", "+ Dips Time", "Up Speed", "Down Speed", "Col. Limit", "Soft Limit", "Weight Dwell", "Soft Ramp", "Driver Mode", "Hybrid Speed"];
  const settingUnits = ["g", "g", "dips", "dips", "s", "s", "mm/s", "mm/s", "g", "mm", "ms", "ms", "", "mm/s"];
  const [settingValues, setSettingValues] = useState<number[]>([1500, 2500, 15, 25, 10, 4, 60, 50, -50, 500, 500, 50, 1, 30]);
  const [dipAborted, setDipAborted] = useState(false);
  const [finalDurationSecs, setFinalDurationSecs] = useState(134);
  const [activeIsSlim, setActiveIsSlim] = useState(true);
  const [activeIsWeight, setActiveIsWeight] = useState(true);

  const copyCode = (text: string, filename: string) => {
    navigator.clipboard.writeText(text);
    setCopiedFile(filename);
    setTimeout(() => setCopiedFile(null), 2000);
  };

  // Color schemes for M5Canvas preview
  const getThemeStyles = () => {
    if (theme === 0) { // Original Navy/Maroon
      return {
        bg: '#000000',
        bannerBg: '#008b8b',
        bannerTxt: '#ffffff',
        btn1Bg: '#000080',
        btn2Bg: '#800000',
        btnTxt: '#ffffff',
        activeBg: '#006400',
        outlineBg: '#a9a9a9'
      };
    } else if (theme === 1) { // Bee Mine Pastel
      return {
        bg: '#232323',
        bannerBg: '#ffeb82',
        bannerTxt: '#000000',
        btn1Bg: '#ffeb82',
        btn2Bg: '#8cd2b4',
        btnTxt: '#000000',
        activeBg: '#82e682',
        outlineBg: '#505050'
      };
    } else { // Dark Mode
      return {
        bg: '#000000',
        bannerBg: '#282828',
        bannerTxt: '#d3d3d3',
        btn1Bg: '#3c3c3c',
        btn2Bg: '#3c3c3c',
        btnTxt: '#ffffff',
        activeBg: '#50a050',
        outlineBg: '#646464'
      };
    }
  };

  const ts = getThemeStyles();

  const getPageTitle = (p: number) => {
    switch (p) {
      case 0: return subState === 0 ? "Weight Dipping" : "Confirm Weight Dip";
      case 1: return subState === 0 ? "Dips Dipping" : "Confirm Dips Dip";
      case 2: return "Manual Control";
      case 3: return "Settings Hub";
      case 8: return "Wi-Fi Portal";
      default: return "Settings";
    }
  };

  return (
    <div className="min-h-screen bg-neutral-900 text-neutral-100 flex flex-col font-sans">
      {/* Header Bar */}
      <header className="border-b border-neutral-800 bg-neutral-950 px-6 py-4 flex items-center justify-between">
        <div className="flex items-center gap-3">
          <div className="w-9 h-9 rounded-lg bg-amber-400/10 border border-amber-400/30 flex items-center justify-center text-amber-400 font-bold text-lg">
            🐝
          </div>
          <div>
            <h1 className="text-lg font-semibold text-white tracking-tight flex items-center gap-2">
              Bee Mine Candle Dipper
              <span className="text-xs px-2 py-0.5 rounded bg-emerald-500/10 border border-emerald-500/30 text-emerald-400 font-mono">
                Phase 2 - Step 1 Complete
              </span>
            </h1>
            <p className="text-xs text-neutral-400">
              Refactored DisplayManager Class (Double-Buffered M5Canvas UI Engine)
            </p>
          </div>
        </div>

        {/* Navigation Tabs */}
        <div className="flex items-center gap-1 bg-neutral-900 p-1 rounded-lg border border-neutral-800 flex-wrap">
          <button
            onClick={() => setActiveTab('preview')}
            className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all ${
              activeTab === 'preview' 
                ? 'bg-amber-400 text-neutral-950 shadow' 
                : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
            }`}
          >
            <Eye className="w-3.5 h-3.5" />
            Simulator
          </button>
          <button
            onClick={() => setActiveTab('main')}
            className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all ${
              activeTab === 'main' 
                ? 'bg-amber-400 text-neutral-950 shadow' 
                : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
            }`}
          >
            <Code2 className="w-3.5 h-3.5" />
            main.cpp
          </button>
          <button
            onClick={() => setActiveTab('motor')}
            className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all ${
              activeTab === 'motor' 
                ? 'bg-amber-400 text-neutral-950 shadow' 
                : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
            }`}
          >
            <Cpu className="w-3.5 h-3.5" />
            MotorManager
          </button>
          <button
            onClick={() => setActiveTab('scale')}
            className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all ${
              activeTab === 'scale' 
                ? 'bg-amber-400 text-neutral-950 shadow' 
                : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
            }`}
          >
            <Scale className="w-3.5 h-3.5" />
            ScaleManager
          </button>
          <button
            onClick={() => setActiveTab('sensor')}
            className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all ${
              activeTab === 'sensor' 
                ? 'bg-amber-400 text-neutral-950 shadow' 
                : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
            }`}
          >
            <Sliders className="w-3.5 h-3.5" />
            SensorManager
          </button>
          <button
            onClick={() => setActiveTab('pinconfig')}
            className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all ${
              activeTab === 'pinconfig' 
                ? 'bg-amber-400 text-neutral-950 shadow' 
                : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
            }`}
          >
            <Settings className="w-3.5 h-3.5" />
            PinConfig.h
          </button>
          <button
            onClick={() => setActiveTab('header')}
            className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all ${
              activeTab === 'header' 
                ? 'bg-amber-400 text-neutral-950 shadow' 
                : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
            }`}
          >
            DisplayManager.h
          </button>
          <button
            onClick={() => setActiveTab('cpp')}
            className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md text-xs font-medium transition-all ${
              activeTab === 'cpp' 
                ? 'bg-amber-400 text-neutral-950 shadow' 
                : 'text-neutral-400 hover:text-white hover:bg-neutral-800'
            }`}
          >
            DisplayManager.cpp
          </button>
        </div>
      </header>

      {/* Main Content Area */}
      <div className="flex-1 p-6 max-w-7xl mx-auto w-full">
        {activeTab === 'preview' && (
          <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
            {/* CoreS3 Hardware Device Frame */}
            <div className="lg:col-span-7 flex flex-col items-center justify-center">
              <div className="relative bg-neutral-800 border-4 border-neutral-700 rounded-3xl p-6 shadow-2xl max-w-md w-full">
                {/* M5Stack CoreS3 Logo & Status */}
                <div className="flex items-center justify-between mb-4 px-2">
                  <div className="flex items-center gap-2">
                    <span className="text-xs font-bold font-mono tracking-widest text-neutral-300">
                      M5STACK CORES3 SE
                    </span>
                  </div>
                  <div className="flex items-center gap-2">
                    <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse"></span>
                    <span className="text-[10px] text-neutral-400 font-mono">320x240 IPS Touch</span>
                  </div>
                </div>

                {/* Simulated 320x240 Display Screen */}
                <div 
                  className="w-full aspect-[4/3] rounded-xl overflow-hidden relative shadow-inner border border-neutral-950 select-none cursor-pointer"
                  style={{ backgroundColor: ts.bg }}
                >
                  {/* Top Banner (15% height = 36px) */}
                  <div 
                    className="h-[36px] w-full px-3 flex items-center justify-between text-xs font-bold font-mono"
                    style={{ backgroundColor: ts.bannerBg, color: ts.bannerTxt }}
                  >
                    {page === 2 ? (
                      <>
                        <span>POS: {posMM.toFixed(1)}mm</span>
                        <span>WT: {weight.toFixed(0)}g</span>
                      </>
                    ) : page === 9 ? (
                      <span className="mx-auto font-sans">DIPPING IN PROGRESS</span>
                    ) : page === 10 ? (
                      <span className="mx-auto font-sans">CONFIRM ABORT</span>
                    ) : page === 11 ? (
                      <span className="mx-auto font-sans">{dipAborted ? 'PROCESS CANCELLED' : 'PROCESS COMPLETE'}</span>
                    ) : (
                      <>
                        <span className="font-sans">Thu 01/01/26</span>
                        <span className="font-sans">15:10</span>
                      </>
                    )}
                  </div>

                  {/* Main Page Content (Center 70% height = 168px) */}
                  <div className="h-[168px] w-full p-2 flex flex-col justify-center items-center relative">
                    {/* Pages 0 & 1: Weight / Dips Based */}
                    {(page === 0 || page === 1) && (
                      <div className="w-full h-full flex items-center justify-center gap-3 px-2">
                        {subState === 0 ? (
                          <>
                            <button
                              onClick={() => {
                                setSubState(1);
                                setActiveIsSlim(true);
                                setActiveIsWeight(page === 0);
                                setLastEvent(`Selected SLIM (${page === 0 ? settingValues[0] + 'g' : settingValues[2] + ' dips'})`);
                              }}
                              className="flex-1 h-24 rounded-xl font-bold text-xs shadow-md transition-all active:scale-95 flex flex-col items-center justify-center gap-1"
                              style={{ backgroundColor: ts.btn1Bg, color: ts.btnTxt }}
                            >
                              <span className="text-sm">SLIM</span>
                              <span className="text-[10px] opacity-80">
                                ({page === 0 ? `${settingValues[0]} g` : `${settingValues[2]} dips`})
                              </span>
                            </button>
                            <button
                              onClick={() => {
                                setSubState(2);
                                setActiveIsSlim(false);
                                setActiveIsWeight(page === 0);
                                setLastEvent(`Selected STANDARD (${page === 0 ? settingValues[1] + 'g' : settingValues[3] + ' dips'})`);
                              }}
                              className="flex-1 h-24 rounded-xl font-bold text-xs shadow-md transition-all active:scale-95 flex flex-col items-center justify-center gap-1"
                              style={{ backgroundColor: ts.btn2Bg, color: ts.btnTxt }}
                            >
                              <span className="text-sm">STANDARD</span>
                              <span className="text-[10px] opacity-80">
                                ({page === 0 ? `${settingValues[1]} g` : `${settingValues[3]} dips`})
                              </span>
                            </button>
                          </>
                        ) : (
                          <div className="w-full h-full flex flex-col justify-between p-1">
                            <div className="bg-neutral-800/80 p-2 rounded-lg text-[11px] flex flex-col gap-1 text-white border border-white/10">
                              <div className="flex justify-between">
                                <span className="opacity-70">Mode:</span>
                                <span className="font-semibold">{activeIsWeight ? 'Weight' : 'Dips'}</span>
                                <span className="opacity-70 ml-2">Profile:</span>
                                <span className="font-semibold">{activeIsSlim ? 'Slim' : 'Standard'}</span>
                              </div>
                              <div className="flex justify-between">
                                <span className="opacity-70">Target:</span>
                                <span className="font-bold text-yellow-300">
                                  {activeIsWeight
                                    ? (activeIsSlim ? `${settingValues[0]} g` : `${settingValues[1]} g`)
                                    : (activeIsSlim ? `${settingValues[2]} dips` : `${settingValues[3]} dips`)}
                                </span>
                                <span className="opacity-70">Est:</span>
                                <span className="font-semibold">02:15</span>
                              </div>
                            </div>
                            <button
                              onClick={() => { setPage(9); setDipAborted(false); setLastEvent('Started Dipping Process'); }}
                              className="w-full py-2.5 rounded-xl font-bold text-xs shadow-md transition-all active:scale-95 flex items-center justify-center"
                              style={{ backgroundColor: ts.activeBg, color: '#000000' }}
                            >
                              START DIP
                            </button>
                          </div>
                        )}
                      </div>
                    )}

                    {/* Page 2: Manual Control */}
                    {page === 2 && (
                      <div className="w-full h-full grid grid-cols-12 gap-2 p-1">
                        <div className="col-span-7 flex flex-col justify-between">
                          <button
                            onMouseDown={() => startManualMove(true)}
                            onMouseUp={stopManualMove}
                            onMouseLeave={stopManualMove}
                            onTouchStart={() => startManualMove(true)}
                            onTouchEnd={stopManualMove}
                            className={`h-11 rounded-xl flex items-center justify-center font-bold text-xs select-none ${limitSwitch ? 'ring-2 ring-emerald-400' : ''}`}
                            style={{ backgroundColor: ts.btn1Bg, color: ts.btnTxt }}
                          >
                            ▲ UP
                          </button>
                          <div className="flex items-center justify-between px-2 text-xs font-bold" style={{ color: ts.btnTxt }}>
                            <button onClick={() => setSpeed(Math.max(5, speed - 5))} className="w-8 h-8 rounded bg-neutral-700 text-white font-bold flex items-center justify-center">-</button>
                            <span>{speed}</span>
                            <button onClick={() => setSpeed(Math.min(100, speed + 5))} className="w-8 h-8 rounded bg-neutral-700 text-white font-bold flex items-center justify-center">+</button>
                          </div>
                          <button
                            onMouseDown={() => startManualMove(false)}
                            onMouseUp={stopManualMove}
                            onMouseLeave={stopManualMove}
                            onTouchStart={() => startManualMove(false)}
                            onTouchEnd={stopManualMove}
                            className="h-11 rounded-xl flex items-center justify-center font-bold text-xs select-none"
                            style={{ backgroundColor: ts.btn1Bg, color: ts.btnTxt }}
                          >
                            ▼ DOWN
                          </button>
                        </div>
                        <div className="col-span-5 flex flex-col justify-between gap-1">
                          <button
                            onClick={() => {
                              if (isHoming) {
                                stopHoming();
                              } else {
                                setIsHoming(true);
                                setLastEvent('Homing Stage 1: Rapid search towards top limit switch...');
                                const t1 = setTimeout(() => {
                                  setLastEvent('Homing Stage 2: Backing down 2mm & slow precision re-approach...');
                                  const t2 = setTimeout(() => {
                                    setPosMM(0.0);
                                    setLimitSwitch(true);
                                    setIsHoming(false);
                                    setLastEvent('Homing complete: Position zeroed to 0.0mm');
                                  }, 800);
                                  homingTimerRef.current.push(t2);
                                }, 800);
                                homingTimerRef.current = [t1];
                              }
                            }}
                            className="flex-1 rounded-lg text-xs font-bold flex items-center justify-center transition-all"
                            style={{ 
                              backgroundColor: isHoming ? '#ef4444' : (limitSwitch ? ts.activeBg : ts.outlineBg), 
                              color: isHoming ? '#ffffff' : (limitSwitch ? '#000000' : '#ffffff') 
                            }}
                          >
                            {isHoming ? 'STOP' : 'HOME'}
                          </button>
                          <button
                            onClick={() => {
                              if (isDipBot) {
                                stopDipBot();
                              } else {
                                setIsDipBot(true);
                                setLastEvent('Dip Bot Stage 1: Lowering to capacitive wax sensor...');
                                const t1 = setTimeout(() => {
                                  setPosMM(120.0);
                                  setCapSensor(true);
                                  setLastEvent('Dip Bot Stage 2: In wax, holding...');
                                  const t2 = setTimeout(() => {
                                    setCapSensor(false);
                                    setLastEvent('Dip Bot Stage 3: Returning to 0.0mm home...');
                                    const t3 = setTimeout(() => {
                                      setPosMM(0.0);
                                      setLimitSwitch(true);
                                      setIsDipBot(false);
                                      setLastEvent('Dip Bot complete: Carriage home');
                                    }, 800);
                                    dipBotTimerRef.current.push(t3);
                                  }, 800);
                                  dipBotTimerRef.current.push(t2);
                                }, 800);
                                dipBotTimerRef.current = [t1];
                              }
                            }}
                            className="flex-1 rounded-lg text-xs font-bold flex items-center justify-center transition-all"
                            style={{ 
                              backgroundColor: isDipBot ? '#ef4444' : (capSensor ? ts.activeBg : ts.outlineBg), 
                              color: isDipBot ? '#ffffff' : (capSensor ? '#000000' : '#ffffff') 
                            }}
                          >
                            {isDipBot ? 'STOP' : 'DIP BOT'}
                          </button>
                          <button
                            onClick={() => { setWeight(0); setLastEvent('Tare executed: Scale zeroed (10 samples)'); }}
                            className="flex-1 rounded-lg text-xs font-bold bg-amber-500 text-black flex items-center justify-center"
                          >
                            TARE
                          </button>
                        </div>
                      </div>
                    )}

                    {/* Page 3: Settings Hub */}
                    {page === 3 && (
                      <div className="w-full h-full grid grid-cols-2 gap-2 p-2">
                        <button onClick={() => setPage(4)} className="rounded-xl font-bold text-xs" style={{ backgroundColor: ts.btn1Bg, color: ts.btnTxt }}>Dipping</button>
                        <button onClick={() => setPage(5)} className="rounded-xl font-bold text-xs" style={{ backgroundColor: ts.btn1Bg, color: ts.btnTxt }}>Motion</button>
                        <button onClick={() => setPage(6)} className="rounded-xl font-bold text-xs" style={{ backgroundColor: ts.btn1Bg, color: ts.btnTxt }}>Sensor</button>
                        <button onClick={() => setPage(7)} className="rounded-xl font-bold text-xs" style={{ backgroundColor: ts.btn2Bg, color: ts.btnTxt }}>Screen</button>
                      </div>
                    )}

                    {/* Sub Settings Pages (4, 5, 6) */}
                    {(page === 4 || page === 5 || page === 6) && !showNumpad && (
                      <div className="w-full h-full flex flex-col gap-1 p-1">
                        {(page === 4 ? [0,1,2,3] : page === 5 ? [4,5,6,7] : [8,9,10,11]).map((idx) => (
                          <div key={idx} className="flex items-center justify-between text-xs px-2 py-0.5 rounded bg-neutral-800/40">
                            <span style={{ color: ts.bannerTxt === '#000000' ? '#fff' : ts.btnTxt }}>{settingNames[idx]}</span>
                            <button
                              onClick={() => { setActiveSettingIdx(idx); setNumpadVal(''); setShowNumpad(true); }}
                              className="px-2 py-0.5 rounded font-bold text-xs text-white"
                              style={{ backgroundColor: ts.outlineBg }}
                            >
                              {settingValues[idx]} {settingUnits[idx]}
                            </button>
                          </div>
                        ))}
                        {page === 6 && (
                          <button
                            onClick={() => { setPage(8); setLastEvent('Opened Wi-Fi Portal'); }}
                            className="mt-1 w-full py-1 rounded-lg font-bold text-xs"
                            style={{ backgroundColor: ts.btn2Bg, color: ts.btnTxt }}
                          >
                            RESET WI-FI PORTAL
                          </button>
                        )}
                      </div>
                    )}

                    {/* Page 7: Screen Dashboard */}
                    {page === 7 && (
                      <div className="w-full h-full flex flex-col items-center justify-center gap-2 p-2 text-xs">
                        <span style={{ color: ts.bannerTxt === '#000000' ? '#fff' : ts.btnTxt }}>Brightness</span>
                        <div className="w-full h-5 rounded-full relative overflow-hidden" style={{ backgroundColor: ts.outlineBg }}>
                          <div className="h-full rounded-full" style={{ width: `${brightness}%`, backgroundColor: ts.activeBg }}></div>
                          <span className="absolute inset-0 flex items-center justify-center text-[10px] font-bold text-white">{brightness}%</span>
                        </div>
                        <span style={{ color: ts.bannerTxt === '#000000' ? '#fff' : ts.btnTxt }}>Color Theme (Tap to Change)</span>
                        <button
                          onClick={() => {
                            const nt = (theme + 1) % 3;
                            setTheme(nt);
                            setLastEvent(`Applied Theme: ${['Original', 'Bee Mine', 'Dark Mode'][nt]}`);
                          }}
                          className="w-full py-2 rounded-xl font-bold"
                          style={{ backgroundColor: ts.btn1Bg, color: ts.btnTxt }}
                        >
                          {['Original', 'Bee Mine', 'Dark Mode'][theme]}
                        </button>
                      </div>
                    )}

                    {/* Page 8: Wifi Portal */}
                    {page === 8 && (
                      <div className="w-full h-full flex flex-col items-center justify-center p-2 text-center text-xs">
                        <span className="font-bold mb-1" style={{ color: ts.bannerTxt === '#000000' ? '#fff' : ts.btnTxt }}>WI-FI SETUP PORTAL</span>
                        <div className="w-full p-2 rounded-xl" style={{ backgroundColor: ts.outlineBg }}>
                          <p className="text-white text-[10px]">Connect phone to Wi-Fi:</p>
                          <p className="text-amber-300 font-bold text-xs">BeeMine-Setup</p>
                          <p className="text-white text-[10px]">Open http://192.168.4.1</p>
                        </div>
                      </div>
                    )}

                    {/* Page 9: Active Dip */}
                    {page === 9 && (
                      <div className="w-full h-full flex flex-col items-center justify-center gap-1 p-2 text-center text-xs">
                        <span className="text-amber-400 font-bold">Dip 1: Lowering...</span>
                        <span style={{ color: ts.bannerTxt === '#000000' ? '#fff' : ts.btnTxt }}>Time: 00:04 / 02:45 (est)</span>
                        <span style={{ color: ts.bannerTxt === '#000000' ? '#fff' : ts.btnTxt }}>
                          {activeIsWeight ? 'Weight: 120g / 1500g' : 'Dips: 1 / 15'}
                        </span>
                        <button
                          onClick={() => setPage(10)}
                          className="mt-2 w-full py-1.5 rounded-lg border border-red-500 text-red-400 font-bold text-[10px]"
                        >
                          PRESS ANYWHERE TO STOP
                        </button>
                      </div>
                    )}

                    {/* Page 10: Stop Confirm */}
                    {page === 10 && (
                      <div className="w-full h-full flex flex-col items-center justify-center gap-3 text-center">
                        <span className="text-white text-xs font-bold">Abort current dipping?</span>
                        <div className="flex items-center gap-3 w-full px-4">
                          <button onClick={() => setPage(9)} className="flex-1 py-3 rounded-xl font-bold text-xs text-white" style={{ backgroundColor: ts.outlineBg }}>NO</button>
                          <button 
                            onClick={() => { setDipAborted(true); setFinalDurationSecs(45); setPage(11); setLastEvent('Dipping cancelled by user'); }} 
                            className="flex-1 py-3 rounded-xl font-bold text-xs bg-red-600 text-white"
                          >
                            YES
                          </button>
                        </div>
                      </div>
                    )}

                    {/* Page 11: Dip Done */}
                    {page === 11 && (
                      <div className="w-full h-full flex flex-col items-center justify-center gap-2 text-center">
                        {dipAborted ? (
                          <>
                            <span className="text-red-400 text-sm font-bold">
                              {activeIsSlim ? 'Slim cancelled' : 'Std. cancelled'}
                            </span>
                            <span className="text-neutral-300 text-xs font-mono">
                              Cancelled at {Math.floor(finalDurationSecs / 60)}m {finalDurationSecs % 60}s
                            </span>
                            <button 
                              onClick={() => { setPage(0); setSubState(0); setDipAborted(false); }} 
                              className="w-full mt-2 py-2.5 rounded-xl font-bold text-xs bg-red-800 text-white"
                            >
                              BACK
                            </button>
                          </>
                        ) : (
                          <>
                            <span className="text-emerald-400 text-sm font-bold">
                              Success!, {activeIsSlim ? 'Slim ready' : 'Std. ready'}
                            </span>
                            <span className="text-neutral-300 text-xs font-mono">
                              Completed in {Math.floor(finalDurationSecs / 60)}m {finalDurationSecs % 60}s
                            </span>
                            <button 
                              onClick={() => { setPage(0); setSubState(0); setDipAborted(false); }} 
                              className="w-full mt-2 py-2.5 rounded-xl font-bold text-xs text-black" 
                              style={{ backgroundColor: ts.activeBg }}
                            >
                              FINISH
                            </button>
                          </>
                        )}
                      </div>
                    )}

                    {/* Numpad Overlay */}
                    {showNumpad && (
                      <div className="absolute inset-1 rounded-xl p-2 flex flex-col justify-between" style={{ backgroundColor: ts.bannerBg }}>
                        <div className="text-center font-bold text-xs" style={{ color: ts.bannerTxt }}>{settingNames[activeSettingIdx]}</div>
                        <div className="bg-black text-white px-2 py-1 rounded text-center font-mono font-bold text-xs">{numpadVal || '0'}</div>
                        <div className="grid grid-cols-3 gap-1">
                          {['1','2','3','4','5','6','7','8','9','DEL','0','CLR'].map((k) => (
                            <button
                              key={k}
                              onClick={() => {
                                if (k === 'DEL') setNumpadVal(numpadVal.slice(0, -1));
                                else if (k === 'CLR') setNumpadVal('');
                                else setNumpadVal(numpadVal + k);
                              }}
                              className="py-1 rounded text-[10px] font-bold text-white"
                              style={{ backgroundColor: ts.outlineBg }}
                            >
                              {k}
                            </button>
                          ))}
                        </div>
                        <div className="flex items-center gap-1 mt-1">
                          <button onClick={() => setShowNumpad(false)} className="flex-1 py-1 rounded bg-red-800 text-white font-bold text-[10px]">CAN</button>
                          <button 
                            onClick={() => {
                              const newArr = [...settingValues];
                              newArr[activeSettingIdx] = parseInt(numpadVal) || 0;
                              setSettingValues(newArr);
                              setShowNumpad(false);
                              setLastEvent(`Updated ${settingNames[activeSettingIdx]} = ${parseInt(numpadVal) || 0}`);
                            }} 
                            className="flex-1 py-1 rounded bg-emerald-700 text-white font-bold text-[10px]"
                          >
                            OK
                          </button>
                        </div>
                      </div>
                    )}
                  </div>

                  {/* Bottom Banner (15% height = 36px) */}
                  {page < 9 && (
                    <div 
                      onClick={() => {
                        if (page === 8) { setPage(3); setLastEvent('Exited Wi-Fi Portal'); }
                        else if (page >= 4 && page <= 7) { setPage(3); setLastEvent('Returned to Settings Hub'); }
                        else if (subState !== 0) { setSubState(0); setLastEvent('Cancelled Profile Confirmation'); }
                        else {
                          const nextP = (page + 1) % 4;
                          setPage(nextP);
                          setLastEvent(`Navigated to ${getPageTitle(nextP)}`);
                        }
                      }}
                      className="h-[36px] w-full flex items-center justify-center text-xs font-bold cursor-pointer text-white"
                      style={{ backgroundColor: ts.outlineBg }}
                    >
                      {page === 8 ? 'EXIT PORTAL' : (page >= 4 && page <= 7) ? '< BACK TO HUB' : subState === 0 ? `< ${getPageTitle(page)} >` : 'CANCEL'}
                    </div>
                  )}
                </div>

                {/* Touch Simulation Help Footer */}
                <div className="mt-4 pt-3 border-t border-neutral-700/50 flex items-center justify-between text-xs text-neutral-400 font-mono">
                  <span>Last Event:</span>
                  <span className="text-amber-400 font-sans truncate max-w-[220px]">{lastEvent}</span>
                </div>
              </div>
            </div>

            {/* Side Control & Event Console */}
            <div className="lg:col-span-5 flex flex-col gap-4">
              <div className="bg-neutral-950 border border-neutral-800 rounded-xl p-5">
                <h3 className="text-sm font-semibold text-white mb-3 flex items-center gap-2">
                  <Sliders className="w-4 h-4 text-amber-400" />
                  M5Canvas DisplayManager Test Bench
                </h3>
                <p className="text-xs text-neutral-400 mb-4 leading-relaxed">
                  Test the extracted <code className="text-amber-300 font-mono">DisplayManager</code> class methods directly. Tap buttons on the screen or use quick state shortcuts below.
                </p>

                <div className="grid grid-cols-2 gap-2 mb-4">
                  <button
                    onClick={() => { setPage(0); setSubState(0); }}
                    className="flex items-center justify-center gap-2 px-3 py-2 rounded-lg bg-neutral-900 border border-neutral-800 text-xs font-medium hover:border-amber-400/50 text-neutral-200"
                  >
                    <Scale className="w-3.5 h-3.5 text-amber-400" />
                    Weight Page
                  </button>
                  <button
                    onClick={() => { setPage(2); }}
                    className="flex items-center justify-center gap-2 px-3 py-2 rounded-lg bg-neutral-900 border border-neutral-800 text-xs font-medium hover:border-amber-400/50 text-neutral-200"
                  >
                    <Sliders className="w-3.5 h-3.5 text-emerald-400" />
                    Manual Control
                  </button>
                  <button
                    onClick={() => { setPage(3); }}
                    className="flex items-center justify-center gap-2 px-3 py-2 rounded-lg bg-neutral-900 border border-neutral-800 text-xs font-medium hover:border-amber-400/50 text-neutral-200"
                  >
                    <Settings className="w-3.5 h-3.5 text-blue-400" />
                    Settings Hub
                  </button>
                  <button
                    onClick={() => { setPage(8); }}
                    className="flex items-center justify-center gap-2 px-3 py-2 rounded-lg bg-neutral-900 border border-neutral-800 text-xs font-medium hover:border-amber-400/50 text-neutral-200"
                  >
                    <Wifi className="w-3.5 h-3.5 text-purple-400" />
                    Wi-Fi Portal
                  </button>
                </div>

                <div className="space-y-3 pt-3 border-t border-neutral-800">
                  <div>
                    <label className="text-xs text-neutral-400 block mb-1">Color Theme Selection</label>
                    <div className="grid grid-cols-3 gap-2">
                      {['Original', 'Bee Mine', 'Dark Mode'].map((tName, i) => (
                        <button
                          key={tName}
                          onClick={() => { setTheme(i); setLastEvent(`Applied Theme: ${tName}`); }}
                          className={`py-1.5 rounded text-xs font-medium border ${
                            theme === i 
                              ? 'bg-amber-400/10 border-amber-400 text-amber-300' 
                              : 'bg-neutral-900 border-neutral-800 text-neutral-400 hover:text-white'
                          }`}
                        >
                          {tName}
                        </button>
                      ))}
                    </div>
                  </div>

                  <div>
                    <div className="flex justify-between text-xs text-neutral-400 mb-1">
                      <span>Screen Brightness PWM</span>
                      <span className="font-mono text-amber-400">{brightness}%</span>
                    </div>
                    <input
                      type="range"
                      min="1"
                      max="99"
                      value={brightness}
                      onChange={(e) => setBrightness(parseInt(e.target.value))}
                      className="w-full accent-amber-400 bg-neutral-800 rounded h-1.5"
                    />
                  </div>
                </div>
              </div>

              {/* Step 1 Verification Status */}
              <div className="bg-emerald-950/30 border border-emerald-800/40 rounded-xl p-4">
                <div className="flex items-center gap-2 text-emerald-400 font-semibold text-xs mb-2">
                  <CheckCircle2 className="w-4 h-4" />
                  STEP 1 Extraction Verification
                </div>
                <ul className="text-xs text-neutral-300 space-y-1.5 list-disc list-inside leading-relaxed">
                  <li>Created <code className="text-emerald-300 font-mono">DisplayManager.h</code> with state structures, UI event enums & public methods.</li>
                  <li>Created <code className="text-emerald-300 font-mono">DisplayManager.cpp</code> encapsulating double-buffered <code className="text-emerald-300 font-mono">M5Canvas</code> drawing.</li>
                  <li>Touch hit-testing extracted into <code className="text-emerald-300 font-mono">updateTouch()</code> returning clean <code className="text-emerald-300 font-mono">UiEvent</code> callbacks.</li>
                  <li>Zero change to visual layout or pixel dimensions (320x240 M5Canvas).</li>
                </ul>
              </div>
            </div>
          </div>
        )}

        {activeTab === 'main' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Code2 className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/main.cpp</span>
              </div>
              <button
                onClick={() => copyCode(MAIN_CPP_CODE, 'main')}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all"
              >
                {copiedFile === 'main' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                {copiedFile === 'main' ? 'Copied main.cpp!' : 'Copy main.cpp'}
              </button>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {MAIN_CPP_CODE}
            </pre>
          </div>
        )}

        {activeTab === 'motor' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Cpu className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/MotorManager.h</span>
              </div>
              <button
                onClick={() => copyCode(MOTOR_MANAGER_H_CODE, 'motor')}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all"
              >
                {copiedFile === 'motor' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                {copiedFile === 'motor' ? 'Copied MotorManager.h!' : 'Copy MotorManager.h'}
              </button>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {MOTOR_MANAGER_H_CODE}
            </pre>
          </div>
        )}

        {activeTab === 'scale' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Scale className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/ScaleManager.h</span>
              </div>
              <button
                onClick={() => copyCode(SCALE_MANAGER_H_CODE, 'scale')}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all"
              >
                {copiedFile === 'scale' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                {copiedFile === 'scale' ? 'Copied ScaleManager.h!' : 'Copy ScaleManager.h'}
              </button>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {SCALE_MANAGER_H_CODE}
            </pre>
          </div>
        )}

        {activeTab === 'sensor' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Sliders className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/SensorManager.h</span>
              </div>
              <button
                onClick={() => copyCode(SENSOR_MANAGER_H_CODE, 'sensor')}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all"
              >
                {copiedFile === 'sensor' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                {copiedFile === 'sensor' ? 'Copied SensorManager.h!' : 'Copy SensorManager.h'}
              </button>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {SENSOR_MANAGER_H_CODE}
            </pre>
          </div>
        )}

        {activeTab === 'header' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Code2 className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/DisplayManager.h</span>
              </div>
              <button
                onClick={() => copyCode(DISPLAY_MANAGER_H_CODE, 'header')}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all"
              >
                {copiedFile === 'header' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                {copiedFile === 'header' ? 'Copied Header!' : 'Copy DisplayManager.h'}
              </button>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {DISPLAY_MANAGER_H_CODE}
            </pre>
          </div>
        )}

        {activeTab === 'cpp' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Cpu className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/DisplayManager.cpp</span>
              </div>
              <button
                onClick={() => copyCode(DISPLAY_MANAGER_CPP_HIGHLIGHT, 'cpp')}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all"
              >
                {copiedFile === 'cpp' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                {copiedFile === 'cpp' ? 'Copied Implementation!' : 'Copy DisplayManager.cpp'}
              </button>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {DISPLAY_MANAGER_CPP_HIGHLIGHT}
            </pre>
          </div>
        )}

        {activeTab === 'pinconfig' && (
          <div className="bg-neutral-950 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
            <div className="px-5 py-3 border-b border-neutral-800 flex items-center justify-between bg-neutral-900/50">
              <div className="flex items-center gap-2">
                <Settings className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-mono font-semibold text-white">/PinConfig.h</span>
              </div>
              <button
                onClick={() => copyCode(PIN_CONFIG_H_CODE, 'pinconfig')}
                className="flex items-center gap-1.5 px-3 py-1 rounded bg-neutral-800 hover:bg-neutral-700 text-xs font-medium text-neutral-200 transition-all"
              >
                {copiedFile === 'pinconfig' ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                {copiedFile === 'pinconfig' ? 'Copied Header!' : 'Copy PinConfig.h'}
              </button>
            </div>
            <pre className="p-5 font-mono text-xs text-neutral-300 overflow-x-auto leading-relaxed bg-neutral-950 max-h-[600px] overflow-y-auto">
              {PIN_CONFIG_H_CODE}
            </pre>
          </div>
        )}

        {activeTab === 'architecture' && (
          <div className="space-y-6">
            <div className="bg-neutral-950 border border-neutral-800 rounded-xl p-6">
              <h2 className="text-base font-semibold text-white mb-2 flex items-center gap-2">
                <Sparkles className="w-5 h-5 text-amber-400" />
                Refactoring & Hardware Integration - Phase 2 (STEP 1)
              </h2>
              <p className="text-xs text-neutral-400 leading-relaxed">
                The monolithic <code className="text-amber-300 font-mono">main.cpp</code> file previously mixed UI rendering, touch event hit-testing, state management, WiFi web server handling, and preference persistence in a single file. Step 1 cleanly decouples the UI layer into <code className="text-amber-300 font-mono">DisplayManager</code> without blocking display updates or altering any pixel coordinates.
              </p>
            </div>

            <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
              <div className="bg-neutral-950 border border-red-900/30 rounded-xl p-5">
                <h3 className="text-sm font-semibold text-red-400 mb-3 flex items-center gap-2">
                  <span>❌ Before: Monolithic main.cpp</span>
                </h3>
                <ul className="text-xs text-neutral-400 space-y-2 list-disc list-inside">
                  <li>Global variables for themes, pages, touch states, and coordinates scattered throughout main file.</li>
                  <li>UI drawing functions directly accessed global state and hardcoded canvas references.</li>
                  <li>Touch handling logic embedded directly in main <code className="text-neutral-300 font-mono">loop()</code>.</li>
                  <li>Adding stepper motor or load cell polling risks blocking frame renders.</li>
                </ul>
              </div>

              <div className="bg-neutral-950 border border-emerald-900/30 rounded-xl p-5">
                <h3 className="text-sm font-semibold text-emerald-400 mb-3 flex items-center gap-2">
                  <span>✅ After: Object-Oriented DisplayManager</span>
                </h3>
                <ul className="text-xs text-neutral-400 space-y-2 list-disc list-inside">
                  <li>Encapsulated <code className="text-emerald-300 font-mono">M5Canvas</code> instance inside <code className="text-emerald-300 font-mono">DisplayManager</code>.</li>
                  <li>Exposes clean methods: <code className="text-emerald-300 font-mono">display.updateTouch()</code> and <code className="text-emerald-300 font-mono">display.renderCurrentPage()</code>.</li>
                  <li>Touch actions return decoupled <code className="text-emerald-300 font-mono">UiEvent</code> codes for non-blocking main loop execution.</li>
                  <li>Ready for safe STEP 2 hardware integration (TMC2209 stepper driver & HX711 scale).</li>
                </ul>
              </div>
            </div>
          </div>
        )}
      </div>
    </div>
  );
}
