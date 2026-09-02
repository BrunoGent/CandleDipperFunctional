#ifndef SCALE_MANAGER_H
#define SCALE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "PinConfig.h"

/**
 * ============================================================================
 * SCALE MANAGER COMPONENT
 * ============================================================================
 * Manages the high-precision HX711 24-bit ADC load cell amplifier.
 * 
 * Hardware Architecture:
 * - Load Cell: 5kg/10kg strain gauge mounted to the carriage arm
 * - Interface: Serial Bit-bang (PIN_HX711_DT data line & PIN_HX711_SCK clock)
 * - Filtering: Non-blocking 10Hz polling with Exponential Moving Average (EMA)
 * - Persistence: Zero offset and calibration factor stored permanently in NVS Preferences
 * ============================================================================
 */

class ScaleManager {
public:
  ScaleManager();

  // Initialization & Hardware Setup
  void begin(Preferences* prefs = nullptr);
  void update(); // Non-blocking periodic poll (10Hz)

  // Tare & Calibration Routines
  void tare(uint8_t samples = 10);
  void calibrate(float knownWeightGrams, uint8_t samples = 10);

  // Live Weight & Calibration Accessors
  float getWeightGrams() const { return _currentWeightGrams; }
  long getRawValue() const { return _rawReadout; }
  void setScaleFactor(float factor);
  float getScaleFactor() const { return _scaleFactor; }
  long getZeroOffset() const { return _zeroOffset; }

  // Low-level 24-bit bitbang reader
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

