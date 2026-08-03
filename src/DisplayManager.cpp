#include "DisplayManager.h"

// Setting metadata arrays
static const char* settingNames[15] = {
  "Slim Weight", "Std Weight", "Slim Dips", "Std Dips",
  "1st Dip Time", "+ Dips Time", "Up Speed", "Down Speed",
  "Col. Limit", "Soft Limit", "Weight Dwell", "Soft Ramp",
  "Driver Mode", "Hybrid Speed", "Auto Dip Timer"
};
static const char* settingUnits[15] = {
  "g", "g", "dips", "dips",
  "s", "s", "mm/s", "mm/s",
  "g", "mm", "ms", "ms",
  "", "mm/s", "s"
};
static const char* themeNames[3] = { "Original", "Bee Mine", "Dark Mode" };

DisplayManager::DisplayManager() : _display(&M5.Display), canvas(&M5.Display) {
  // Constructor
}

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

void DisplayManager::applyTheme(int themeId) {
  data.s_theme = themeId;
  if (data.s_theme == 0) { // Original (Classic Navy & Maroon)
    c_bg = TFT_BLACK; 
    c_banner = TFT_DARKCYAN; 
    c_bannerTxt = TFT_WHITE;
    c_btn1 = TFT_NAVY; 
    c_btn2 = TFT_MAROON; 
    c_btnTxt = TFT_WHITE;
    c_active = TFT_DARKGREEN; 
    c_outline = TFT_DARKGREY;
  } else if (data.s_theme == 1) { // Bee Mine Pastel (Website Match)
    c_bg = canvas.color565(35, 35, 35);           // Charcoal
    c_banner = canvas.color565(255, 235, 130);    // Soft Pastel Yellow
    c_bannerTxt = TFT_BLACK;
    c_btn1 = canvas.color565(255, 235, 130);      // Soft Yellow
    c_btn2 = canvas.color565(140, 210, 180);      // Mint Accent
    c_btnTxt = TFT_BLACK;
    c_active = canvas.color565(130, 230, 130);    // Light Green
    c_outline = canvas.color565(80, 80, 80);
  } else { // Dark Mode
    c_bg = TFT_BLACK; 
    c_banner = canvas.color565(40, 40, 40); 
    c_bannerTxt = TFT_LIGHTGREY;
    c_btn1 = canvas.color565(60, 60, 60); 
    c_btn2 = canvas.color565(60, 60, 60); 
    c_btnTxt = TFT_WHITE;
    c_active = canvas.color565(80, 160, 80); 
    c_outline = canvas.color565(100, 100, 100);
  }
}

void DisplayManager::setBrightness(int percent) {
  data.s_brightness = constrain(percent, 1, 99);
  _display->setBrightness(map(data.s_brightness, 1, 99, 10, 255));
}

const char* DisplayManager::getPageTitle() const {
  switch (data.currentPage) {
    case PAGE_WEIGHT_DIP:     return "Weight-Based";
    case PAGE_DIPS_DIP:       return "Dips-Based";
    case PAGE_MANUAL:         return "Manual Control";
    case PAGE_SETTINGS_HUB:   return "Settings (1/2)";
    case PAGE_SETTINGS_HUB_2: return "Settings (2/2)";
    case PAGE_SETTING_DIPPING:return "Dipping Settings";
    case PAGE_SETTING_MOTION: return "Motion Settings";
    case PAGE_SETTING_SENSOR: return "Sensor Settings";
    case PAGE_SETTING_SCREEN: return "Screen Settings";
    case PAGE_SETTING_MOTOR:  return "Motor Settings";
    case PAGE_WIFI_PORTAL:    return "Wi-Fi Portal";
    default:                  return "Settings";
  }
}

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
     case 11: return data.s_softRamp;
     case 12: return data.s_tmcMode;
     case 13: return data.s_tmcThreshold;
     case 14: return data.s_autoDipTimer;
  }
  return 0;
}

