#include "DisplayManager.h"

// Parameter metadata arrays (indices 0..22 map to preferences keys "s0".."s22"; index 23 is Scale Calibration weight)
static const char* settingNames[24] = {
  "Slim Weight", "Std Weight", "Slim Dips", "Std Dips",
  "1st Dip Time", "+ Dips Time", "Up Speed", "Down Speed",
  "Col. Limit", "Soft Limit", "Weight Dwell", "Default Cool",
  "Driver Mode", "Hybrid Speed",
  "Jig Below Sens", "Base Melt Time", "Clean Jig Time",
  "1st Cool Time", "Last Cool Time",
  "Immerse Speed", "Extract Speed",
  "Drip Offset", "Drip Time",
  "Known Weight (g)"
};

// Helpful 1-line subtitle hints displayed inside each 50px parameter card
static const char* settingHints[24] = {
  "Target wax weight (Slim)",    // s0
  "Target wax weight (Std)",     // s1
  "Total dip cycles (Slim)",     // s2
  "Total dip cycles (Std)",      // s3
  "Wick soak hold on Dip 1",     // s4
  "Submerged hold on Dips 2+",   // s5
  "Fast lift in open air",       // s6
  "Fast lower in open air",      // s7
  "Upward push safety trip",     // s8
  "Max Z travel & Low Wax",      // s9
  "Scale settle wait at top",    // s10
  "Rest time at top of stroke",  // s11
  "Stealth / Spread / Adapt",    // s12
  "Adaptive mode threshold",     // s13
  "Sensor to candle base",       // s14
  "Bottom flatten melt hold",    // s15
  "Full basket wax-off soak",    // s16
  "Dip 1 cool (0 = Default)",    // s17
  "End cool (0 = Default)",      // s18
  "Slows 15mm before wax",       // s19
  "Slow lift while in wax",      // s20
  "+ above / - in wax (0=Off)",  // s21
  "Drip pause hold (0=Off)",     // s22
  "Calibration test mass"        // s23
};

static const char* settingUnits[24] = {
  "g", "g", "dips", "dips",
  "s", "s", "mm/s", "mm/s",
  "g", "mm", "ms", "s",
  "", "mm/s",
  "mm", "s", "s",
  "s", "s",
  "mm/s", "mm/s",
  "mm", "s",
  "g"
};
static const char* themeNames[3] = { "Ind. Slate", "Bee Mine", "Midnight" };

// Parameter index lists for the scrollable 3-card sub-pages
static const int dippingSettingIndices[4] = { 0, 1, 2, 3 };   // Slim Wt, Std Wt, Slim Dips, Std Dips
static const int timingSettingIndices[4]  = { 4, 5, 21, 22 }; // 1st Dip Time, + Dips Time, Drip Offset (mm), Drip Time (s)
static const int coolingSettingIndices[3] = { 11, 17, 18 };   // Default Cool (s), 1st Cool (s), Last Cool (s)
static const int speedsSettingIndices[4]  = { 6, 7, 19, 20 }; // Up Speed, Down Speed, Immerse Speed, Extract Speed
static const int jigSettingIndices[4]     = { 14, 15, 16, 9 };// Jig Below Sens (mm), Base Melt Time (s), Clean Jig Time (s), Soft Limit (mm)

DisplayManager::DisplayManager() : _display(&M5.Display), canvas(&M5.Display) {
  // Constructor binds M5Canvas off-screen sprite to M5.Display
}

// Allocates the full-screen 320x240 sprite buffer in RAM and applies initial theme & brightness
bool DisplayManager::begin(M5GFX* display) {
  if (display) {
    _display = display;
  }
  
  if (!canvas.createSprite(_display->width(), _display->height())) {
    Serial.println("Canvas creation failed!");
    return false;
  }

  _display->setBrightness(map(data.s_brightness, 1, 99, 10, 255));
  applyTheme(data.s_theme);
  data.pageChanged = true;
  return true;
}

// Updates the 16-bit RGB565 color palette used by all drawing methods:
// - Theme 0 ("Ind. Slate"): Refined Industrial Slate & Teal (replaces harsh 90s VGA Navy/Maroon on pitch black)
// - Theme 1 ("Bee Mine"): Artisanal Honeycomb Gold, Warm Espresso & Botanical Sage (distinct banner vs card hierarchy)
// - Theme 2 ("Midnight"): High-Contrast Workshop OLED Dark with distinct Indigo primary & Teal-Slate secondary cards
void DisplayManager::applyTheme(int themeId) {
  data.s_theme = themeId % 3;
  if (data.s_theme == 0) { // Theme 0: Industrial Slate & Terracotta
    c_bg        = canvas.color565(13, 19, 26);    // Deep Midnight Navy (#0D131A)
    c_banner    = canvas.color565(15, 118, 110);  // Rich Industrial Teal (#0F766E)
    c_bannerTxt = TFT_WHITE;
    c_btn1      = canvas.color565(30, 64, 124);   // Steel Royal Blue (#1E407C)
    c_btn2      = canvas.color565(127, 45, 58);   // Warm Terracotta Burgundy (#7F2D3A)
    c_btnTxt    = TFT_WHITE;
    c_active    = canvas.color565(16, 185, 129);  // Vibrant Emerald (#10B981)
    c_outline   = canvas.color565(51, 65, 85);    // Cool Slate (#334155)
  } else if (data.s_theme == 1) { // Theme 1: Bee Mine Artisanal Honeycomb
    c_bg        = canvas.color565(28, 22, 18);    // Warm Deep Espresso (#1C1612)
    c_banner    = canvas.color565(245, 176, 35);  // Rich Golden Honeycomb (#F5B023)
    c_bannerTxt = canvas.color565(24, 18, 12);    // Deep Espresso text
    c_btn1      = canvas.color565(252, 211, 77);  // Warm Beeswax Gold (#FCD34D)
    c_btn2      = canvas.color565(110, 205, 165); // Botanical Meadow Sage (#6ECDA5)
    c_btnTxt    = canvas.color565(24, 18, 12);    // Deep Espresso text
    c_active    = canvas.color565(74, 222, 128);  // Fresh Clover Green (#4ADE80)
    c_outline   = canvas.color565(72, 60, 50);    // Warm Umber Slate (#483C32)
  } else { // Theme 2: Midnight Workshop OLED
    c_bg        = canvas.color565(10, 14, 20);    // Deep Obsidian (#0A0E14)
    c_banner    = canvas.color565(30, 41, 59);    // Elevated Charcoal Slate (#1E293B)
    c_bannerTxt = canvas.color565(226, 232, 240); // Crisp Silver-White (#E2E8F0)
    c_btn1      = canvas.color565(37, 56, 88);    // Gunmetal Indigo (#253858)
    c_btn2      = canvas.color565(31, 68, 76);    // Deep Teal-Slate (#1F444C)
    c_btnTxt    = TFT_WHITE;
    c_active    = canvas.color565(34, 197, 94);   // High-Contrast Emerald (#22C55E)
    c_outline   = canvas.color565(64, 78, 98);    // Muted Steel (#404E62)
  }
}

// Maps user percentage (1..99%) to hardware PWM duty cycle (10..255)
void DisplayManager::setBrightness(int percent) {
  data.s_brightness = constrain(percent, 1, 99);
  _display->setBrightness(map(data.s_brightness, 1, 99, 10, 255));
}

// Reads a parameter value by numeric index (0..23) for generic list rendering and NVS saving
int DisplayManager::getSettingValue(int index) const {
  switch(index) {
     case 0: return data.s_slimWt; 
     case 1: return data.s_stdWt; 
     case 2: return data.s_slimDips; 
     case 3: return data.s_stdDips;
     case 4: return data.s_dip1Time; 
     case 5: return data.s_subDipTime; 
     case 6: return data.s_upSpeed; 
     case 7: return data.s_downSpeed;
     case 8: return data.s_colLimit;
     case 9: return data.s_softLimit;
     case 10: return data.s_weightDwell;
     case 11: return data.s_coolTime;
     case 12: return data.s_tmcMode;
     case 13: return data.s_tmcThreshold;
     case 14: return data.s_jigLength;
     case 15: return data.s_baseMeltTime;
     case 16: return data.s_jigCleanTime;
     case 17: return data.s_coolTime1;
     case 18: return data.s_coolTimeEnd;
     case 19: return data.s_immerseSpeed;
     case 20: return data.s_extractSpeed;
     case 21: return data.s_dripOffset;
     case 22: return data.s_dripTime;
     case 23: return data.calibrationRefGrams;
  }
  return 0;
}

// Writes a parameter value by numeric index (0..23) when the user presses "OK" on the numpad
void DisplayManager::setSettingValue(int index, int val) {
  switch(index) {
     case 0: data.s_slimWt = max(1, abs(val)); break;
     case 1: data.s_stdWt = max(1, abs(val)); break;
     case 2: data.s_slimDips = max(1, abs(val)); break;
     case 3: data.s_stdDips = max(1, abs(val)); break;
     case 4: data.s_dip1Time = abs(val); break;
     case 5: data.s_subDipTime = abs(val); break;
     case 6: data.s_upSpeed = constrain(abs(val), 1, 150); break;
     case 7: data.s_downSpeed = constrain(abs(val), 1, 150); break;
     case 8: data.s_colLimit = (val == 0) ? 0 : -abs(val); break; // Negative gram threshold for upward collision push (0 = disabled)
     case 9: data.s_softLimit = abs(val); break;
     case 10: data.s_weightDwell = abs(val); break;
     case 11: data.s_coolTime = abs(val); break;
     case 12: data.s_tmcMode = (val < 0) ? 0 : (val % 3); break;
     case 13: data.s_tmcThreshold = abs(val); break;
     case 14: data.s_jigLength = abs(val); break;
     case 15: data.s_baseMeltTime = abs(val); break;
     case 16: data.s_jigCleanTime = abs(val); break;
     case 17: data.s_coolTime1 = abs(val); break;   // 0 = use s_coolTime
     case 18: data.s_coolTimeEnd = abs(val); break; // 0 = use s_coolTime
     case 19: data.s_immerseSpeed = constrain(abs(val), 0, 150); break; // 0 = use s_downSpeed
     case 20: data.s_extractSpeed = constrain(abs(val), 0, 150); break; // 0 = use s_upSpeed
     case 21: data.s_dripOffset = constrain(val, -200, 200); break;     // Positive = above wax, Negative = below, 0 = off
     case 22: data.s_dripTime = abs(val); break;                        // 0 = disabled
     case 23: data.calibrationRefGrams = max(1, abs(val)); break;
  }
}

// Formats seconds as "MM:SS" (e.g., 135 -> "02:15")
void DisplayManager::formatTime(int totalSeconds, char* buffer, int bufferSize) const {
  int m = totalSeconds / 60;
  int s = totalSeconds % 60;
  snprintf(buffer, bufferSize, "%02d:%02d", m, s);
}

// Helper to draw a rounded rectangle button with centered text
void DisplayManager::drawButton(int x, int y, int w, int h, const char* label, uint32_t color, uint32_t txtColor) {
  canvas.fillRoundRect(x, y, w, h, 8, color);
  canvas.setTextColor(txtColor, color);
  canvas.setTextDatum(middle_center);
  canvas.drawString(label, x + (w / 2), y + (h / 2));
}

