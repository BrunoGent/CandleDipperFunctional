#ifndef DISPLAY_MANAGER_H
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
  UI_EVENT_MANUAL_TARE,
  UI_EVENT_MANUAL_HOME,
  UI_EVENT_MANUAL_DIP_BOT
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

  // Internal Drawing Helpers (Exactly from original code)
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

#endif // DISPLAY_MANAGER_H
