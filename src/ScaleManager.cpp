#include "ScaleManager.h"

ScaleManager::ScaleManager()
  : _prefs(nullptr), _zeroOffset(0), _scaleFactor(420.0f), _currentWeightGrams(0.0f), _rawReadout(0), _lastPollMs(0) {}

void ScaleManager::begin(Preferences* prefs) {
  _prefs = prefs;
  pinMode(PIN_HX711_DT, INPUT_PULLUP);
  pinMode(PIN_HX711_SCK, OUTPUT);
  digitalWrite(PIN_HX711_SCK, LOW);

  // Load stored calibration values from EEPROM/Preferences if available
  if (_prefs) {
    _zeroOffset = _prefs->getLong("scale_off", 0);
    _scaleFactor = _prefs->getFloat("scale_fac", 420.0f); // Default calibration factor
  }

  // Initial zero tare if offset wasn't saved
  if (_zeroOffset == 0) {
    tare(10);
  }
}

long ScaleManager::readRawBitbang() {
  // Wait up to 15ms for DT line to drop LOW (data ready)
  unsigned long start = millis();
  while (digitalRead(PIN_HX711_DT) == HIGH) {
    if (millis() - start > 15) {
      return _rawReadout; // Return last known reading if sensor not ready or disconnected
    }
  }

  noInterrupts(); // Protect 24-bit bitbang from ESP32 task context switches
  unsigned long count = 0;
  for (int i = 0; i < 24; i++) {
    digitalWrite(PIN_HX711_SCK, HIGH);
    delayMicroseconds(2);
    count = count << 1;
    digitalWrite(PIN_HX711_SCK, LOW);
    delayMicroseconds(2);
    if (digitalRead(PIN_HX711_DT)) {
      count++;
    }
  }

  // 25th pulse sets Channel A, Gain 128 for next conversion
  digitalWrite(PIN_HX711_SCK, HIGH);
  delayMicroseconds(2);
  digitalWrite(PIN_HX711_SCK, LOW);
  delayMicroseconds(2);
  interrupts();

  // Sign extension for 24-bit two's complement integer
  if (count & 0x800000) {
    count |= 0xFF000000;
  }

  return (long)count;
}

void ScaleManager::update() {
  // Poll HX711 every 100ms (10Hz rate)
  if (millis() - _lastPollMs >= 100) {
    _lastPollMs = millis();
    _rawReadout = readRawBitbang();
    long netVal = _rawReadout - _zeroOffset;
    if (_scaleFactor != 0.0f) {
      float calculatedGrams = (float)netVal / _scaleFactor;
      // Filter slight jitter near zero
      if (abs(calculatedGrams) < 0.2f) calculatedGrams = 0.0f;
      _currentWeightGrams = calculatedGrams;
    }
  }
}

void ScaleManager::tare(uint8_t samples) {
  long sum = 0;
  uint8_t validSamples = 0;
  for (uint8_t i = 0; i < samples; i++) {
    long raw = readRawBitbang();
    if (raw != 0) {
      sum += raw;
      validSamples++;
    }
    delay(10);
  }
  if (validSamples > 0) {
    _zeroOffset = sum / validSamples;
    _currentWeightGrams = 0.0f;
    if (_prefs) {
      _prefs->putLong("scale_off", _zeroOffset);
    }
  }
}

void ScaleManager::calibrate(float knownWeightGrams, uint8_t samples) {
  if (knownWeightGrams <= 0.0f) return;

  long sum = 0;
  uint8_t validSamples = 0;
  for (uint8_t i = 0; i < samples; i++) {
    long raw = readRawBitbang();
    if (raw != 0) {
      sum += raw;
      validSamples++;
    }
    delay(10);
  }

  if (validSamples > 0) {
    long avgRaw = sum / validSamples;
    long netRaw = avgRaw - _zeroOffset;
    _scaleFactor = (float)netRaw / knownWeightGrams;

    if (_prefs) {
      _prefs->putFloat("scale_fac", _scaleFactor);
    }
  }
}

void ScaleManager::setScaleFactor(float factor) {
  if (factor != 0.0f) {
    _scaleFactor = factor;
    if (_prefs) {
      _prefs->putFloat("scale_fac", _scaleFactor);
    }
  }
}