// Draws the 45px-wide right-hand scroll column (x = 269..314, y = 40..200) when a list has > 3 items:
// - Top Button (y = 40..84):     Up arrow triangle ▲
// - Middle Track (y = 88..152):  Proportional scroll thumb + "1/2" page indicator
// - Bottom Button (y = 156..200): Down arrow triangle ▼
void DisplayManager::drawRightScrollBar(int totalItems) {
  if (totalItems <= 3) return;

  int maxOffset = ((totalItems - 1) / 3) * 3;
  bool canUp = (data.scrollOffset > 0);
  bool canDown = (data.scrollOffset < maxOffset);

  // 1. Up Button (x = 269..314, y = 40..84)
  uint16_t upBg = canUp ? c_btn1 : c_outline;
  uint16_t upFg = canUp ? c_btnTxt : canvas.color565(120, 130, 140);
  canvas.fillRoundRect(269, 40, 45, 44, 7, upBg);
  canvas.fillTriangle(291, 52, 279, 71, 303, 71, upFg);

  // 2. Middle Scroll Track & Page Indicator (x = 269..314, y = 88..152)
  canvas.fillRoundRect(269, 88, 45, 64, 7, c_outline);
  int totalPages = (totalItems + 2) / 3;
  int curPage = (data.scrollOffset / 3) + 1;
  // Proportional vertical thumb along the right edge of the track
  int trackH = 56;
  int thumbH = max(18, trackH / totalPages);
  int thumbY = 92 + ((trackH - thumbH) * (curPage - 1)) / max(1, totalPages - 1);
  canvas.fillRoundRect(307, thumbY, 4, thumbH, 2, c_active);

  char pgStr[12];
  snprintf(pgStr, sizeof(pgStr), "%d/%d", curPage, totalPages);
  canvas.setTextSize(1.4);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(TFT_WHITE, c_outline);
  canvas.drawString(pgStr, 289, 120);
  canvas.setTextSize(2.0);

  // 3. Down Button (x = 269..314, y = 156..200)
  uint16_t dnBg = canDown ? c_btn1 : c_outline;
  uint16_t dnFg = canDown ? c_btnTxt : canvas.color565(120, 130, 140);
  canvas.fillRoundRect(269, 156, 45, 44, 7, dnBg);
  canvas.fillTriangle(279, 169, 303, 169, 291, 188, dnFg);
}

// Draws a single 50px-tall full-width card (at x = 8, y = 40 / 95 / 150) containing:
// - Bold Title on top-left
// - Subtle 1-line Hint on bottom-left
// - Right-aligned Value / Action Pill
void DisplayManager::drawCardWithHint(int y, int w, const char* title, const char* hint, const char* badge,
                                      uint16_t cardBg, uint16_t titleColor, uint16_t badgeBg, uint16_t badgeTxt) {
  canvas.fillRoundRect(8, y, w, 50, 8, cardBg);

  // Top-left Title
  canvas.setTextDatum(middle_left);
  canvas.setTextSize(1.55);
  canvas.setTextColor(titleColor, cardBg);
  canvas.drawString(title, 16, y + 16);

  // Bottom-left Hint
  canvas.setTextSize(1.1);
  uint16_t hintColor = (titleColor == TFT_WHITE)
                       ? canvas.color565(190, 205, 220)
                       : canvas.color565(55, 45, 35);
  canvas.setTextColor(hintColor, cardBg);
  canvas.drawString(hint, 16, y + 37);

  // Right-side Value / Action Pill
  if (badge && badge[0] != '\0') {
    int pillW = 88;
    int pillX = 8 + w - pillW - 6;
    canvas.fillRoundRect(pillX, y + 7, pillW, 36, 6, badgeBg);
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(1.45);
    canvas.setTextColor(badgeTxt, badgeBg);
    canvas.drawString(badge, pillX + (pillW / 2), y + 25);
  }
  canvas.setTextSize(2.0);
}

// Draws the top 15% status bar (36px tall on 240px screen)
void DisplayManager::drawTopBanner() {
  int bannerHeight = 36;
  canvas.fillRect(0, 0, canvas.width(), bannerHeight, c_banner);
  canvas.setTextColor(c_bannerTxt, c_banner);
  canvas.setTextSize(2.0);

  bool lowWaxWarning = (data.lastWaxLevelMM > 0.0f && data.s_softLimit > 0 &&
                        data.lastWaxLevelMM >= (float)(data.s_softLimit - 20));

  if (data.currentPage == PAGE_MANUAL || data.currentPage == PAGE_SCALE_CAL) {
      char posStr[32]; char wtStr[32];
      snprintf(posStr, sizeof(posStr), "POS: %d mm", (int)data.currentPosition);
      if (fabsf(data.currentWeight) < 1000.0f) {
        snprintf(wtStr, sizeof(wtStr), "WT: %.1f g", data.currentWeight);
      } else {
        snprintf(wtStr, sizeof(wtStr), "WT: %d g", (int)roundf(data.currentWeight));
      }
      canvas.setTextDatum(middle_left); 
      canvas.drawString(posStr, 10, bannerHeight / 2);
      canvas.setTextDatum(middle_right); 
      canvas.drawString(wtStr, canvas.width() - 10, bannerHeight / 2);

      if (data.currentPage == PAGE_MANUAL) {
        canvas.setTextDatum(middle_center);
        canvas.setTextSize(1.5);
        if (data.limitSwitchOn) {
          canvas.setTextColor(canvas.color565(255, 60, 60), c_banner);
          canvas.drawString("TOP: HIT", 160, bannerHeight / 2);
        } else if (data.capSensorOn) {
          canvas.setTextColor(canvas.color565(255, 180, 0), c_banner);
          canvas.drawString("WAX: HIT", 160, bannerHeight / 2);
        } else {
          canvas.setTextColor(canvas.color565(40, 160, 40), c_banner);
          canvas.drawString("TOP: OPEN", 160, bannerHeight / 2);
        }
        canvas.setTextSize(2.0);
      }
  } else if (data.currentPage == PAGE_JIG_TOOLS) {
      canvas.setTextSize(1.6);
      char waxStr[32]; char baseStr[32];
      if (data.lastWaxLevelMM > 0.0f) {
        int baseMM = max(0, (int)roundf(data.lastWaxLevelMM - (float)data.s_jigLength));
        snprintf(waxStr, sizeof(waxStr), lowWaxWarning ? "LOW WAX:%dmm" : "WAX: %dmm", (int)roundf(data.lastWaxLevelMM));
        snprintf(baseStr, sizeof(baseStr), "BASE: %dmm", baseMM);
      } else {
        snprintf(waxStr, sizeof(waxStr), "WAX: UNSET");
        snprintf(baseStr, sizeof(baseStr), "JIG: %dmm", data.s_jigLength);
      }
      canvas.setTextDatum(middle_left);
      canvas.drawString(waxStr, 10, bannerHeight / 2);
      canvas.setTextDatum(middle_right);
      canvas.drawString(baseStr, canvas.width() - 10, bannerHeight / 2);
      canvas.setTextSize(2.0);
  } else if (data.currentPage == PAGE_SETTINGS_HUB) {
      canvas.setTextSize(1.7);
      canvas.setTextDatum(middle_center);
      canvas.drawString("SETUP: SELECT BUCKET", 160, bannerHeight / 2);
      canvas.setTextSize(2.0);
  } else if (data.currentPage == PAGE_RECIPE_HUB || data.currentPage == PAGE_MACHINE_HUB) {
      canvas.setTextSize(1.6);
      canvas.setTextDatum(middle_left);
      canvas.drawString(data.currentPage == PAGE_RECIPE_HUB ? "1. RECIPE PARAMS" : "2. MACHINE SETUP", 10, bannerHeight / 2);
      drawButton(208, 4, 104, 28, data.currentPage == PAGE_RECIPE_HUB ? "MACHINE >" : "< RECIPE", c_outline, TFT_WHITE);
      canvas.setTextSize(2.0);
  } else if (!data.showNumpad && (data.currentPage == PAGE_SETTING_DIPPING ||
                                  data.currentPage == PAGE_SETTING_SPEEDS ||
                                  data.currentPage == PAGE_SETTING_TIMING ||
                                  data.currentPage == PAGE_SETTING_COOLING ||
                                  data.currentPage == PAGE_SETTING_JIG ||
                                  data.currentPage == PAGE_SETTING_SENSOR ||
                                  data.currentPage == PAGE_SETTING_MOTOR ||
                                  data.currentPage == PAGE_SETTING_SCREEN ||
                                  data.currentPage == PAGE_SETTING_SOUND)) {
      canvas.setTextSize(1.7);
      canvas.setTextDatum(middle_center);
      const char* hdr = "PARAMETERS";
      if (data.currentPage == PAGE_SETTING_DIPPING)      hdr = "RECIPE: TARGETS";
      else if (data.currentPage == PAGE_SETTING_SPEEDS)  hdr = "RECIPE: SPEEDS";
      else if (data.currentPage == PAGE_SETTING_TIMING)  hdr = "RECIPE: DIP & DRIP";
      else if (data.currentPage == PAGE_SETTING_COOLING) hdr = "RECIPE: COOLING";
      else if (data.currentPage == PAGE_SETTING_JIG)     hdr = "MACHINE: JIG & LIMIT";
      else if (data.currentPage == PAGE_SETTING_SENSOR)  hdr = "MACHINE: SCALE & COL";
      else if (data.currentPage == PAGE_SETTING_MOTOR)   hdr = "MACHINE: MOTOR";
      else if (data.currentPage == PAGE_SETTING_SCREEN)  hdr = "MACHINE: SCREEN";
      else if (data.currentPage == PAGE_SETTING_SOUND)   hdr = "MACHINE: SOUND & TUNES";
      canvas.drawString(hdr, 160, bannerHeight / 2);
      canvas.setTextSize(2.0);
  } else if (data.currentPage == PAGE_ACTIVE_DIP || data.currentPage == PAGE_STOP_CONFIRM || data.currentPage == PAGE_DIP_DONE) {
      canvas.setTextDatum(middle_center);
      if (data.currentPage == PAGE_ACTIVE_DIP) canvas.drawString("DIPPING IN PROGRESS", 160, bannerHeight / 2);
      else if (data.currentPage == PAGE_STOP_CONFIRM) {
        if (data.lowWaxTriggered)        canvas.drawString("LOW WAX ALERT", 160, bannerHeight / 2);
        else if (data.collisionTriggered) canvas.drawString("COLLISION ALERT", 160, bannerHeight / 2);
        else                             canvas.drawString("CONFIRM ABORT", 160, bannerHeight / 2);
      }
      else canvas.drawString(data.dipWasAborted ? "PROCESS ABORTED" : "PROCESS COMPLETE", 160, bannerHeight / 2);
  } else {
      m5::rtc_datetime_t dt;
      M5.Rtc.getDateTime(&dt);
      
      if (dt.date.year < 2024) {
          dt.date.year = 2026; dt.date.month = 1; dt.date.date = 1; dt.date.weekDay = 4;
      }
      
      const char* shortDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
      int wd = (dt.date.weekDay >= 0 && dt.date.weekDay <= 6) ? dt.date.weekDay : 0;
      char dateStr[32]; char timeStr[32];
      
      if (lowWaxWarning && data.currentPage == PAGE_WEIGHT_DIP) {
        snprintf(dateStr, sizeof(dateStr), "! LOW WAX (%dmm)", (int)roundf(data.lastWaxLevelMM));
      } else {
        snprintf(dateStr, sizeof(dateStr), "%s %02d/%02d/%02d", shortDays[wd], dt.date.date, dt.date.month, dt.date.year % 100);
      }
      snprintf(timeStr, sizeof(timeStr), "%02d:%02d", dt.time.hours, dt.time.minutes);
      
      canvas.setTextDatum(middle_left); canvas.drawString(dateStr, 10, bannerHeight / 2);
      canvas.setTextDatum(middle_right); canvas.drawString(timeStr, 310, bannerHeight / 2);
  }
}