void DisplayManager::setSettingValue(int index, int val) {
  switch(index) {
     case 0: data.s_slimWt = val; break;
     case 1: data.s_stdWt = val; break;
     case 2: data.s_slimDips = val; break;
     case 3: data.s_stdDips = val; break;
     case 4: data.s_dip1Time = val; break;
     case 5: data.s_subDipTime = val; break;
     case 6: data.s_upSpeed = val; break;
     case 7: data.s_downSpeed = val; break;
     case 8: data.s_colLimit = -abs(val); break;
     case 9: data.s_softLimit = abs(val); break;
     case 10: data.s_weightDwell = abs(val); break;
     case 11: data.s_softRamp = abs(val); break;
     case 12: data.s_tmcMode = (val < 0) ? 0 : (val % 3); break;
     case 13: data.s_tmcThreshold = abs(val); break;
     case 14: data.s_autoDipTimer = abs(val); break;
  }
}

void DisplayManager::formatTime(int totalSeconds, char* buffer, int bufferSize) const {
  int m = totalSeconds / 60;
  int s = totalSeconds % 60;
  snprintf(buffer, bufferSize, "%02d:%02d", m, s);
}

void DisplayManager::drawButton(int x, int y, int w, int h, const char* label, uint32_t color, uint32_t txtColor) {
  canvas.fillRoundRect(x, y, w, h, 10, color);
  canvas.setTextColor(txtColor, color);
  canvas.setTextDatum(middle_center);
  canvas.drawString(label, x + (w / 2), y + (h / 2));
}

void DisplayManager::drawTopBanner() {
  int bannerHeight = canvas.height() * 0.15;
  canvas.fillRect(0, 0, canvas.width(), bannerHeight, c_banner);
  canvas.setTextColor(c_bannerTxt, c_banner);
  canvas.setTextSize(2.0);

  if (data.currentPage == PAGE_MANUAL) {
      char posStr[32]; char wtStr[32];
      snprintf(posStr, sizeof(posStr), "POS: %d mm", (int)data.currentPosition);
      snprintf(wtStr, sizeof(wtStr), "WT: %d g", (int)data.currentWeight);
      canvas.setTextDatum(middle_left); 
      canvas.drawString(posStr, 10, bannerHeight / 2);
      canvas.setTextDatum(middle_right); 
      canvas.drawString(wtStr, canvas.width() - 10, bannerHeight / 2);
  } else if (data.currentPage == PAGE_ACTIVE_DIP || data.currentPage == PAGE_STOP_CONFIRM || data.currentPage == PAGE_DIP_DONE) {
      canvas.setTextDatum(middle_center);
      if (data.currentPage == PAGE_ACTIVE_DIP) canvas.drawString("DIPPING IN PROGRESS", 160, bannerHeight / 2);
      else if (data.currentPage == PAGE_STOP_CONFIRM) canvas.drawString("CONFIRM ABORT", 160, bannerHeight / 2);
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
      
      snprintf(dateStr, sizeof(dateStr), "%s %02d/%02d/%02d", shortDays[wd], dt.date.date, dt.date.month, dt.date.year % 100);
      snprintf(timeStr, sizeof(timeStr), "%02d:%02d", dt.time.hours, dt.time.minutes);
      
      canvas.setTextDatum(middle_left); canvas.drawString(dateStr, 10, bannerHeight / 2);
      canvas.setTextDatum(middle_right); canvas.drawString(timeStr, 310, bannerHeight / 2);
  }
}

