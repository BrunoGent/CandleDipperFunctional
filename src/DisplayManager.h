#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <M5Unified.h>
#include "SoundManager.h"

// --- Application States (Screens / Pages) ---
// Top-level screens use a persistent 4-tab bottom bar: [ DIP ] | [ MANUAL ] | [ TOOLS ] | [ SETUP ]
enum AppState {
  PAGE_WEIGHT_DIP,      // Main Tab 1 (DIP): Unified Dipping Launcher (Weight/Dips toggle, Melt Bases checkbox, Slim/Std, Start)
  PAGE_MANUAL,          // Main Tab 2 (MANUAL): Jog Up/Down, Speed -/+, Home, Dip Bot, Scale Tare
  PAGE_JIG_TOOLS,       // Main Tab 3 (TOOLS): Standalone "Melt Candle Bases" & "Clean Whole Jig" actions + Wax Level readout
  PAGE_SETTINGS_HUB,    // Main Tab 4 (SETUP): Top-Level 2-Bucket Selector (1. Recipe Parameters vs. 2. Machine Setup)
  PAGE_RECIPE_HUB,      // Bucket 1 Hub: Recipe / Process Categories (3 full-width cards + right-side ▲/▼ scroll bar)
  PAGE_MACHINE_HUB,     // Bucket 2 Hub: Machine / Hardware Categories (3 full-width cards + right-side ▲/▼ scroll bar)
  PAGE_SETTING_DIPPING, // Recipe Sub-page (3-card scroll): Slim/Std target weights (g) and target dip counts
  PAGE_SETTING_TIMING,  // Recipe Sub-page (3-card scroll): 1st dip hold (s), + dips hold (s), Drip Offset (mm), Drip Time (s)
  PAGE_SETTING_COOLING, // Recipe Sub-page (3 cards): Default Cool (s), 1st Cool (s), Last Cool (s)
  PAGE_SETTING_SPEEDS,  // Recipe Sub-page (3-card scroll): Up speed, Down speed, Immerse speed, Extract speed (mm/s)
  PAGE_SETTING_JIG,     // Machine Sub-page (3-card scroll): Jig Below Sens (mm), Base Melt Time (s), Clean Jig Time (s), Soft Limit (mm)
  PAGE_SETTING_MOTOR,   // Machine Sub-page (3 cards): TMC2209 Driver Mode, Hybrid speed threshold, Motor coil power toggle
  PAGE_SETTING_SENSOR,  // Machine Sub-page (3 cards): Collision limit (g), Weight Dwell (ms), Load Cell Calibration Wizard
  PAGE_SCALE_CAL,       // Machine Sub-page: Guided 2-step Load Cell Calibration Wizard (1. Zero Unloaded, 2. Known Weight)
  PAGE_SETTING_SCREEN,  // Machine Sub-page (2 cards): Screen brightness control and Color Theme selector
  PAGE_SETTING_SOUND,   // Machine Sub-page (3-card scroll): Speaker Volume, Startup Tune, Finish Tune, Alert Tune (live preview)
  PAGE_WIFI_PORTAL,     // Machine Sub-page: SoftAP Captive Portal instructions for Wi-Fi setup
  PAGE_ACTIVE_DIP,      // Modal Screen: Live telemetry while an automated dipping cycle is running
  PAGE_STOP_CONFIRM,    // Modal Screen: Pause / Abort confirmation (or Collision / Low Wax alert after retracting to top rest)
  PAGE_DIP_DONE         // Modal Screen: Process completion or cancellation summary screen
};