// Draws the bottom 15% navigation bar (y = 204..240):
// - On the 4 main screens (DIP, MANUAL, TOOLS, SETUP): renders a persistent 4-tab bar so any section is 1 tap away!
// - On Recipe / Machine sub-pages: renders a clear "< BACK TO ..." button.
void DisplayManager::drawBottomBanner() {
  if (data.currentPage == PAGE_ACTIVE_DIP || data.currentPage == PAGE_STOP_CONFIRM || data.currentPage == PAGE_DIP_DONE) return;
  int bannerHeight = 36;
  int startY = canvas.height() - bannerHeight; // y = 204

  bool isMainTab = (data.currentPage == PAGE_WEIGHT_DIP ||
                    data.currentPage == PAGE_MANUAL ||
                    data.currentPage == PAGE_JIG_TOOLS ||
                    data.currentPage == PAGE_SETTINGS_HUB);

  if (isMainTab) {
    canvas.fillRect(0, startY, canvas.width(), bannerHeight, c_bg);
    canvas.setTextSize(1.5);

    // Tab 1: DIP (x = 2..79)
    bool dipActive = (data.currentPage == PAGE_WEIGHT_DIP);
    drawButton(2, startY + 2, 77, bannerHeight - 4, "DIP",
               dipActive ? c_banner : c_outline,
               dipActive ? c_bannerTxt : TFT_WHITE);

    // Tab 2: MANUAL (x = 82..159)
    bool manActive = (data.currentPage == PAGE_MANUAL);
    drawButton(82, startY + 2, 77, bannerHeight - 4, "MANUAL",
               manActive ? c_banner : c_outline,
               manActive ? c_bannerTxt : TFT_WHITE);

    // Tab 3: TOOLS (x = 162..239)
    bool toolsActive = (data.currentPage == PAGE_JIG_TOOLS);
    drawButton(162, startY + 2, 77, bannerHeight - 4, "TOOLS",
               toolsActive ? c_banner : c_outline,
               toolsActive ? c_bannerTxt : TFT_WHITE);

    // Tab 4: SETUP (x = 242..318)
    bool setActive = (data.currentPage == PAGE_SETTINGS_HUB);
    drawButton(242, startY + 2, 76, bannerHeight - 4, "SETUP",
               setActive ? c_banner : c_outline,
               setActive ? c_bannerTxt : TFT_WHITE);

    canvas.setTextSize(2.0);
  } else {
    canvas.fillRect(0, startY, canvas.width(), bannerHeight, c_outline);
    canvas.setTextColor(TFT_WHITE, c_outline);
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(1.7);
    if (data.currentPage == PAGE_SCALE_CAL) {
      canvas.drawString("< BACK TO SCALE & COL", 160, startY + (bannerHeight / 2));
    } else if (data.currentPage == PAGE_RECIPE_HUB || data.currentPage == PAGE_MACHINE_HUB) {
      canvas.drawString("< BACK TO SETUP BUCKETS", 160, startY + (bannerHeight / 2));
    } else if (data.currentPage == PAGE_SETTING_DIPPING ||
               data.currentPage == PAGE_SETTING_SPEEDS ||
               data.currentPage == PAGE_SETTING_TIMING ||
               data.currentPage == PAGE_SETTING_COOLING) {
      canvas.drawString("< BACK TO RECIPE PARAMS", 160, startY + (bannerHeight / 2));
    } else {
      canvas.drawString("< BACK TO MACHINE SETUP", 160, startY + (bannerHeight / 2));
    }
    canvas.setTextSize(2.0);
  }
}

// Draws the unified 1-screen Dipping Launcher (PAGE_WEIGHT_DIP):
// - Top toggle row (y = 41..71):
//     Left pill (w=145):  [ MODE: WEIGHT ] / [ MODE: DIPS ] (tap to switch mode)
//     Right pill (w=145): [x] MELT BASES / [ ] MELT BASES (checkbox to run base melting after final dip)
// - Middle row (y = 76..144): Large [ SLIM ] and [ STANDARD ] profile cards (145x68px) with Target & Est. Time
// - Bottom action button (y = 150..198): Full-width [ START DIP ] (300x48px)
void DisplayManager::drawDipLauncherPage() {
  canvas.fillScreen(c_bg);
  drawTopBanner();
  drawBottomBanner();

  // 1. Top Control Row: Mode Toggle (left) & Base Melt Checkbox (right)
  canvas.setTextSize(1.5);
  bool isWt = data.isActiveWeightBased;
  drawButton(10, 41, 145, 30, isWt ? "MODE: WEIGHT" : "MODE: DIPS", c_btn1, c_btnTxt);

  bool meltOn = data.meltBasesEnabled;
  drawButton(165, 41, 145, 30, meltOn ? "[x] MELT BASES" : "[ ] MELT BASES",
             meltOn ? c_active : c_outline,
             meltOn ? TFT_BLACK : TFT_WHITE);

  // 2. Profile Cards Row (SLIM on left, STANDARD on right)
  bool isSlim = data.isActiveSlimProfile;
  uint16_t slimFill = isSlim ? c_btn2 : c_outline;
  uint16_t slimTxt  = isSlim ? c_btnTxt : TFT_LIGHTGREY;
  uint16_t stdFill  = !isSlim ? c_btn2 : c_outline;
  uint16_t stdTxt   = !isSlim ? c_btnTxt : TFT_LIGHTGREY;

  // Slim Card (x = 10..155, y = 76..144)
  canvas.fillRoundRect(10, 76, 145, 68, 8, slimFill);
  if (isSlim) canvas.drawRoundRect(10, 76, 145, 68, 8, TFT_WHITE);
  canvas.setTextColor(slimTxt, slimFill);
  canvas.setTextDatum(middle_center);
  canvas.setTextSize(1.8);
  canvas.drawString("SLIM", 82, 92);

  canvas.setTextSize(1.4);
  char slimTarget[32];
  if (isWt) snprintf(slimTarget, sizeof(slimTarget), "Target: %d g", data.s_slimWt);
  else      snprintf(slimTarget, sizeof(slimTarget), "Target: %d dips", data.s_slimDips);
  canvas.drawString(slimTarget, 82, 113);

  int estSlim = getAverageTime(isWt, true);
  if (estSlim <= 0) estSlim = data.s_slimDips * (data.s_dip1Time + data.s_coolTime + 6);
  char estSlimStr[16]; formatTime(estSlim, estSlimStr, sizeof(estSlimStr));
  char slimEstBuf[32]; snprintf(slimEstBuf, sizeof(slimEstBuf), "Est: %s", estSlimStr);
  canvas.drawString(slimEstBuf, 82, 131);

  // Standard Card (x = 165..310, y = 76..144)
  canvas.fillRoundRect(165, 76, 145, 68, 8, stdFill);
  if (!isSlim) canvas.drawRoundRect(165, 76, 145, 68, 8, TFT_WHITE);
  canvas.setTextColor(stdTxt, stdFill);
  canvas.setTextDatum(middle_center);
  canvas.setTextSize(1.8);
  canvas.drawString("STANDARD", 237, 92);

  canvas.setTextSize(1.4);
  char stdTarget[32];
  if (isWt) snprintf(stdTarget, sizeof(stdTarget), "Target: %d g", data.s_stdWt);
  else      snprintf(stdTarget, sizeof(stdTarget), "Target: %d dips", data.s_stdDips);
  canvas.drawString(stdTarget, 237, 113);

  int estStd = getAverageTime(isWt, false);
  if (estStd <= 0) estStd = data.s_stdDips * (data.s_dip1Time + data.s_coolTime + 6);
  char estStdStr[16]; formatTime(estStd, estStdStr, sizeof(estStdStr));
  char stdEstBuf[32]; snprintf(stdEstBuf, sizeof(stdEstBuf), "Est: %s", estStdStr);
  canvas.drawString(stdEstBuf, 237, 131);

  // 3. Large START DIP Button (x = 10..310, y = 150..198)
  canvas.setTextSize(2.0);
  drawButton(10, 150, 300, 48, "START DIP", c_active, TFT_BLACK);
}

// Draws the Manual Control page:
// - Left column: UP triangle button, "- / speed / +" speed adjuster, DOWN triangle button
// - Right column: HOME (or STOP), DIP BOT (or STOP), and TARE buttons
void DisplayManager::drawManualPage() {
  canvas.fillScreen(c_bg);
  drawTopBanner(); drawBottomBanner();
  
  if (data.isHomingActive) {
    drawButton(197, 41, 118, 46, "STOP", canvas.color565(255, 0, 0), TFT_WHITE);
  } else {
    drawButton(197, 41, 118, 46, "HOME", data.limitSwitchOn ? c_active : c_outline, data.limitSwitchOn ? TFT_BLACK : TFT_WHITE);
  }

  if (data.isDipBotActive) {
    drawButton(197, 97, 118, 46, "STOP", canvas.color565(255, 0, 0), TFT_WHITE);
  } else {
    drawButton(197, 97, 118, 46, "DIP BOT", data.capSensorOn ? c_active : c_outline, data.capSensorOn ? TFT_BLACK : TFT_WHITE);
  }

  drawButton(197, 153, 118, 46, "TARE", TFT_ORANGE, TFT_BLACK);

  canvas.fillRoundRect(36, 41, 120, 50, 10, data.isUpPressed ? c_active : c_btn1);
  canvas.fillTriangle(96, 51, 66, 81, 126, 81, data.isUpPressed ? TFT_BLACK : c_btnTxt);
  canvas.fillRoundRect(36, 149, 120, 50, 10, data.isDownPressed ? c_active : c_btn1);
  canvas.fillTriangle(66, 159, 126, 159, 96, 189, data.isDownPressed ? TFT_BLACK : c_btnTxt);

  drawButton(10, 100, 45, 40, "-", c_outline, TFT_WHITE);
  drawButton(137, 100, 45, 40, "+", c_outline, TFT_WHITE);

  canvas.setTextColor(c_btnTxt, c_bg);
  canvas.setTextDatum(middle_center);
  char speedStr[16]; snprintf(speedStr, sizeof(speedStr), "%d", data.manualSpeed);
  canvas.drawString(speedStr, 96, 120);
}