void DisplayManager::drawBottomBanner() {
  if (data.currentPage == PAGE_ACTIVE_DIP || data.currentPage == PAGE_STOP_CONFIRM || data.currentPage == PAGE_DIP_DONE) return;
  int bannerHeight = canvas.height() * 0.15;
  int startY = canvas.height() - bannerHeight;
  canvas.fillRect(0, startY, canvas.width(), bannerHeight, c_outline);
  canvas.setTextColor(TFT_WHITE, c_outline);
  canvas.setTextDatum(middle_center);
  
  if (data.currentPage == PAGE_WIFI_PORTAL) {
      canvas.drawString("< BACK TO HUB", 160, startY + (bannerHeight / 2));
  } else if (data.currentPage == PAGE_SETTINGS_HUB) {
      canvas.setTextDatum(middle_left);
      canvas.drawString("< MAIN MENU", 15, startY + (bannerHeight / 2));
      canvas.setTextDatum(middle_right);
      canvas.drawString("PAGE 2 >", 305, startY + (bannerHeight / 2));
  } else if (data.currentPage == PAGE_SETTINGS_HUB_2) {
      canvas.setTextDatum(middle_left);
      canvas.drawString("< PAGE 1", 15, startY + (bannerHeight / 2));
      canvas.setTextDatum(middle_right);
      canvas.drawString("MAIN MENU >", 305, startY + (bannerHeight / 2));
  } else if ((data.currentPage >= PAGE_SETTING_DIPPING && data.currentPage <= PAGE_SETTING_SCREEN) || data.currentPage == PAGE_SETTING_MOTOR) {
      canvas.drawString("< BACK TO HUB", 160, startY + (bannerHeight / 2));
  } else if (data.currentSubState == SUB_MAIN) {
      char bannerText[32]; snprintf(bannerText, sizeof(bannerText), "< %s >", getPageTitle());
      canvas.drawString(bannerText, 160, startY + (bannerHeight / 2));
  } else {
      canvas.drawString("CANCEL", 160, startY + (bannerHeight / 2));
  }
}

void DisplayManager::drawPage1_2() {
  canvas.fillScreen(c_bg);
  drawTopBanner(); drawBottomBanner();
  bool isWt = (data.currentPage == PAGE_WEIGHT_DIP);
  if (data.currentSubState == SUB_MAIN) {
      drawButton(10, 70, 140, 100, "SLIM", c_btn1, c_btnTxt);
      drawButton(170, 70, 140, 100, "STANDARD", c_btn2, c_btnTxt);
  } else if (data.currentSubState == SUB_CONFIRM_SLIM) {
      drawButton(10, 70, 300, 100, isWt ? "CONFIRM SLIM (Wt)" : "CONFIRM SLIM (Dips)", c_active, TFT_BLACK);
  } else if (data.currentSubState == SUB_CONFIRM_STD) {
      drawButton(10, 70, 300, 100, isWt ? "CONFIRM STD (Wt)" : "CONFIRM STD (Dips)", c_active, TFT_BLACK);
  }
}

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

void DisplayManager::drawSettingsHub() {
  canvas.fillScreen(c_bg);
  drawTopBanner(); drawBottomBanner();
  
  drawButton(10, 45, 145, 65, "Dipping", c_btn1, c_btnTxt);
  drawButton(165, 45, 145, 65, "Motion", c_btn1, c_btnTxt);
  drawButton(10, 125, 145, 65, "Sensor", c_btn1, c_btnTxt);
  drawButton(165, 125, 145, 65, "Screen", c_btn2, c_btnTxt);
}

void DisplayManager::drawSettingsHub2() {
  canvas.fillScreen(c_bg);
  drawTopBanner(); drawBottomBanner();
  
  drawButton(10, 45, 145, 65, "Motor", c_btn1, c_btnTxt);
  drawButton(165, 45, 145, 65, "Wi-Fi", c_btn2, c_btnTxt);

  // Reserved card slots
  canvas.fillRoundRect(10, 125, 145, 65, 10, c_outline);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(TFT_DARKGREY, c_outline);
  canvas.drawString("--", 82, 157);

  canvas.fillRoundRect(165, 125, 145, 65, 10, c_outline);
  canvas.drawString("--", 237, 157);
}