// --- Action Events Triggered by UI Touch Interactions ---
// Returned by DisplayManager::updateTouch() so main.cpp can execute hardware actions
enum UiEvent {
  UI_EVENT_NONE,                // No actionable button was pressed this frame
  UI_EVENT_START_DIP,           // User tapped "START DIP" on the Dipping Launcher
  UI_EVENT_ABORT_DIP,           // User confirmed "YES" on abort confirmation screen
  UI_EVENT_FINISH_DIP,          // User tapped "FINISH" or "BACK" on completion screen
  UI_EVENT_START_WIFI_PORTAL,   // User opened the Wi-Fi setup portal from Machine Setup
  UI_EVENT_STOP_WIFI_PORTAL,    // User tapped "< BACK" on the Wi-Fi portal screen
  UI_EVENT_SETTING_UPDATED,     // User saved a numeric parameter (s0..s22) via numpad or cycled Driver Mode
  UI_EVENT_MELT_TOGGLED,        // User checked/unchecked the "[x] MELT BASES" option on the Dipping Launcher
  UI_EVENT_THEME_CHANGED,       // User cycled the UI color theme
  UI_EVENT_BRIGHTNESS_CHANGED,  // User adjusted the screen brightness
  UI_EVENT_VOLUME_CHANGED,      // User adjusted the I2S speaker volume (0..100%)
  UI_EVENT_STARTUP_TUNE_CHANGED,// User cycled Startup Tune (saves to NVS & plays live preview)
  UI_EVENT_FINISH_TUNE_CHANGED, // User cycled Finish Tune (saves to NVS & plays live preview)
  UI_EVENT_ALERT_TUNE_CHANGED,  // User cycled Alert/Error Tune (saves to NVS & plays live preview)
  UI_EVENT_MANUAL_TARE,         // User tapped "TARE" on Manual Control or Scale Calibration screen (RAM-only zero)
  UI_EVENT_CALIBRATE_SCALE,     // User entered a known reference weight (g) in the Scale Calibration Wizard (saved to NVS)
  UI_EVENT_MANUAL_HOME,         // User tapped "HOME" on Manual Control screen
  UI_EVENT_MANUAL_DIP_BOT,      // User tapped "DIP BOT" on Manual Control screen
  UI_EVENT_TOOL_MELT_BASES,     // User tapped "MELT CANDLE BASES" on the Tools screen
  UI_EVENT_TOOL_CLEAN_JIG,      // User tapped "CLEAN WHOLE JIG" on the Tools screen
  UI_EVENT_MANUAL_STOP,         // User tapped "STOP" during Homing, Dip Bot, Melt Bases, or Clean Jig
  UI_EVENT_MOTOR_ENABLE_TOGGLE  // User toggled Motor Power (ENABLED / DISABLED) on Motor Settings screen
};

// --- Display Data Container ---
// Centralized state structure shared between DisplayManager (UI) and main.cpp (Hardware logic)
struct DisplayData {
  // Navigation & Page State
  AppState currentPage = PAGE_WEIGHT_DIP;
  bool pageChanged = true; // Dirty flag: when true, renderCurrentPage() redraws the sprite and pushes to LCD
  int scrollOffset = 0;    // Current top item index (0, 3, ...) for 3-card vertical lists with right-hand ▲/▼ scroll column

  // Manual Control, Tools & Live Telemetry State
  bool limitSwitchOn = false;     // Live debounced state of top limit switch
  bool capSensorOn = false;       // Live debounced state of bottom wax capacitive sensor
  bool isUpPressed = false;       // True while manual UP jog is toggled active
  bool isDownPressed = false;     // True while manual DOWN jog is toggled active
  bool isHomingActive = false;    // True while 4-stage homing sequence is running
  bool isDipBotActive = false;    // True while single-cycle Dip Bot sequence is running
  bool isMeltBasesActive = false; // True while standalone Melt Candle Bases tool is running
  bool isCleanJigActive = false;  // True while standalone Clean Whole Jig tool is running
  bool justStoppedByTouch = false;// Prevents touch-to-stop release event from re-triggering buttons
  int manualSpeed = 50;           // Manual jog speed in mm/s (adjustable via - / + buttons from 5 to 100)
  float currentPosition = 0.0f;   // Current Z-axis carriage position in mm (0 = top home)
  float currentWeight = 0.0f;     // Current load cell reading in grams (tared in RAM)
  float scaleFactor = 420.0f;     // Current HX711 calibration factor (raw counts per gram, persisted in NVS)
  int calibrationRefGrams = 500;  // Reference weight (g) entered by user in the Scale Calibration Wizard
  float lastWaxLevelMM = 0.0f;    // Z position (mm from top home) where capacitive sensor last detected the wax surface