// Draws the Jig & Wax Finishing Tools page (PAGE_JIG_TOOLS):
// - Button 1 (y = 42..92):  MELT CANDLE BASES (lowers to lastWaxLevelMM - s_jigLength for s_baseMeltTime s)
// - Button 2 (y = 98..148): CLEAN WHOLE JIG (lowers to capacitive sensor limit for s_jigCleanTime s)
// - Button 3 (y = 154..198): JIG & MELT SETTINGS > (opens PAGE_SETTING_JIG to adjust Jig Length & Melt Times)
void DisplayManager::drawJigToolsPage() {
  canvas.fillScreen(c_bg);
  drawTopBanner();
  drawBottomBanner();

  canvas.setTextSize(1.7);

  // Button 1: Melt Candle Bases (50px tall)
  if (data.isMeltBasesActive) {
    drawButton(10, 42, 300, 50, "STOP BASE MELT", canvas.color565(255, 0, 0), TFT_WHITE);
  } else {
    char meltLbl[40];
    snprintf(meltLbl, sizeof(meltLbl), "MELT CANDLE BASES (%ds)", data.s_baseMeltTime);
    drawButton(10, 42, 300, 50, meltLbl, c_btn1, c_btnTxt);
  }

  // Button 2: Clean Whole Jig (50px tall)
  if (data.isCleanJigActive) {
    drawButton(10, 98, 300, 50, "STOP JIG CLEAN", canvas.color565(255, 0, 0), TFT_WHITE);
  } else {
    char cleanLbl[40];
    snprintf(cleanLbl, sizeof(cleanLbl), "CLEAN WHOLE JIG (%ds)", data.s_jigCleanTime);
    drawButton(10, 98, 300, 50, cleanLbl, c_btn2, c_btnTxt);
  }

  // Button 3: Shortcut to Jig & Melt Settings (44px tall)
  canvas.setTextSize(1.6);
  char cfgLbl[48];
  snprintf(cfgLbl, sizeof(cfgLbl), "JIG LEN (%dmm) & TIMES >", data.s_jigLength);
  drawButton(10, 154, 300, 44, cfgLbl, c_outline, TFT_WHITE);
  canvas.setTextSize(2.0);
}

// Draws Top-Level SETUP Hub (PAGE_SETTINGS_HUB):
// Splits all settings into two unmistakable full-width buckets (74px tall each):
// 1. RECIPE PARAMETERS (Targets, Speeds, Dip/Drip Timing, Cooling Curve)
// 2. MACHINE SETUP (Jig & Soft Limit, Scale & Collision, Stepper Motor, Screen, Wi-Fi)
void DisplayManager::drawSettingsHub() {
  canvas.fillScreen(c_bg);
  drawTopBanner(); drawBottomBanner();

  // Bucket 1 Card (x = 10..310, y = 42..116, h = 74)
  canvas.fillRoundRect(10, 42, 300, 74, 10, c_btn1);
  canvas.setTextDatum(middle_left);
  canvas.setTextSize(1.75);
  canvas.setTextColor(c_btnTxt, c_btn1);
  canvas.drawString("1. RECIPE PARAMS >", 20, 66);
  canvas.setTextSize(1.2);
  canvas.drawString("Weights, Dips, Speeds, Cool & Drip", 20, 96);

  // Bucket 2 Card (x = 10..310, y = 124..198, h = 74)
  canvas.fillRoundRect(10, 124, 300, 74, 10, c_btn2);
  canvas.setTextDatum(middle_left);
  canvas.setTextSize(1.75);
  canvas.setTextColor(c_btnTxt, c_btn2);
  canvas.drawString("2. MACHINE SETUP >", 20, 148);
  canvas.setTextSize(1.2);
  canvas.drawString("Jig, Scale Cal, Limits, Motor, Screen", 20, 178);
  canvas.setTextSize(2.0);
}

// Draws Bucket 1: Recipe Parameters Hub (PAGE_RECIPE_HUB) using 3 full-width vertical cards + right-hand ▲/▼ scroll bar
void DisplayManager::drawRecipeHub() {
  canvas.fillScreen(c_bg);
  drawTopBanner(); drawBottomBanner();

  static const char* catTitles[4] = {
    "Targets (Wt & Dips)",
    "Motion Speeds",
    "Dip & Drip Timing",
    "Cooling Curve"
  };
  static const char* catHints[4] = {
    "Slim & Std batch setpoints",
    "Up, Down, Immerse & Extract",
    "Submerge hold & Drip pause",
    "Default, 1st & Last cool time"
  };

  int total = 4;
  int maxOff = ((total - 1) / 3) * 3;
  data.scrollOffset = constrain(data.scrollOffset, 0, maxOff);

  int yCoords[3] = { 40, 95, 150 };
  for (int slot = 0; slot < 3; slot++) {
    int idx = data.scrollOffset + slot;
    if (idx >= total) break;
    uint16_t cardBg = (idx % 2 == 0) ? c_btn1 : c_btn2;
    drawCardWithHint(yCoords[slot], 256, catTitles[idx], catHints[idx], "OPEN >",
                     cardBg, c_btnTxt, c_outline, TFT_WHITE);
  }
  drawRightScrollBar(total);
}

// Draws Bucket 2: Machine Setup Hub (PAGE_MACHINE_HUB) using 3 full-width vertical cards + right-hand ▲/▼ scroll bar
void DisplayManager::drawMachineHub() {
  canvas.fillScreen(c_bg);
  drawTopBanner(); drawBottomBanner();

  static const char* machTitles[6] = {
    "Jig & Soft Limit",
    "Scale & Collision",
    "Stepper Motor",
    "Screen & Theme",
    "Sound & Tunes",
    "Wi-Fi Portal"
  };
  static const char* machHints[6] = {
    "Jig length, Melt times, Max Z",
    "Load Cell Cal & Col. Limit",
    "TMC2209 mode & Coil power",
    "Brightness & Color Palette",
    "Volume, Startup, Finish & Alert",
    "Broadcast BeeMine-Setup AP"
  };

  int total = 6;
  int maxOff = ((total - 1) / 3) * 3;
  data.scrollOffset = constrain(data.scrollOffset, 0, maxOff);

  int yCoords[3] = { 40, 95, 150 };
  for (int slot = 0; slot < 3; slot++) {
    int idx = data.scrollOffset + slot;
    if (idx >= total) break;
    uint16_t cardBg = (idx % 2 == 0) ? c_btn1 : c_btn2;
    drawCardWithHint(yCoords[slot], 256, machTitles[idx], machHints[idx], "OPEN >",
                     cardBg, c_btnTxt, c_outline, TFT_WHITE);
  }
  drawRightScrollBar(total);
}

// Draws the Motor Settings sub-page (3 full-width 50px cards with hints: Driver Mode, Hybrid Speed, Motor Power)
void DisplayManager::drawMotorPage() {
  canvas.fillScreen(c_bg);
  drawTopBanner(); drawBottomBanner();

  static const char* modeLabels[3] = { "Stealth", "Spread", "Adaptive" };

  // Card 0: Driver Mode (y = 40..90)
  drawCardWithHint(40, 304, settingNames[12], settingHints[12], modeLabels[data.s_tmcMode % 3],
                   c_outline, TFT_WHITE, c_btn1, c_btnTxt);

  // Card 1: Hybrid Speed Threshold (y = 95..145)
  String threshStr = String(data.s_tmcThreshold) + " mm/s";
  uint16_t threshBg = (data.s_tmcMode == 2) ? c_btn1 : canvas.color565(45, 50, 58);
  uint16_t threshTxt = (data.s_tmcMode == 2) ? c_btnTxt : TFT_LIGHTGREY;
  drawCardWithHint(95, 304, settingNames[13], settingHints[13], threshStr.c_str(),
                   c_outline, TFT_WHITE, threshBg, threshTxt);

  // Card 2: Motor Power Toggle (y = 150..200)
  uint16_t pwrColor = data.isMotorEnabled ? c_active : canvas.color565(180, 40, 40);
  uint16_t pwrTxt   = data.isMotorEnabled ? TFT_BLACK : TFT_WHITE;
  drawCardWithHint(150, 304, "Motor Power", "Enable or release Z coils",
                   data.isMotorEnabled ? "ENABLED" : "DISABLED",
                   c_outline, TFT_WHITE, pwrColor, pwrTxt);
}

// Draws the Scale & Collision Settings page (3 full-width 50px cards with hints):
// Card 0 (y = 40..90):  Col. Limit (g) - negative gram threshold for wax collision retraction
// Card 1 (y = 95..145): Weight Dwell (ms) - settling delay at top of stroke before reading scale
// Card 2 (y = 150..200): Load Cell Calibration Wizard button ("WIZARD >")
void DisplayManager::drawSensorAndScalePage() {
  canvas.fillScreen(c_bg);
  drawTopBanner(); drawBottomBanner();

  // Card 0: Collision Weight Limit (s8)
  String colStr = (data.s_colLimit == 0) ? "0 (Off)" : (String(data.s_colLimit) + " g");
  drawCardWithHint(40, 304, settingNames[8], settingHints[8], colStr.c_str(),
                   c_outline, TFT_WHITE, c_btn1, c_btnTxt);

  // Card 1: Weight Dwell (s10)
  String dwellStr = String(data.s_weightDwell) + " ms";
  drawCardWithHint(95, 304, settingNames[10], settingHints[10], dwellStr.c_str(),
                   c_outline, TFT_WHITE, c_btn1, c_btnTxt);

  // Card 2: Load Cell Calibration Wizard
  drawCardWithHint(150, 304, "Load Cell Cal.", "2-step 0g & known weight", "WIZARD >",
                   c_outline, TFT_WHITE, c_active, TFT_BLACK);
}

// Draws the Guided 2-Step Load Cell Calibration Wizard (PAGE_SCALE_CAL):
// - Displays live scale reading (g) and active calibration factor (saved in NVS)
// - Button 1: "1. ZERO UNLOADED (0g)" (zeroes the current reading in RAM before hanging reference weight)
// - Button 2: "2. SET KNOWN WEIGHT (g)" (opens numpad to input reference weight in grams and saves factor to NVS)
void DisplayManager::drawScaleCalPage() {
  canvas.fillScreen(c_bg);
  drawTopBanner(); drawBottomBanner();

  // Status & Live Readout Card (y = 42..106)
  canvas.fillRoundRect(10, 42, 300, 62, 8, c_outline);
  canvas.setTextColor(TFT_WHITE, c_outline);
  canvas.setTextSize(1.5);
  canvas.setTextDatum(middle_left);
  char liveStr[32];
  snprintf(liveStr, sizeof(liveStr), "Live: %.1f g", data.currentWeight);
  canvas.drawString(liveStr, 20, 58);

  canvas.setTextDatum(middle_right);
  char facStr[32];
  snprintf(facStr, sizeof(facStr), "Factor: %.1f", data.scaleFactor);
  canvas.drawString(facStr, 300, 58);

  canvas.setTextDatum(middle_center);
  canvas.setTextColor(TFT_YELLOW, c_outline);
  canvas.setTextSize(1.3);
  canvas.drawString(data.calStatusText.c_str(), 160, 86);

  // Step 1 & Step 2 Action Buttons (y = 112..196)
  canvas.setTextSize(1.6);
  drawButton(10, 112, 300, 38, "1. ZERO UNLOADED SCALE", TFT_ORANGE, TFT_BLACK);
  drawButton(10, 156, 300, 38, "2. SET KNOWN WEIGHT (g)", c_active, TFT_BLACK);
  canvas.setTextSize(2.0);
}