void DisplayManager::drawMotorPage() {
  canvas.fillScreen(c_bg);
  drawTopBanner(); drawBottomBanner();

  static const char* modeLabels[3] = { "StealthChop", "SpreadCycle", "Adaptive" };

  // Row 0: Driver Mode
  int y0 = 45;
  canvas.setTextDatum(middle_left);
  canvas.setTextColor(c_bannerTxt == TFT_BLACK ? TFT_WHITE : c_btnTxt, c_bg);
  canvas.drawString("Driver Mode", 10, y0 + 15);
  canvas.fillRoundRect(160, y0, 150, 32, 5, c_btn1);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(c_btnTxt, c_btn1);
  canvas.drawString(modeLabels[data.s_tmcMode % 3], 235, y0 + 16);

  // Row 1: Speed Threshold (for Adaptive mode)
  int y1 = 83;
  canvas.setTextDatum(middle_left);
  canvas.setTextColor(c_bannerTxt == TFT_BLACK ? TFT_WHITE : c_btnTxt, c_bg);
  canvas.drawString("Hybrid Speed", 10, y1 + 15);
  uint16_t threshBg = (data.s_tmcMode == 2) ? c_btn1 : c_outline;
  uint16_t threshTxt = (data.s_tmcMode == 2) ? c_btnTxt : TFT_DARKGREY;
  canvas.fillRoundRect(160, y1, 150, 32, 5, threshBg);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(threshTxt, threshBg);
  String threshStr = String(data.s_tmcThreshold) + " mm/s";
  canvas.drawString(threshStr.c_str(), 235, y1 + 16);

  // Row 2: Motor Power Toggle
  int y2 = 121;
  canvas.setTextDatum(middle_left);
  canvas.setTextColor(c_bannerTxt == TFT_BLACK ? TFT_WHITE : c_btnTxt, c_bg);
  canvas.drawString("Motor Power", 10, y2 + 15);
  uint16_t btnColor = data.isMotorEnabled ? canvas.color565(40, 140, 40) : canvas.color565(160, 40, 40);
  canvas.fillRoundRect(160, y2, 150, 32, 5, btnColor);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(TFT_WHITE, btnColor);
  canvas.drawString(data.isMotorEnabled ? "ENABLED" : "DISABLED", 235, y2 + 16);
}

void DisplayManager::drawSettingsList(int startIndex, int count) {
  canvas.fillScreen(c_bg);
  drawTopBanner(); drawBottomBanner();
  
  for (int i = 0; i < count; i++) {
      int index = startIndex + i;
      int yPos = 45 + (i * 38); 
      canvas.setTextDatum(middle_left);
      canvas.setTextColor(c_bannerTxt == TFT_BLACK ? TFT_WHITE : c_btnTxt, c_bg); 
      canvas.drawString(settingNames[index], 10, yPos + 15);
      
      canvas.fillRoundRect(160, yPos, 150, 32, 5, c_outline);
      canvas.setTextDatum(middle_center);
      canvas.setTextColor(TFT_WHITE, c_outline);
      String valStr = String(getSettingValue(index)) + " " + settingUnits[index];
      canvas.drawString(valStr.c_str(), 235, yPos + 16);
  }
}