  // Process & Machine Parameters (Persisted in ESP32 NVS Preferences as keys "s0".."s22")
  int s_slimWt = 1500;       // s0: Target weight (g) for Slim profile in Weight mode
  int s_stdWt = 2500;        // s1: Target weight (g) for Standard profile in Weight mode
  int s_slimDips = 15;       // s2: Target dip count for Slim profile in Dips mode
  int s_stdDips = 25;        // s3: Target dip count for Standard profile in Dips mode
  int s_dip1Time = 10;       // s4: Dwell time (seconds) submerged in wax on the 1st dip (wick soaking)
  int s_subDipTime = 4;      // s5: Dwell time (seconds) submerged in wax for dips 2+
  int s_upSpeed = 60;        // s6: Fast open-air raising speed (mm/s) once candle base is clear of wax
  int s_downSpeed = 50;      // s7: Fast open-air lowering speed (mm/s) before candle base touches wax
  int s_colLimit = -50;      // s8: Collision weight limit (g) - retracts to top rest if weight drops by >= |s_colLimit|g (0 = off)
  int s_softLimit = 500;     // s9: Soft Limit Guard in mm (default 500) - max downward travel & low-wax cutoff
  int s_weightDwell = 500;   // s10: Settling delay (ms) at top of stroke before reading scale weight
  int s_coolTime = 15;       // s11: Default cooling rest time (seconds) at top of stroke between dips
  int s_tmcMode = 1;         // s12: TMC2209 chopper mode (0: StealthChop, 1: SpreadCycle, 2: Adaptive)
  int s_tmcThreshold = 30;   // s13: Adaptive Threshold (mm/s) above which driver switches to SpreadCycle
  int s_jigLength = 240;     // s14: Length of jig/candles below the capacitive sensor (mm)
  int s_baseMeltTime = 5;    // s15: Time (seconds) to hold candle bases in hot wax to melt/flatten bottoms
  int s_jigCleanTime = 30;   // s16: Time (seconds) to hold the entire jig submerged to melt and clean off old wax
  int s_coolTime1 = 0;       // s17: 1st dip cooling time (seconds) for tapered cool curve (0 = use Default Cool s11)
  int s_coolTimeEnd = 0;     // s18: Final dip cooling time (seconds) for tapered cool curve (0 = use Default Cool s11)
  int s_immerseSpeed = 25;   // s19: Immersion speed (mm/s) from before candle base touches wax until sensor triggers (0 = s_downSpeed)
  int s_extractSpeed = 20;   // s20: Extraction speed (mm/s) while lifting candles out of wax until base clears (0 = s_upSpeed)
  int s_dripOffset = 15;     // s21: Drip-off pause position relative to wax surface in mm (+ = above wax, - = below, 0 = off)
  int s_dripTime = 3;        // s22: Drip-off pause duration in seconds (0 = disabled)
  bool meltBasesEnabled = false; // Checkbox option: if true, melts candle bases as the final stage of dipping
  bool isMotorEnabled = true; // Motor power enable/disable toggle
  int s_brightness = 50;     // Screen backlight brightness (1 to 99%)
  int s_theme = 1;           // Color theme ID (0: Industrial Slate, 1: Bee Mine, 2: Midnight Workshop)

  // Audio & Melody Configuration (Persisted in ESP32 NVS Preferences)
  int s_volume = 70;         // I2S Speaker Volume (0 to 100%, 0 = Muted)
  int s_startupTune = 1;     // Startup tune ID (0: Off, 1: Standard, 2: Dota 2, 3: SpongeBob, 4: Adv. Time, 5: Classical, 6: Heavy Metal, 7: Mario 1-Up)
  int s_finishTune = 1;      // Process complete tune ID (0: Off, 1: Standard, 2: Dota 2, 3: SpongeBob, 4: Adv. Time, 5: Classical, 6: Heavy Metal, 7: Kitchen Timer, 8: iPhone Ding, 9: Bird Song)
  int s_alertTune = 1;       // Error/Collision/Low-Wax tune ID (0: Off, 1: Standard, 2: Mario Death, 3: Dog Bark, 4: Bird Chirp, 5: Timer Ring, 6: iPhone Ding, 7: Metal Doom, 8: Retro Fail)

  // Numpad Editor State (Modal overlay for editing numeric parameters s0..s22, or 23 for Scale Cal weight)
  bool showNumpad = false;
  int activeSetting = -1;    // Index (0..22 for parameters, 23 for Scale Calibration reference weight)
  String numpadStr = "";     // Digits typed into the numpad so far

  // Active Dipping State
  unsigned long dipStartTime = 0;         // millis() timestamp when the dipping job started
  unsigned long stopConfirmEnterTime = 0; // millis() timestamp when PAGE_STOP_CONFIRM opened (for 500ms touch debounce)
  unsigned long lastUiTick = 0;           // millis() timestamp for 1Hz elapsed timer refresh on screen
  int estimatedTotalSeconds = 0;          // Estimated total run duration (from rolling history or formula)
  int finalElapsedSeconds = 0;            // Total seconds elapsed when dipping finished or was aborted
  bool isActiveWeightBased = true;        // True = Weight Dipping mode, False = Dips Dipping mode
  bool isActiveSlimProfile = true;        // True = Slim candle profile, False = Standard candle profile
  bool dipWasAborted = false;             // True if user cancelled the dipping process early
  bool collisionTriggered = false;        // True if PAGE_STOP_CONFIRM was triggered by load-cell collision detection
  bool lowWaxTriggered = false;           // True if PAGE_STOP_CONFIRM was triggered by reaching s_softLimit without sensing wax
  int currentDipCount = 0;                // Current dip number (1, 2, ... N)
  String currentPhaseText = "Starting...";// Status text shown on active dip screen (e.g., "Dip 1: Lowering...")
  String calStatusText = "Step 1: Unload scale & tap ZERO"; // Status guidance on Scale Calibration page