// Renders up to 3 full-width vertical parameter cards (50px tall each, with Title + Hint + Value Pill)
// and draws the 45px-wide right-hand ▲/▼ scroll column when count > 3!
void DisplayManager::drawSettingsList(const int* indices, int count) {
  canvas.fillScreen(c_bg);
  drawTopBanner(); drawBottomBanner();

  int maxOff = ((count - 1) / 3) * 3;
  data.scrollOffset = constrain(data.scrollOffset, 0, maxOff);

  int cardW = (count > 3) ? 256 : 304;
  int yCoords[3] = { 40, 95, 150 };

  for (int slot = 0; slot < 3; slot++) {
    int itemIdx = data.scrollOffset + slot;
    if (itemIdx >= count) break;

    int index = indices[itemIdx];
    int val = getSettingValue(index);
    String valStr;
    if ((index == 17 || index == 18 || index == 19 || index == 20) && val == 0) {
      valStr = "0 (Def)";
    } else if ((index == 21 || index == 22) && val == 0) {
      valStr = "0 (Off)";
    } else if (index == 21 && val > 0) {
      valStr = "+" + String(val) + " " + settingUnits[index];
    } else {
      valStr = String(val) + " " + settingUnits[index];
    }

    drawCardWithHint(yCoords[slot], cardW, settingNames[index], settingHints[index],
                     valStr.c_str(), c_outline, TFT_WHITE, c_btn1, c_btnTxt);
  }

  drawRightScrollBar(count);
}

// Draws the Screen Settings sub-page (Brightness bar and Color Theme button)
void DisplayManager::drawScreenDashboard() {
  canvas.fillScreen(c_bg);
  drawTopBanner(); drawBottomBanner();
  
  // Row 0: Brightness (y = 50..105)
  canvas.fillRoundRect(8, 50, 304, 52, 8, c_outline);
  canvas.setTextDatum(middle_left);
  canvas.setTextSize(1.55);
  canvas.setTextColor(TFT_WHITE, c_outline);
  canvas.drawString("Brightness", 16, 66);
  canvas.setTextSize(1.1);
  canvas.setTextColor(canvas.color565(190, 205, 220), c_outline);
  canvas.drawString("Drag bar (1% to 99% PWM)", 16, 87);

  canvas.fillRoundRect(164, 58, 140, 36, 6, c_bg);
  int fillWidth = map(data.s_brightness, 1, 99, 14, 140);
  canvas.fillRoundRect(164, 58, fillWidth, 36, 6, c_active);
  String bStr = String(data.s_brightness) + "%";
  canvas.setTextDatum(middle_center);
  canvas.setTextSize(1.5);
  canvas.setTextColor(TFT_BLACK, c_active);
  canvas.drawString(bStr.c_str(), 234, 76);

  // Row 1: Color Theme (y = 120..172)
  drawCardWithHint(120, 304, "Color Theme", "Tap to cycle UI palette",
                   themeNames[data.s_theme % 3], c_outline, TFT_WHITE, c_btn1, c_btnTxt);
}

// Draws the Sound & Tunes Settings sub-page (PAGE_SETTING_SOUND):
// Uses the 3-card vertical layout (256px wide cards) + 45px right-hand ▲/▼ scroll column for 4 items:
// - Item 0: Speaker Volume (0% MUTE to 100% — tap to step by 20% or drag slider bar)
// - Item 1: Startup Tune (Tap to cycle & preview immediately)
// - Item 2: Finish Tune (Batch complete melody — tap to cycle & preview immediately)
// - Item 3: Alert / Error Tune (Collision / Low-Wax alarm — tap to cycle & preview immediately)
void DisplayManager::drawSoundPage() {
  canvas.fillScreen(c_bg);
  drawTopBanner(); drawBottomBanner();

  int total = 4;
  int maxOff = ((total - 1) / 3) * 3;
  data.scrollOffset = constrain(data.scrollOffset, 0, maxOff);

  int yCoords[3] = { 40, 95, 150 };
  for (int slot = 0; slot < 3; slot++) {
    int itemIdx = data.scrollOffset + slot;
    if (itemIdx >= total) break;
    int y = yCoords[slot];

    if (itemIdx == 0) {
      // Item 0: Speaker Volume Card with visual progress bar
      canvas.fillRoundRect(8, y, 256, 50, 8, c_outline);
      canvas.setTextDatum(middle_left);
      canvas.setTextSize(1.55);
      canvas.setTextColor(TFT_WHITE, c_outline);
      canvas.drawString("Volume", 16, y + 16);
      canvas.setTextSize(1.1);
      canvas.setTextColor(canvas.color565(190, 205, 220), c_outline);
      canvas.drawString("Tap +20% or drag bar", 16, y + 37);

      // Volume Bar (x = 148..258, w = 110)
      canvas.fillRoundRect(148, y + 7, 110, 36, 6, c_bg);
      if (data.s_volume > 0) {
        int fillW = map(data.s_volume, 0, 100, 12, 110);
        canvas.fillRoundRect(148, y + 7, fillW, 36, 6, c_active);
      }
      String vStr = (data.s_volume <= 0) ? "MUTE" : (String(data.s_volume) + "%");
      canvas.setTextDatum(middle_center);
      canvas.setTextSize(1.45);
      canvas.setTextColor(data.s_volume > 0 ? TFT_BLACK : TFT_RED, data.s_volume > 0 ? c_active : c_bg);
      canvas.drawString(vStr.c_str(), 203, y + 25);
      canvas.setTextSize(2.0);
    } else if (itemIdx == 1) {
      // Item 1: Startup Tune Selector
      const char* tName = SoundManager::getStartupTuneName(data.s_startupTune);
      uint16_t pillBg = (data.s_startupTune == 0) ? c_bg : c_btn1;
      uint16_t pillTxt = (data.s_startupTune == 0) ? TFT_LIGHTGREY : c_btnTxt;
      drawCardWithHint(y, 256, "Startup Tune", "Tap to cycle & preview", tName,
                       c_outline, TFT_WHITE, pillBg, pillTxt);
    } else if (itemIdx == 2) {
      // Item 2: Batch Finish Tune Selector
      const char* tName = SoundManager::getFinishTuneName(data.s_finishTune);
      uint16_t pillBg = (data.s_finishTune == 0) ? c_bg : c_btn2;
      uint16_t pillTxt = (data.s_finishTune == 0) ? TFT_LIGHTGREY : c_btnTxt;
      drawCardWithHint(y, 256, "Finish Tune", "Batch done (Tap to test)", tName,
                       c_outline, TFT_WHITE, pillBg, pillTxt);
    } else if (itemIdx == 3) {
      // Item 3: Alert / Error Tune Selector
      const char* tName = SoundManager::getAlertTuneName(data.s_alertTune);
      uint16_t pillBg = (data.s_alertTune == 0) ? c_bg : TFT_MAROON;
      uint16_t pillTxt = (data.s_alertTune == 0) ? TFT_LIGHTGREY : TFT_WHITE;
      drawCardWithHint(y, 256, "Alert Tune", "Error/Collision (Tap test)", tName,
                       c_outline, TFT_WHITE, pillBg, pillTxt);
    }
  }

  drawRightScrollBar(total);
}

// Draws instructions while the ESP32 is broadcasting the "BeeMine-Setup" Wi-Fi Captive Portal AP
void DisplayManager::drawWifiPortalPage() {
  canvas.fillScreen(c_bg);
  drawTopBanner(); drawBottomBanner();

  canvas.setTextDatum(middle_center);
  canvas.setTextColor(c_bannerTxt == TFT_BLACK ? TFT_WHITE : c_btnTxt, c_bg);
  canvas.drawString("WI-FI SETUP PORTAL", 160, 50);

  canvas.fillRoundRect(20, 70, 280, 110, 10, c_outline);
  canvas.setTextColor(TFT_WHITE, c_outline);
  canvas.drawString("Connect phone to Wi-Fi:", 160, 90);
  canvas.setTextColor(TFT_YELLOW, c_outline);
  canvas.drawString("BeeMine-Setup", 160, 115);
  canvas.setTextColor(TFT_WHITE, c_outline);
  canvas.drawString("Open http://192.168.4.1", 160, 145);
}

// Draws the pop-up numeric keypad overlay for entering parameter values or calibration reference weight
void DisplayManager::drawNumpad() {
  canvas.fillRect(20, 30, 280, 195, c_banner);
  canvas.setTextDatum(top_center);
  canvas.setTextColor(c_bannerTxt, c_banner);
  int labelIdx = constrain(data.activeSetting, 0, 23);
  canvas.drawString(settingNames[labelIdx], 160, 35);
  
  canvas.fillRect(30, 55, 260, 30, TFT_BLACK);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.drawString(data.numpadStr.c_str(), 175, 70);

  // Show "+/-" sign toggle button when editing signed parameters (Col. Limit s8 or Drip Offset s21)
  if (data.activeSetting == 8 || data.activeSetting == 21) {
    canvas.setTextSize(1.5);
    drawButton(32, 57, 56, 26, "+/-", c_outline, TFT_YELLOW);
    canvas.setTextSize(2.0);
  }
  
  const char* keys[12] = {"1","2","3","4","5","6","7","8","9","DEL","0","CLR"};
  int cols[3] = {50, 115, 180}; int rows[4] = {90, 118, 146, 174};
  
  for(int r = 0; r < 4; r++) {
     for(int c = 0; c < 3; c++) {
        drawButton(cols[c], rows[r], 60, 25, keys[r * 3 + c], c_outline, TFT_WHITE);
     }
  }
  drawButton(245, 90, 50, 53, "CAN", TFT_MAROON, TFT_WHITE);
  drawButton(245, 146, 50, 53, "OK", TFT_DARKGREEN, TFT_WHITE);
}

// Formats seconds into human-readable duration (e.g., "2m 15s" or "45s") for the completion screen
static void formatPrettyDuration(int totalSeconds, char* buffer, size_t bufferSize) {
  int m = totalSeconds / 60;
  int s = totalSeconds % 60;
  if (m > 0) {
    snprintf(buffer, bufferSize, "%dm %ds", m, s);
  } else {
    snprintf(buffer, bufferSize, "%ds", s);
  }
}

// Maps (isWeight, isSlim) booleans to history bucket index 0..3
static int getProcessTypeIndex(bool isWeight, bool isSlim) {
  if (isWeight && isSlim) return 0;       // Slim Weight
  if (isWeight && !isSlim) return 1;      // Std Weight
  if (!isWeight && isSlim) return 2;      // Slim Dips
  return 3;                               // Std Dips
}