void DisplayManager::drawScreenDashboard() {
  canvas.fillScreen(c_bg);
  drawTopBanner(); drawBottomBanner();
  
  // Row 0: Brightness
  int y0 = 45;
  canvas.setTextDatum(middle_left);
  canvas.setTextColor(c_bannerTxt == TFT_BLACK ? TFT_WHITE : c_btnTxt, c_bg);
  canvas.drawString("Brightness", 10, y0 + 15);
  
  canvas.fillRoundRect(160, y0, 150, 32, 5, c_outline);
  int fillWidth = map(data.s_brightness, 1, 99, 10, 150);
  canvas.fillRoundRect(160, y0, fillWidth, 32, 5, c_active);
  String bStr = String(data.s_brightness) + "%";
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(TFT_WHITE, c_active);
  canvas.drawString(bStr.c_str(), 235, y0 + 16);

  // Row 1: Color Theme
  int y1 = 83;
  canvas.setTextDatum(middle_left);
  canvas.setTextColor(c_bannerTxt == TFT_BLACK ? TFT_WHITE : c_btnTxt, c_bg);
  canvas.drawString("Color Theme", 10, y1 + 15);
  canvas.fillRoundRect(160, y1, 150, 32, 5, c_btn1);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(c_btnTxt, c_btn1);
  canvas.drawString(themeNames[data.s_theme], 235, y1 + 16);

  // Row 2: Auto Dip Timer
  int y2 = 121;
  canvas.setTextDatum(middle_left);
  canvas.setTextColor(c_bannerTxt == TFT_BLACK ? TFT_WHITE : c_btnTxt, c_bg);
  canvas.drawString("Auto Dip Timer", 10, y2 + 15);
  canvas.fillRoundRect(160, y2, 150, 32, 5, c_outline);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(TFT_WHITE, c_outline);
  String timerStr = (data.s_autoDipTimer == 0) ? "Disabled" : (String(data.s_autoDipTimer) + " s");
  canvas.drawString(timerStr.c_str(), 235, y2 + 16);
}

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

