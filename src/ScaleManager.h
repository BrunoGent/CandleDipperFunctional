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

  // Initialization & Hardware Setup: configures HX711 pins, loads calibration factor from NVS, and tares in RAM
  void begin(Preferences* prefs = nullptr);
  bool update(); // Non-blocking periodic poll (10Hz); returns true when a fresh sample was read

  // Tare & Calibration Routines
  void tare(uint8_t samples = 10); // Zeroes the scale in RAM by averaging 'samples' fresh readings (not saved to NVS since jigs vary)
  void calibrate(float knownWeightGrams, uint8_t samples = 10); // Computes new _scaleFactor from a known weight and saves to NVS

  // Live Weight & Calibration Accessors
  float getWeightGrams() const { return _currentWeightGrams; }
  long getRawValue() const { return _rawReadout; }
  void setScaleFactor(float factor);
  float getScaleFactor() const { return _scaleFactor; }
  long getZeroOffset() const { return _zeroOffset; }

  // Low-level 24-bit bitbang reader for the HX711 ADC (non-blocking if DT is HIGH)
  long readRawBitbang();

  // Waits up to timeoutMs for HX711 DT pin to go LOW (conversion ready) and reads one fresh 24-bit sample
  long readBlockingSample(uint32_t timeoutMs = 150);

  // Averages 'samples' fresh HX711 conversions and immediately updates _currentWeightGrams (eliminating EMA lag after a dip)
  float readSettledWeight(uint8_t samples = 4);

private:
  Preferences* _prefs;        // Pointer to ESP32 non-volatile storage (NVS) for saving tare & calibration
  long _zeroOffset;           // Raw ADC value corresponding to 0 grams (tare weight)
  float _scaleFactor;         // Raw ADC counts per gram (used to convert net raw counts into grams)
  float _currentWeightGrams;  // Filtered live weight in grams
  long _rawReadout;           // Most recent valid 24-bit signed raw reading from the HX711
  unsigned long _lastPollMs;  // Timestamp (ms) of the last 10Hz poll
};

#endif // SCALE_MANAGER_H


