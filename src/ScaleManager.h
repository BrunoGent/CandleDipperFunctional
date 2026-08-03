#ifndef SCALE_MANAGER_H
#define SCALE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "PinConfig.h"

class ScaleManager {
public:
  ScaleManager();

  void begin(Preferences* prefs = nullptr);
  void update(); // Non-blocking periodic poll

  // Tare & Calibration
  void tare(uint8_t samples = 10);
  void calibrate(float knownWeightGrams, uint8_t samples = 10);

  // Getters & Setters
  float getWeightGrams() const { return _currentWeightGrams; }
  long getRawValue() const { return _rawReadout; }
  void setScaleFactor(float factor);
  float getScaleFactor() const { return _scaleFactor; }
  long getZeroOffset() const { return _zeroOffset; }

  // Load cell hardware polling
  long readRawBitbang();

private:
  Preferences* _prefs;
  long _zeroOffset;
  float _scaleFactor;
  float _currentWeightGrams;
  long _rawReadout;
  unsigned long _lastPollMs;
};

#endif // SCALE_MANAGER_H