// Computes the average run duration (in seconds) of up to the last 5 completed runs for a profile
int DisplayManager::getAverageTime(bool isWeight, bool isSlim) const {
  int idx = getProcessTypeIndex(isWeight, isSlim);
  int cnt = data.historyCount[idx];
  if (cnt <= 0) return 0;
  long sum = 0;
  for (int i = 0; i < cnt; i++) {
    sum += data.processHistory[idx][i];
  }
  return (int)(sum / cnt);
}

// Inserts a newly completed run's duration into the 5-entry circular history buffer
void DisplayManager::recordCompletedProcess(bool isWeight, bool isSlim, int seconds) {
  if (seconds <= 0) return;
  int idx = getProcessTypeIndex(isWeight, isSlim);
  int head = data.historyHead[idx];
  data.processHistory[idx][head] = seconds;
  data.historyHead[idx] = (head + 1) % 5;
  if (data.historyCount[idx] < 5) {
    data.historyCount[idx]++;
  }
}

// Draws the live telemetry screen during an automated dipping run
void DisplayManager::drawActiveDipPage() {
  canvas.fillScreen(c_bg);
  drawTopBanner();
  canvas.drawRoundRect(20, 185, 280, 45, 10, TFT_RED);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(TFT_RED, c_bg);
  canvas.drawString("PRESS ANYWHERE TO STOP", 160, 207);

  int elapsedSeconds = (millis() - data.dipStartTime) / 1000;
  char elapsedStr[16]; char estStr[16];
  formatTime(elapsedSeconds, elapsedStr, sizeof(elapsedStr));
  formatTime(data.estimatedTotalSeconds, estStr, sizeof(estStr));

  char timeStr[64]; snprintf(timeStr, sizeof(timeStr), "Time: %s / %s (est)", elapsedStr, estStr);
  char targetStr[64];
  if (data.isActiveWeightBased) {
    snprintf(targetStr, sizeof(targetStr), "Weight: %dg / %dg", (int)roundf(data.currentWeight), (data.isActiveSlimProfile ? data.s_slimWt : data.s_stdWt));
  } else {
    snprintf(targetStr, sizeof(targetStr), "Dips: %d / %d", data.currentDipCount, (data.isActiveSlimProfile ? data.s_slimDips : data.s_stdDips));
  }

  canvas.setTextDatum(middle_center);
  canvas.setTextColor(TFT_ORANGE, c_bg);
  canvas.drawString(data.currentPhaseText.c_str(), 160, 75);
  canvas.setTextColor(c_bannerTxt == TFT_BLACK ? TFT_WHITE : c_btnTxt, c_bg);
  canvas.drawString(timeStr, 160, 115);
  canvas.drawString(targetStr, 160, 150);
}

// Draws the confirmation modal after the carriage has safely retracted to the top rest position
void DisplayManager::drawStopConfirmPage() {
  canvas.fillScreen(c_bg); 
  drawTopBanner();
  canvas.setTextDatum(middle_center); 
  if (data.lowWaxTriggered) {
    canvas.setTextSize(1.6);
    canvas.setTextColor(TFT_ORANGE, c_bg);
    canvas.drawString("Low Wax! Pot below Soft Limit", 160, 65);
    canvas.setTextSize(2.0);
    canvas.setTextColor(TFT_WHITE, c_bg);
    canvas.drawString("Abort current dipping?", 160, 105);
  } else if (data.collisionTriggered) {
    canvas.setTextSize(1.6);
    canvas.setTextColor(TFT_ORANGE, c_bg);
    canvas.drawString("Collision! Raised to top rest", 160, 65);
    canvas.setTextSize(2.0);
    canvas.setTextColor(TFT_WHITE, c_bg);
    canvas.drawString("Abort current dipping?", 160, 105);
  } else {
    canvas.setTextColor(TFT_WHITE, c_bg);
    canvas.drawString("Abort current dipping?", 160, 80);
  }
  drawButton(20, 150, 130, 70, "NO", c_outline, TFT_WHITE); 
  drawButton(170, 150, 130, 70, "YES", TFT_RED, TFT_WHITE);
}

// Draws the post-dip summary screen (either green "Success!" or red "cancelled")
void DisplayManager::drawDipDonePage() {
  canvas.fillScreen(c_bg); 
  drawTopBanner();
  canvas.setTextDatum(middle_center); 

  char prettyTime[32];
  formatPrettyDuration(data.finalElapsedSeconds, prettyTime, sizeof(prettyTime));

  const char* candleType = data.isActiveSlimProfile ? "Slim" : "Std.";

  if (!data.dipWasAborted) {
    // Success Screen
    char line1[64];
    snprintf(line1, sizeof(line1), "Success! %s ready", candleType);

    char line2[64];
    snprintf(line2, sizeof(line2), "Completed in %s", prettyTime);

    canvas.setTextColor(canvas.color565(130, 230, 130), c_bg); // Soft pastel green
    canvas.drawString(line1, 160, 75);
    canvas.setTextColor(TFT_WHITE, c_bg);
    canvas.drawString(line2, 160, 115);

    drawButton(10, 144, 300, 75, "FINISH", c_active, TFT_BLACK);
  } else {
    // Cancelled Screen
    char line1[64];
    snprintf(line1, sizeof(line1), "%s cancelled", candleType);

    char line2[64];
    snprintf(line2, sizeof(line2), "Cancelled at %s", prettyTime);

    canvas.setTextColor(TFT_RED, c_bg);
    canvas.drawString(line1, 160, 75);
    canvas.setTextColor(TFT_WHITE, c_bg);
    canvas.drawString(line2, 160, 115);

    drawButton(10, 144, 300, 75, "BACK", c_btn2, TFT_WHITE);
  }
}

// Main rendering dispatcher: only redraws and pushes sprite to SPI LCD when data.pageChanged is true
void DisplayManager::renderCurrentPage() {
  if (!data.pageChanged) return;

  switch (data.currentPage) {
    case PAGE_WEIGHT_DIP: 
      drawDipLauncherPage(); 
      break;
    case PAGE_MANUAL: 
      drawManualPage(); 
      break;
    case PAGE_JIG_TOOLS:
      drawJigToolsPage();
      break;
    case PAGE_SETTINGS_HUB: 
      drawSettingsHub(); 
      break;
    case PAGE_RECIPE_HUB:
      drawRecipeHub();
      break;
    case PAGE_MACHINE_HUB:
      drawMachineHub();
      break;
    case PAGE_SETTING_DIPPING: 
      if (data.showNumpad) drawNumpad(); else drawSettingsList(dippingSettingIndices, 4); 
      break;
    case PAGE_SETTING_TIMING: 
      if (data.showNumpad) drawNumpad(); else drawSettingsList(timingSettingIndices, 4); 
      break;
    case PAGE_SETTING_COOLING:
      if (data.showNumpad) drawNumpad(); else drawSettingsList(coolingSettingIndices, 3);
      break;
    case PAGE_SETTING_SPEEDS:
      if (data.showNumpad) drawNumpad(); else drawSettingsList(speedsSettingIndices, 4);
      break;
    case PAGE_SETTING_JIG:
      if (data.showNumpad) drawNumpad(); else drawSettingsList(jigSettingIndices, 4);
      break;
    case PAGE_SETTING_SENSOR: 
      if (data.showNumpad) drawNumpad(); else drawSensorAndScalePage(); 
      break;
    case PAGE_SCALE_CAL:
      if (data.showNumpad) drawNumpad(); else drawScaleCalPage();
      break;
    case PAGE_SETTING_SCREEN: 
      if (data.showNumpad) drawNumpad(); else drawScreenDashboard(); 
      break;
    case PAGE_SETTING_SOUND:
      drawSoundPage();
      break;
    case PAGE_SETTING_MOTOR:
      if (data.showNumpad) drawNumpad(); else drawMotorPage();
      break;
    case PAGE_WIFI_PORTAL: 
      drawWifiPortalPage(); 
      break;
    case PAGE_ACTIVE_DIP: 
      drawActiveDipPage(); 
      break;
    case PAGE_STOP_CONFIRM: 
      drawStopConfirmPage();
      break;
    case PAGE_DIP_DONE:   
      drawDipDonePage();
      break;
  }

  canvas.pushSprite(0, 0); 
  data.pageChanged = false;
}

// Initializes UI state variables and estimated duration before switching to PAGE_ACTIVE_DIP
void DisplayManager::startDippingProcess(bool isWeight, bool isSlim) {
  data.isActiveWeightBased = isWeight; 
  data.isActiveSlimProfile = isSlim;
  data.dipStartTime = millis(); 
  data.dipWasAborted = false; 
  data.collisionTriggered = false;
  data.lowWaxTriggered = false;
  data.currentDipCount = 1;
  data.finalElapsedSeconds = 0;
  data.currentPhaseText = "Dip 1: Lowering...";

  int avg = getAverageTime(isWeight, isSlim);
  if (avg > 0) {
    data.estimatedTotalSeconds = avg;
  } else {
    int targetDips = isSlim ? data.s_slimDips : data.s_stdDips;
    int startCool = (data.s_coolTime1 > 0) ? data.s_coolTime1 : data.s_coolTime;
    int endCool   = (data.s_coolTimeEnd > 0) ? data.s_coolTimeEnd : data.s_coolTime;
    int avgCool   = (startCool + endCool) / 2;
    int dripExtra = (data.s_dripOffset != 0 && data.s_dripTime > 0) ? data.s_dripTime : 0;
    int extraMelt = data.meltBasesEnabled ? (data.s_baseMeltTime + 6) : 0;
    data.estimatedTotalSeconds = targetDips * (data.s_subDipTime + avgCool + dripExtra + 6) + extraMelt;
  }

  data.currentPage = PAGE_ACTIVE_DIP; 
  data.pageChanged = true;
}

// Records elapsed time (and updates history if not aborted) before switching to PAGE_DIP_DONE
void DisplayManager::endDippingProcess(bool aborted, int elapsedSeconds) {
  data.dipWasAborted = aborted; 
  data.collisionTriggered = false;
  data.lowWaxTriggered = false;
  if (elapsedSeconds > 0) {
    data.finalElapsedSeconds = elapsedSeconds;
  } else {
    data.finalElapsedSeconds = (int)((millis() - data.dipStartTime) / 1000);
  }

  if (!aborted) {
    recordCompletedProcess(data.isActiveWeightBased, data.isActiveSlimProfile, data.finalElapsedSeconds);
  }

  data.currentPage = PAGE_DIP_DONE; 
  data.pageChanged = true;
}

// Helper to map a touch Y coordinate (40..200) to card slot 0, 1, or 2 (or -1 if outside cards)
static int getCardSlotFromY(int ty) {
  if (ty >= 40 && ty <= 90)  return 0;
  if (ty >= 95 && ty <= 145) return 1;
  if (ty >= 150 && ty <= 200) return 2;
  return -1;
}