  // Rotating 5-entry circular buffer history of completion times (in seconds) for 4 process types:
  // Index 0: Slim Weight, 1: Std Weight, 2: Slim Dips, 3: Std Dips
  int processHistory[4][5] = {{0}};
  int historyCount[4] = {0}; // Number of valid entries recorded so far (up to 5)
  int historyHead[4] = {0};  // Next write index (0..4) in the circular buffer

  // Network & Portal State
  String wifiSSID = "";
  bool portalActive = false; // True while SoftAP Wi-Fi configuration portal is running
};

class DisplayManager {
public:
  DisplayManager();

  // Initialization & Setup: allocates the 320x240 16-bit off-screen sprite buffer in RAM
  bool begin(M5GFX* display = &M5.Display);

  // Theme & Screen Control
  void applyTheme(int themeId);
  void setBrightness(int percent);

  // Core Render & Touch Execution
  void renderCurrentPage(); // Redraws off-screen canvas and pushes via DMA if data.pageChanged is true
  UiEvent updateTouch();    // Polls touch screen, updates UI navigation state, and returns hardware events

  // Process Controls & History
  void startDippingProcess(bool isWeight, bool isSlim);
  void endDippingProcess(bool aborted, int elapsedSeconds = 0);
  int getAverageTime(bool isWeight, bool isSlim) const;
  void recordCompletedProcess(bool isWeight, bool isSlim, int seconds);

  // Data Accessors & Mutators
  DisplayData& getData() { return data; }
  const DisplayData& getData() const { return data; }

  AppState getCurrentPage() const { return data.currentPage; }
  void setCurrentPage(AppState page) { data.currentPage = page; data.scrollOffset = 0; data.pageChanged = true; }

  bool isPageChanged() const { return data.pageChanged; }
  void markPageChanged(bool changed = true) { data.pageChanged = changed; }

  // Maps parameter index (0..23) to the corresponding DisplayData struct member
  int getSettingValue(int index) const;
  void setSettingValue(int index, int val);

private:
  M5GFX* _display;    // Pointer to physical M5Stack display
  M5Canvas canvas;    // Off-screen double buffer (320x240) to prevent screen tearing/flicker
  DisplayData data;   // Complete UI & machine state container

  // Active Theme Palette Colors (16-bit RGB565)
  uint16_t c_bg;        // Screen background color
  uint16_t c_banner;    // Top banner & numpad background color
  uint16_t c_bannerTxt; // Top banner text color
  uint16_t c_btn1;      // Primary button fill color
  uint16_t c_btn2;      // Secondary button fill color
  uint16_t c_btnTxt;    // Button label text color
  uint16_t c_active;    // Highlight / active / start button color (green)
  uint16_t c_outline;   // Neutral container / bottom navigation bar color

  // Internal Drawing Helpers
  void drawTopBanner();
  void drawBottomBanner();
  void drawButton(int x, int y, int w, int h, const char* label, uint32_t color, uint32_t txtColor);
  void drawRightScrollBar(int totalItems);
  void drawCardWithHint(int y, int w, const char* title, const char* hint, const char* badge, uint16_t cardBg, uint16_t titleColor, uint16_t badgeBg, uint16_t badgeTxt);

  void drawDipLauncherPage();
  void drawManualPage();
  void drawJigToolsPage();
  void drawSettingsHub();
  void drawRecipeHub();
  void drawMachineHub();
  void drawMotorPage();
  void drawSensorAndScalePage();
  void drawScaleCalPage();
  void drawSettingsList(const int* indices, int count);
  void drawScreenDashboard();
  void drawSoundPage();
  void drawWifiPortalPage();
  void drawNumpad();
  void drawActiveDipPage();
  void drawStopConfirmPage();
  void drawDipDonePage();

  // Text & Label Utilities
  void formatTime(int totalSeconds, char* buffer, int bufferSize) const;
};

#endif // DISPLAY_MANAGER_H