void DisplayManager::drawNumpad() {
  canvas.fillRect(20, 30, 280, 195, c_banner);
  canvas.setTextDatum(top_center);
  canvas.setTextColor(c_bannerTxt, c_banner);
  canvas.drawString(settingNames[data.activeSetting], 160, 35);
  
  canvas.fillRect(30, 55, 260, 30, TFT_BLACK);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.drawString(data.numpadStr.c_str(), 160, 70);
  
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

static void formatPrettyDuration(int totalSeconds, char* buffer, size_t bufferSize) {
  int m = totalSeconds / 60;
  int s = totalSeconds % 60;
  if (m > 0) {
    snprintf(buffer, bufferSize, "%dm %ds", m, s);
  } else {
    snprintf(buffer, bufferSize, "%ds", s);
  }
}

static int getProcessTypeIndex(bool isWeight, bool isSlim) {
  if (isWeight && isSlim) return 0;       // Slim Weight
  if (isWeight && !isSlim) return 1;      // Std Weight
  if (!isWeight && isSlim) return 2;      // Slim Dips
  return 3;                               // Std Dips
}

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

  // Display "(est)" next to estimated total time as requested
  char timeStr[64]; snprintf(timeStr, sizeof(timeStr), "Time: %s / %s (est)", elapsedStr, estStr);
  char targetStr[64];
  if (data.isActiveWeightBased) {
    snprintf(targetStr, sizeof(targetStr), "Weight: %dg / %dg", (int)data.currentWeight, (data.isActiveSlimProfile ? data.s_slimWt : data.s_stdWt));
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

void DisplayManager::drawStopConfirmPage() {
  canvas.fillScreen(c_bg); 
  drawTopBanner();
  canvas.setTextDatum(middle_center); 
  canvas.setTextColor(TFT_WHITE, c_bg);
  canvas.drawString("Abort current dipping?", 160, 80);
  drawButton(20, 150, 130, 70, "NO", c_outline, TFT_WHITE); 
  drawButton(170, 150, 130, 70, "YES", TFT_RED, TFT_WHITE);
}

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

void DisplayManager::renderCurrentPage() {
  if (!data.pageChanged) return;

  switch (data.currentPage) {
    case PAGE_WEIGHT_DIP: 
    case PAGE_DIPS_DIP: 
      drawPage1_2(); 
      break;
    case PAGE_MANUAL: 
      drawManualPage(); 
      break;
    case PAGE_SETTINGS_HUB: 
      drawSettingsHub(); 
      break;
    case PAGE_SETTINGS_HUB_2:
      drawSettingsHub2();
      break;
    case PAGE_SETTING_DIPPING: 
      if (data.showNumpad) drawNumpad(); else drawSettingsList(0, 4); 
      break;
    case PAGE_SETTING_MOTION: 
      if (data.showNumpad) drawNumpad(); else drawSettingsList(4, 4); 
      break;
    case PAGE_SETTING_SENSOR: 
      if (data.showNumpad) drawNumpad(); else drawSettingsList(8, 4); 
      break;
    case PAGE_SETTING_SCREEN: 
      drawScreenDashboard(); 
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

void DisplayManager::startDippingProcess(bool isWeight, bool isSlim) {
  data.isActiveWeightBased = isWeight; 
  data.isActiveSlimProfile = isSlim;
  data.dipStartTime = millis(); 
  data.dipWasAborted = false; 
  data.currentDipCount = 0;
  data.finalElapsedSeconds = 0;
  data.currentPhaseText = "Starting process...";

  int avg = getAverageTime(isWeight, isSlim);
  if (avg > 0) {
    data.estimatedTotalSeconds = avg;
  } else {
    int targetDips = isSlim ? data.s_slimDips : data.s_stdDips;
    data.estimatedTotalSeconds = targetDips * (data.s_dip1Time + data.s_subDipTime + 6);
  }

  data.currentPage = PAGE_ACTIVE_DIP; 
  data.pageChanged = true;
}

void DisplayManager::endDippingProcess(bool aborted, int elapsedSeconds) {
  data.dipWasAborted = aborted; 
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

UiEvent DisplayManager::updateTouch() {
  UiEvent eventTriggered = UI_EVENT_NONE;

  data.isUpPressed = false;
  data.isDownPressed = false;

  if (M5.Touch.getCount() > 0) {
    auto touch = M5.Touch.getDetail();

    // Brightness bar slider adjustment
    if (data.currentPage == PAGE_SETTING_SCREEN && touch.isPressed() && !data.showNumpad) {
      if (touch.y >= 60 && touch.y <= 110 && touch.x >= 40 && touch.x <= 280) {
        int bVal = map(touch.x, 40, 280, 1, 99);
        setBrightness(bVal);
        data.pageChanged = true;
        eventTriggered = UI_EVENT_BRIGHTNESS_CHANGED;
      }
    }

    if (touch.wasReleased()) {
      int tx = touch.x; int ty = touch.y;
      int bottomStartY = M5.Display.height() - (M5.Display.height() * 0.15);

      if (data.currentPage == PAGE_WIFI_PORTAL) {
        if (ty > bottomStartY) {
          eventTriggered = UI_EVENT_STOP_WIFI_PORTAL;
        }
      }
      else if (data.currentPage == PAGE_ACTIVE_DIP) { 
        data.currentPage = PAGE_STOP_CONFIRM; 
        data.pageChanged = true; 
      }
      else if (data.currentPage == PAGE_STOP_CONFIRM) {
        if (tx >= 20 && tx <= 150 && ty >= 150 && ty <= 220) { 
          data.currentPage = PAGE_ACTIVE_DIP; 
          data.pageChanged = true; 
          data.lastUiTick = millis(); 
        } 
        else if (tx >= 170 && tx <= 300 && ty >= 150 && ty <= 220) {
          endDippingProcess(true);
          eventTriggered = UI_EVENT_ABORT_DIP;
        }
      }
      else if (data.currentPage == PAGE_DIP_DONE) {
        if (ty >= 144) { 
          data.currentPage = data.isActiveWeightBased ? PAGE_WEIGHT_DIP : PAGE_DIPS_DIP; 
          data.currentSubState = SUB_MAIN; 
          data.pageChanged = true; 
          eventTriggered = UI_EVENT_FINISH_DIP;
        }
      }
      else if (ty > bottomStartY && (data.currentPage >= PAGE_SETTING_DIPPING && data.currentPage <= PAGE_SETTING_SCREEN)) {
        data.currentPage = PAGE_SETTINGS_HUB; 
        data.pageChanged = true;
      }
      else if (ty > bottomStartY && data.currentPage == PAGE_SETTING_MOTOR) {
        data.currentPage = PAGE_SETTINGS_HUB_2;
        data.pageChanged = true;
      }
      else if (ty > bottomStartY && data.currentPage == PAGE_SETTINGS_HUB) {
        if (tx < 160) data.currentPage = PAGE_WEIGHT_DIP;
        else data.currentPage = PAGE_SETTINGS_HUB_2;
        data.pageChanged = true;
      }
      else if (ty > bottomStartY && data.currentPage == PAGE_SETTINGS_HUB_2) {
        if (tx < 160) data.currentPage = PAGE_SETTINGS_HUB;
        else data.currentPage = PAGE_WEIGHT_DIP;
        data.pageChanged = true;
      }
      else if (ty > bottomStartY && !data.showNumpad) {
        if (data.currentSubState != SUB_MAIN) { 
          data.currentSubState = SUB_MAIN; 
          data.pageChanged = true; 
        } 
        else {
          if (tx < 160) data.currentPage = static_cast<AppState>((data.currentPage + 3) % 4);
          else data.currentPage = static_cast<AppState>((data.currentPage + 1) % 4);
          data.pageChanged = true;
        }
      } 
      else if (data.showNumpad) {
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
          if (ty >= 90 && ty <= 143) { 
            data.showNumpad = false; 
            data.pageChanged = true; 
          } 
          else if (ty >= 146 && ty <= 199) { 
            int val = data.numpadStr.toInt();
            setSettingValue(data.activeSetting, val);
            data.showNumpad = false; 
            data.pageChanged = true;
            eventTriggered = UI_EVENT_SETTING_UPDATED;
          }
        }
      }
      else if (data.currentPage == PAGE_SETTINGS_HUB) {
        if (ty >= 45 && ty <= 110) {
          if (tx < 160) data.currentPage = PAGE_SETTING_DIPPING;
          else data.currentPage = PAGE_SETTING_MOTION;
          data.pageChanged = true;
        } else if (ty >= 125 && ty <= 190) {
          if (tx < 160) data.currentPage = PAGE_SETTING_SENSOR;
          else data.currentPage = PAGE_SETTING_SCREEN;
          data.pageChanged = true;
        }
      }
      else if (data.currentPage == PAGE_SETTINGS_HUB_2) {
        if (ty >= 45 && ty <= 110) {
          if (tx < 160) {
            data.currentPage = PAGE_SETTING_MOTOR;
            data.pageChanged = true;
          } else {
            data.currentPage = PAGE_WIFI_PORTAL;
            data.pageChanged = true;
            eventTriggered = UI_EVENT_START_WIFI_PORTAL;
          }
        }
      }
      else if (data.currentPage == PAGE_SETTING_MOTOR) {
        if (tx >= 160 && tx <= 310) {
          if (ty >= 45 && ty <= 77) { // Driver Mode Toggle
            data.s_tmcMode = (data.s_tmcMode + 1) % 3;
            data.pageChanged = true;
            eventTriggered = UI_EVENT_SETTING_UPDATED;
          } else if (ty >= 83 && ty <= 115) { // Hybrid Speed Threshold Numpad
            data.activeSetting = 13;
            data.numpadStr = "";
            data.showNumpad = true;
            data.pageChanged = true;
          } else if (ty >= 121 && ty <= 153) { // Motor Power Toggle
            data.isMotorEnabled = !data.isMotorEnabled;
            data.pageChanged = true;
            eventTriggered = UI_EVENT_MOTOR_ENABLE_TOGGLE;
          }
        }
      }
      else if (data.currentPage >= PAGE_SETTING_DIPPING && data.currentPage <= PAGE_SETTING_SENSOR) {
        if (tx >= 160 && tx <= 310) {
          int row = (ty - 45) / 38;
          if (row >= 0 && row < 4) {
            if (data.currentPage == PAGE_SETTING_DIPPING) data.activeSetting = 0 + row;
            else if (data.currentPage == PAGE_SETTING_MOTION) data.activeSetting = 4 + row;
            else if (data.currentPage == PAGE_SETTING_SENSOR) data.activeSetting = 8 + row;
            
            if (data.activeSetting <= 14) {
              data.numpadStr = ""; 
              data.showNumpad = true; 
              data.pageChanged = true;
            }
          }
        }
      }
      else if (data.currentPage == PAGE_SETTING_SCREEN) {
        if (tx >= 160 && tx <= 310) {
          if (ty >= 45 && ty <= 77) { // Brightness adjustment
            int newB = data.s_brightness + 25;
            if (newB > 100) newB = 25;
            setBrightness(newB);
            data.pageChanged = true;
            eventTriggered = UI_EVENT_SETTING_UPDATED;
          } else if (ty >= 83 && ty <= 115) { // Theme cycle
            int nextTheme = (data.s_theme + 1) % 3;
            applyTheme(nextTheme);
            data.pageChanged = true;
            eventTriggered = UI_EVENT_THEME_CHANGED;
          } else if (ty >= 121 && ty <= 153) { // Auto Dip Timer Numpad
            data.activeSetting = 14;
            data.numpadStr = "";
            data.showNumpad = true;
            data.pageChanged = true;
          }
        }
      }
      else if (data.currentPage == PAGE_WEIGHT_DIP || data.currentPage == PAGE_DIPS_DIP) {
        if (data.currentSubState == SUB_MAIN) {
          if (tx >= 10 && tx <= 150 && ty >= 70 && ty <= 170) { 
            data.currentSubState = SUB_CONFIRM_SLIM; 
            data.pageChanged = true; 
          } 
          else if (tx >= 170 && tx <= 310 && ty >= 70 && ty <= 170) { 
            data.currentSubState = SUB_CONFIRM_STD; 
            data.pageChanged = true; 
          }
        } else {
          if (tx >= 10 && tx <= 310 && ty >= 70 && ty <= 170) {
            startDippingProcess((data.currentPage == PAGE_WEIGHT_DIP), (data.currentSubState == SUB_CONFIRM_SLIM));
            eventTriggered = UI_EVENT_START_DIP;
          }
        }
      }
      else if (data.currentPage == PAGE_MANUAL) {
        if (data.justStoppedByTouch) {
          data.justStoppedByTouch = false;
          data.pageChanged = true;
          return UI_EVENT_NONE;
        }
        if (tx >= 197 && tx <= 315 && ty >= 41 && ty <= 87) { 
          data.pageChanged = true; 
          eventTriggered = data.isHomingActive ? UI_EVENT_MANUAL_STOP : UI_EVENT_MANUAL_HOME;
        }
        else if (tx >= 197 && tx <= 315 && ty >= 97 && ty <= 143) { 
          data.pageChanged = true; 
          eventTriggered = data.isDipBotActive ? UI_EVENT_MANUAL_STOP : UI_EVENT_MANUAL_DIP_BOT;
        }
        else if (tx >= 197 && tx <= 315 && ty >= 153 && ty <= 199) { 
          data.currentWeight = 0.0; 
          data.pageChanged = true; 
          eventTriggered = UI_EVENT_MANUAL_TARE;
        }
        else if (tx >= 10 && tx <= 55 && ty >= 100 && ty <= 140) { 
          data.manualSpeed -= 5; 
          if (data.manualSpeed < 5) data.manualSpeed = 5; 
          data.pageChanged = true; 
        }
        else if (tx >= 137 && tx <= 182 && ty >= 100 && ty <= 140) { 
          data.manualSpeed += 5; 
          if (data.manualSpeed > 100) data.manualSpeed = 100; 
          data.pageChanged = true; 
        }
      }
    }
  }

  return eventTriggered;
}