// Helper to handle taps on the 45px-wide right-hand ▲/▼ scroll column (x >= 266, y = 40..200)
static bool handleRightScrollTouch(int tx, int ty, int totalItems, DisplayData& data) {
  if (totalItems <= 3 || tx < 266) return false;
  int maxOff = ((totalItems - 1) / 3) * 3;
  if (ty >= 40 && ty <= 86) { // Up ▲ button
    data.scrollOffset = max(0, data.scrollOffset - 3);
    data.pageChanged = true;
    return true;
  }
  if (ty >= 154 && ty <= 202) { // Down ▼ button
    data.scrollOffset = min(maxOff, data.scrollOffset + 3);
    data.pageChanged = true;
    return true;
  }
  if (ty >= 87 && ty <= 153) { // Middle page indicator cycles pages
    data.scrollOffset = (data.scrollOffset + 3 > maxOff) ? 0 : (data.scrollOffset + 3);
    data.pageChanged = true;
    return true;
  }
  return false;
}

// Polls the FT6336U capacitive touch screen, handles page navigation, and returns hardware action events
UiEvent DisplayManager::updateTouch() {
  UiEvent eventTriggered = UI_EVENT_NONE;

  if (M5.Touch.getCount() > 0) {
    auto touch = M5.Touch.getDetail();

    // Continuous drag handler: Brightness bar slider adjustment on Screen Settings page (y = 50..105)
    if (data.currentPage == PAGE_SETTING_SCREEN && touch.isPressed() && !data.showNumpad) {
      if (touch.y >= 50 && touch.y <= 105 && touch.x >= 160 && touch.x <= 310) {
        int bVal = map(touch.x, 164, 304, 1, 99);
        setBrightness(bVal);
        data.pageChanged = true;
        eventTriggered = UI_EVENT_BRIGHTNESS_CHANGED;
      }
    }
    // Continuous drag handler: Volume bar slider adjustment on Sound & Tunes page (Item 0 at y = 40..90 when scrollOffset == 0)
    else if (data.currentPage == PAGE_SETTING_SOUND && data.scrollOffset == 0 && touch.isPressed()) {
      if (touch.y >= 40 && touch.y <= 90 && touch.x >= 145 && touch.x <= 262) {
        int vVal = constrain((int)map(touch.x, 150, 256, 0, 100), 0, 100);
        if (vVal != data.s_volume) {
          data.s_volume = vVal;
          data.pageChanged = true;
          eventTriggered = UI_EVENT_VOLUME_CHANGED;
        }
      }
    }

    // Discrete button tap handler (triggers when finger is lifted off the screen)
    if (touch.wasReleased()) {
      int tx = touch.x; int ty = touch.y;
      int bottomStartY = 204; // Bottom 36px navigation bar

      // 1. Numpad overlay touch hit-testing (takes priority when modal is open)
      if (data.showNumpad) {
        // "+/-" sign toggle button when editing signed parameters (Col. Limit s8 or Drip Offset s21)
        if ((data.activeSetting == 8 || data.activeSetting == 21) &&
            tx >= 30 && tx <= 90 && ty >= 55 && ty <= 85) {
          if (data.numpadStr.startsWith("-")) {
            data.numpadStr.remove(0, 1);
          } else {
            data.numpadStr = "-" + data.numpadStr;
          }
          data.pageChanged = true;
          return eventTriggered;
        }

        int cols[3] = {50, 115, 180}; int rows[4] = {90, 118, 146, 174};
        for(int r = 0; r < 4; r++) { 
          for(int c = 0; c < 3; c++) {
            if (tx >= cols[c] && tx <= cols[c] + 60 && ty >= rows[r] && ty <= rows[r] + 25) {
               int k = r * 3 + c;
               if (k < 9) data.numpadStr += String(k + 1); 
               else if (k == 9 && data.numpadStr.length() > 0) data.numpadStr.remove(data.numpadStr.length() - 1);
               else if (k == 10) data.numpadStr += "0";
               else if (k == 11) data.numpadStr = "";
               data.pageChanged = true; 
               return eventTriggered;
            }
          }
        }
        if (tx >= 245 && tx <= 295) {
          if (ty >= 90 && ty <= 143) { // "CAN" (Cancel) button
            data.showNumpad = false; 
            data.pageChanged = true; 
          } 
          else if (ty >= 146 && ty <= 199) { // "OK" (Confirm) button
            int val = data.numpadStr.toInt();
            setSettingValue(data.activeSetting, val);
            data.showNumpad = false; 
            data.pageChanged = true;
            if (data.activeSetting == 23) {
              eventTriggered = UI_EVENT_CALIBRATE_SCALE;
            } else {
              eventTriggered = UI_EVENT_SETTING_UPDATED;
            }
          }
        }
        return eventTriggered;
      }

      // 2. Touching anywhere during an active dip pauses and opens the Abort Confirm modal
      if (data.currentPage == PAGE_ACTIVE_DIP) { 
        data.collisionTriggered = false;
        data.lowWaxTriggered = false;
        data.currentPage = PAGE_STOP_CONFIRM; 
        data.stopConfirmEnterTime = millis();
        data.pageChanged = true; 
      }
      // 3. Abort Confirm modal ("NO" resumes dipping, "YES" aborts dipping)
      else if (data.currentPage == PAGE_STOP_CONFIRM) {
        if (millis() - data.stopConfirmEnterTime < 500) {
          return UI_EVENT_NONE;
        }
        if (tx >= 20 && tx <= 150 && ty >= 150 && ty <= 220) { 
          data.collisionTriggered = false;
          data.lowWaxTriggered = false;
          data.currentPage = PAGE_ACTIVE_DIP; 
          data.pageChanged = true; 
          data.lastUiTick = millis(); 
        } 
        else if (tx >= 170 && tx <= 300 && ty >= 150 && ty <= 220) {
          endDippingProcess(true);
          eventTriggered = UI_EVENT_ABORT_DIP;
        }
      }
      // 4. Dip Done summary screen ("FINISH" / "BACK" button returns to Dip Launcher)
      else if (data.currentPage == PAGE_DIP_DONE) {
        if (ty >= 144) { 
          data.currentPage = PAGE_WEIGHT_DIP; 
          data.pageChanged = true; 
          eventTriggered = UI_EVENT_FINISH_DIP;
        }
      }
      // 5. Bottom Navigation Bar (y > 204)
      else if (ty > bottomStartY) {
        // Stop manual jogging if navigating away from Manual page
        data.isUpPressed = false;
        data.isDownPressed = false;

        if (data.currentPage == PAGE_WIFI_PORTAL) {
          eventTriggered = UI_EVENT_STOP_WIFI_PORTAL;
        } else if (data.currentPage == PAGE_SCALE_CAL) {
          setCurrentPage(PAGE_SETTING_SENSOR);
        } else if (data.currentPage == PAGE_RECIPE_HUB || data.currentPage == PAGE_MACHINE_HUB) {
          setCurrentPage(PAGE_SETTINGS_HUB);
        } else if (data.currentPage == PAGE_SETTING_DIPPING ||
                   data.currentPage == PAGE_SETTING_SPEEDS ||
                   data.currentPage == PAGE_SETTING_TIMING ||
                   data.currentPage == PAGE_SETTING_COOLING) {
          setCurrentPage(PAGE_RECIPE_HUB);
        } else if (data.currentPage == PAGE_SETTING_JIG ||
                   data.currentPage == PAGE_SETTING_SENSOR ||
                   data.currentPage == PAGE_SETTING_MOTOR ||
                   data.currentPage == PAGE_SETTING_SCREEN ||
                   data.currentPage == PAGE_SETTING_SOUND) {
          setCurrentPage(PAGE_MACHINE_HUB);
        } else {
          // Persistent 4-Tab Bottom Bar: [ DIP (0..79) ] | [ MANUAL (80..159) ] | [ TOOLS (160..239) ] | [ SETUP (240..320) ]
          if (tx < 80)       setCurrentPage(PAGE_WEIGHT_DIP);
          else if (tx < 160) setCurrentPage(PAGE_MANUAL);
          else if (tx < 240) setCurrentPage(PAGE_JIG_TOOLS);
          else               setCurrentPage(PAGE_SETTINGS_HUB);
        }
      }
      // 6. Unified Dipping Launcher Page (Mode Toggle, Melt Bases Checkbox, Slim/Standard Cards, START DIP)
      else if (data.currentPage == PAGE_WEIGHT_DIP) {
        if (ty >= 41 && ty <= 71) {
          if (tx >= 10 && tx <= 155) { // Left pill: Toggle Weight vs Dips mode
            data.isActiveWeightBased = !data.isActiveWeightBased;
            data.pageChanged = true;
          } else if (tx >= 165 && tx <= 310) { // Right pill: Check/uncheck Melt Bases at end of dip
            data.meltBasesEnabled = !data.meltBasesEnabled;
            data.pageChanged = true;
            eventTriggered = UI_EVENT_MELT_TOGGLED;
          }
        } else if (ty >= 76 && ty <= 144) { // Profile selection cards
          if (tx >= 10 && tx <= 155) {
            data.isActiveSlimProfile = true;
            data.pageChanged = true;
          } else if (tx >= 165 && tx <= 310) {
            data.isActiveSlimProfile = false;
            data.pageChanged = true;
          }
        } else if (ty >= 150 && ty <= 198 && tx >= 10 && tx <= 310) { // START DIP button
          startDippingProcess(data.isActiveWeightBased, data.isActiveSlimProfile);
          eventTriggered = UI_EVENT_START_DIP;
        }
      }
      // 7. Jig & Wax Finishing Tools Page (Melt Candle Bases, Clean Whole Jig, Jig Settings shortcut)
      else if (data.currentPage == PAGE_JIG_TOOLS) {
        if (data.justStoppedByTouch) {
          data.justStoppedByTouch = false;
          data.pageChanged = true;
          return UI_EVENT_NONE;
        }
        if (tx >= 10 && tx <= 310) {
          if (ty >= 42 && ty <= 92) { // Button 1: MELT CANDLE BASES / STOP
            data.pageChanged = true;
            eventTriggered = data.isMeltBasesActive ? UI_EVENT_MANUAL_STOP : UI_EVENT_TOOL_MELT_BASES;
          } else if (ty >= 98 && ty <= 148) { // Button 2: CLEAN WHOLE JIG / STOP
            data.pageChanged = true;
            eventTriggered = data.isCleanJigActive ? UI_EVENT_MANUAL_STOP : UI_EVENT_TOOL_CLEAN_JIG;
          } else if (ty >= 154 && ty <= 198) { // Button 3: JIG & MELT SETTINGS
            setCurrentPage(PAGE_SETTING_JIG);
          }
        }
      }
      // 8. Top-Level SETUP Hub (2 Full-Width Bucket Cards: 1. Recipe Params vs 2. Machine Setup)
      else if (data.currentPage == PAGE_SETTINGS_HUB) {
        if (tx >= 10 && tx <= 310) {
          if (ty >= 42 && ty <= 116) {
            setCurrentPage(PAGE_RECIPE_HUB);
          } else if (ty >= 124 && ty <= 198) {
            setCurrentPage(PAGE_MACHINE_HUB);
          }
        }
      }
      // 9. Bucket 1: Recipe Parameters Hub (3-card vertical list + right-hand ▲/▼ scroll column)
      else if (data.currentPage == PAGE_RECIPE_HUB) {
        if (ty <= 36 && tx >= 200) { // Top-right "MACHINE >" quick switch
          setCurrentPage(PAGE_MACHINE_HUB);
        } else if (!handleRightScrollTouch(tx, ty, 4, data) && tx <= 264) {
          int slot = getCardSlotFromY(ty);
          int idx = data.scrollOffset + slot;
          if (slot >= 0 && idx < 4) {
            static const AppState recipePages[4] = {
              PAGE_SETTING_DIPPING, PAGE_SETTING_SPEEDS, PAGE_SETTING_TIMING, PAGE_SETTING_COOLING
            };
            setCurrentPage(recipePages[idx]);
          }
        }
      }
      // 10. Bucket 2: Machine Setup Hub (3-card vertical list + right-hand ▲/▼ scroll column)
      else if (data.currentPage == PAGE_MACHINE_HUB) {
        if (ty <= 36 && tx >= 200) { // Top-right "< RECIPE" quick switch
          setCurrentPage(PAGE_RECIPE_HUB);
        } else if (!handleRightScrollTouch(tx, ty, 6, data) && tx <= 264) {
          int slot = getCardSlotFromY(ty);
          int idx = data.scrollOffset + slot;
          if (slot >= 0 && idx < 6) {
            if (idx == 0) setCurrentPage(PAGE_SETTING_JIG);
            else if (idx == 1) setCurrentPage(PAGE_SETTING_SENSOR);
            else if (idx == 2) setCurrentPage(PAGE_SETTING_MOTOR);
            else if (idx == 3) setCurrentPage(PAGE_SETTING_SCREEN);
            else if (idx == 4) setCurrentPage(PAGE_SETTING_SOUND);
            else if (idx == 5) {
              setCurrentPage(PAGE_WIFI_PORTAL);
              eventTriggered = UI_EVENT_START_WIFI_PORTAL;
            }
          }
        }
      }
      // 11. Scrollable 3-Card Numeric Parameter Lists (Targets, Speeds, Timing & Drip, Cooling, Jig & Limit)
      else if (data.currentPage == PAGE_SETTING_DIPPING ||
               data.currentPage == PAGE_SETTING_SPEEDS ||
               data.currentPage == PAGE_SETTING_TIMING ||
               data.currentPage == PAGE_SETTING_COOLING ||
               data.currentPage == PAGE_SETTING_JIG) {
        const int* listPtr = dippingSettingIndices;
        int listCount = 4;
        if (data.currentPage == PAGE_SETTING_SPEEDS)       { listPtr = speedsSettingIndices;  listCount = 4; }
        else if (data.currentPage == PAGE_SETTING_TIMING)  { listPtr = timingSettingIndices;  listCount = 4; }
        else if (data.currentPage == PAGE_SETTING_COOLING) { listPtr = coolingSettingIndices; listCount = 3; }
        else if (data.currentPage == PAGE_SETTING_JIG)     { listPtr = jigSettingIndices;     listCount = 4; }

        int maxCardX = (listCount > 3) ? 264 : 312;
        if (!handleRightScrollTouch(tx, ty, listCount, data) && tx >= 8 && tx <= maxCardX) {
          int slot = getCardSlotFromY(ty);
          int itemIdx = data.scrollOffset + slot;
          if (slot >= 0 && itemIdx < listCount) {
            data.activeSetting = listPtr[itemIdx];
            data.numpadStr = "";
            data.showNumpad = true;
            data.pageChanged = true;
          }
        }
      }
      // 12. Scale & Collision Settings Sub-Page (3 cards: Col. Limit, Weight Dwell, Load Cell Calibrate Wizard)
      else if (data.currentPage == PAGE_SETTING_SENSOR) {
        int slot = getCardSlotFromY(ty);
        if (slot == 0) { // Card 0: Col. Limit (s8)
          data.activeSetting = 8;
          data.numpadStr = "";
          data.showNumpad = true;
          data.pageChanged = true;
        } else if (slot == 1) { // Card 1: Weight Dwell (s10)
          data.activeSetting = 10;
          data.numpadStr = "";
          data.showNumpad = true;
          data.pageChanged = true;
        } else if (slot == 2) { // Card 2: Open Load Cell Calibration Wizard
          data.calStatusText = "Step 1: Unload scale & tap ZERO";
          setCurrentPage(PAGE_SCALE_CAL);
        }
      }
      // 13. Guided Scale Calibration Wizard Page
      else if (data.currentPage == PAGE_SCALE_CAL) {
        if (tx >= 10 && tx <= 310) {
          if (ty >= 112 && ty <= 150) { // Button 1: ZERO UNLOADED SCALE (RAM only)
            data.calStatusText = "Zeroed! Now hang weight & tap Step 2";
            data.pageChanged = true;
            eventTriggered = UI_EVENT_MANUAL_TARE;
          } else if (ty >= 156 && ty <= 194) { // Button 2: SET KNOWN WEIGHT (Saved to NVS)
            data.activeSetting = 23; // Known Weight (g)
            data.numpadStr = "";
            data.showNumpad = true;
            data.pageChanged = true;
          }
        }
      }
      // 14. Motor Settings Sub-Page (3 cards: Driver Mode, Hybrid Speed, Motor Power)
      else if (data.currentPage == PAGE_SETTING_MOTOR) {
        int slot = getCardSlotFromY(ty);
        if (slot == 0) { // Card 0: Driver Mode Toggle
          data.s_tmcMode = (data.s_tmcMode + 1) % 3;
          data.activeSetting = 12;
          data.pageChanged = true;
          eventTriggered = UI_EVENT_SETTING_UPDATED;
        } else if (slot == 1) { // Card 1: Hybrid Speed Threshold Numpad
          data.activeSetting = 13;
          data.numpadStr = "";
          data.showNumpad = true;
          data.pageChanged = true;
        } else if (slot == 2) { // Card 2: Motor Power Toggle
          data.isMotorEnabled = !data.isMotorEnabled;
          data.pageChanged = true;
          eventTriggered = UI_EVENT_MOTOR_ENABLE_TOGGLE;
        }
      }
      // 15. Screen Settings Sub-Page (Brightness slider saves on release; Theme card cycles palette)
      else if (data.currentPage == PAGE_SETTING_SCREEN) {
        if (ty >= 50 && ty <= 105 && tx >= 160 && tx <= 310) {
          int bVal = map(tx, 164, 304, 1, 99);
          setBrightness(bVal);
          data.pageChanged = true;
          eventTriggered = UI_EVENT_BRIGHTNESS_CHANGED;
        } else if (ty >= 120 && ty <= 172) {
          int nextTheme = (data.s_theme + 1) % 3;
          applyTheme(nextTheme);
          data.pageChanged = true;
          eventTriggered = UI_EVENT_THEME_CHANGED;
        }
      }
      // 16. Sound & Tunes Settings Sub-Page (3-card vertical scroll + live preview on tap)
      else if (data.currentPage == PAGE_SETTING_SOUND) {
        if (!handleRightScrollTouch(tx, ty, 4, data) && tx >= 8 && tx <= 264) {
          int slot = getCardSlotFromY(ty);
          int itemIdx = data.scrollOffset + slot;
          if (slot >= 0 && itemIdx < 4) {
            if (itemIdx == 0) { // Item 0: Volume (drag bar on right or tap left to step by 20%)
              if (tx >= 145 && tx <= 262) {
                data.s_volume = constrain((int)map(tx, 150, 256, 0, 100), 0, 100);
              } else {
                data.s_volume = (data.s_volume >= 100) ? 0 : min(100, ((data.s_volume / 20) + 1) * 20);
              }
              data.pageChanged = true;
              eventTriggered = UI_EVENT_VOLUME_CHANGED;
            } else if (itemIdx == 1) { // Item 1: Startup Tune (cycle & preview)
              data.s_startupTune = (data.s_startupTune + 1) % STARTUP_TUNE_COUNT;
              data.pageChanged = true;
              eventTriggered = UI_EVENT_STARTUP_TUNE_CHANGED;
            } else if (itemIdx == 2) { // Item 2: Finish Tune (cycle & preview)
              data.s_finishTune = (data.s_finishTune + 1) % FINISH_TUNE_COUNT;
              data.pageChanged = true;
              eventTriggered = UI_EVENT_FINISH_TUNE_CHANGED;
            } else if (itemIdx == 3) { // Item 3: Alert / Error Tune (cycle & preview)
              data.s_alertTune = (data.s_alertTune + 1) % ALERT_TUNE_COUNT;
              data.pageChanged = true;
              eventTriggered = UI_EVENT_ALERT_TUNE_CHANGED;
            }
          }
        }
      }
      // 18. Manual Control Page Buttons
      else if (data.currentPage == PAGE_MANUAL) {
        if (data.justStoppedByTouch) {
          data.justStoppedByTouch = false;
          data.isUpPressed = false;
          data.isDownPressed = false;
          data.pageChanged = true;
          return UI_EVENT_NONE;
        }
        if (tx >= 36 && tx <= 156 && ty >= 41 && ty <= 91) { // UP arrow button
          data.isUpPressed = !data.isUpPressed;
          if (data.isUpPressed) data.isDownPressed = false;
          data.pageChanged = true;
        }
        else if (tx >= 36 && tx <= 156 && ty >= 149 && ty <= 199) { // DOWN arrow button
          data.isDownPressed = !data.isDownPressed;
          if (data.isDownPressed) data.isUpPressed = false;
          data.pageChanged = true;
        }
        else if (tx >= 197 && tx <= 315 && ty >= 41 && ty <= 87) { // HOME / STOP button
          data.isUpPressed = false; data.isDownPressed = false;
          data.pageChanged = true; 
          eventTriggered = data.isHomingActive ? UI_EVENT_MANUAL_STOP : UI_EVENT_MANUAL_HOME;
        }
        else if (tx >= 197 && tx <= 315 && ty >= 97 && ty <= 143) { // DIP BOT / STOP button
          data.isUpPressed = false; data.isDownPressed = false;
          data.pageChanged = true; 
          eventTriggered = data.isDipBotActive ? UI_EVENT_MANUAL_STOP : UI_EVENT_MANUAL_DIP_BOT;
        }
        else if (tx >= 197 && tx <= 315 && ty >= 153 && ty <= 199) { // TARE button
          data.isUpPressed = false; data.isDownPressed = false;
          data.currentWeight = 0.0f; 
          data.pageChanged = true; 
          eventTriggered = UI_EVENT_MANUAL_TARE;
        }
        else if (tx >= 10 && tx <= 55 && ty >= 100 && ty <= 140) { // Speed "-" button
          data.manualSpeed -= 5; 
          if (data.manualSpeed < 5) data.manualSpeed = 5; 
          data.pageChanged = true; 
        }
        else if (tx >= 137 && tx <= 182 && ty >= 100 && ty <= 140) { // Speed "+" button
          data.manualSpeed += 5; 
          if (data.manualSpeed > 100) data.manualSpeed = 100; 
          data.pageChanged = true; 
        }
        else { // Tapping empty space on Manual page stops any active jog
          data.isUpPressed = false;
          data.isDownPressed = false;
          data.pageChanged = true;
        }
      }
    }
  }

  return eventTriggered;
}


