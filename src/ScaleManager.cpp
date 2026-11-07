#include "ScaleManager.h"

// Default constructor initializes scale with a typical 420.0 raw-units-per-gram calibration factor
ScaleManager::ScaleManager()
  : _prefs(nullptr), _zeroOffset(0), _scaleFactor(420.0f), _currentWeightGrams(0.0f), _rawReadout(0), _lastPollMs(0) {}

void ScaleManager::begin(Preferences* prefs) {
  _prefs = prefs;
  pinMode(PIN_HX711_DT, INPUT_PULLUP);
  pinMode(PIN_HX711_SCK, OUTPUT);
  digitalWrite(PIN_HX711_SCK, LOW); // Keep SCK LOW so HX711 stays powered on (HIGH >60us enters power-down mode)

  // Load stored load-cell sensitivity factor (counts per gram) from NVS Preferences if available.
  // Note: 0g zero offset is NOT stored in NVS because different candle jigs have different weights;
  // instead, the scale is tared in RAM on startup and automatically at the start of every dip.
  if (_prefs) {
    _scaleFactor = _prefs->getFloat("scale_fac", 420.0f); // Default calibration factor
  }

  // Initial zero tare in RAM
  tare(10);
}

long ScaleManager::readRawBitbang() {
  // Check if DT line is HIGH (sensor busy / data not ready) - return immediately to avoid blocking stepper pulses
  if (digitalRead(PIN_HX711_DT) == HIGH) {
    return _rawReadout;
  }

  // Disable interrupts during bit-banging because if an interrupt delays SCK HIGH > 60us,
  // the HX711 chip will interpret it as a power-down command and corrupt the readout.
  noInterrupts(); // Protect 24-bit bitbang from ESP32 task context switches
  unsigned long count = 0;
  // Clock out all 24 bits (MSB first) from the HX711 shift register
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

  // Sign extension for 24-bit two's complement integer to 32-bit signed long:
  // If bit 23 (0x800000, the sign bit) is 1, fill the upper 8 bits with 1s (0xFF000000).
  if (count & 0x800000) {
    count |= 0xFF000000;
  }

  return (long)count;
}

// Waits up to timeoutMs for the HX711 DOUT line to go LOW (indicating a fresh 24-bit conversion is ready)
// before clocking out the sample. Returns 0 if the sensor times out or is disconnected.
long ScaleManager::readBlockingSample(uint32_t timeoutMs) {
  unsigned long start = millis();
  while (digitalRead(PIN_HX711_DT) == HIGH) {
    if (millis() - start >= timeoutMs) {
      return 0; // Timeout: sensor disconnected or not ready
    }
    delay(1);
  }
  long raw = readRawBitbang();
  if (raw != 0 && raw != 0x7FFFFF && raw != -1) {
    _rawReadout = raw;
    return raw;
  }
  return 0;
}

// Reads 'samples' fresh conversions from the HX711, computes the average weight in grams,
// and immediately sets _currentWeightGrams so post-dip weight checks have zero EMA filter lag.
float ScaleManager::readSettledWeight(uint8_t samples) {
  if (_scaleFactor == 0.0f) return _currentWeightGrams;

  long sum = 0;
  uint8_t validSamples = 0;
  for (uint8_t i = 0; i < samples; i++) {
    long raw = readBlockingSample(150);
    if (raw != 0) {
      sum += raw;
      validSamples++;
    }
  }

  if (validSamples > 0) {
    long avgRaw = sum / validSamples;
    long netVal = avgRaw - _zeroOffset;
    float grams = (float)netVal / _scaleFactor;
    if (abs(grams) < 0.2f) grams = 0.0f;
    _currentWeightGrams = grams;
    _lastPollMs = millis();
  }
  return _currentWeightGrams;
}

bool ScaleManager::update() {
  // Poll HX711 every 100ms (10Hz rate) matching the HX711 hardware sample rate
  if (millis() - _lastPollMs >= 100) {
    _lastPollMs = millis();
    // Only clock out data if DT is LOW (new sample ready)
    if (digitalRead(PIN_HX711_DT) == HIGH) {
      return false;
    }
    long newRaw = readRawBitbang();

    // Ignore invalid readings (0 or 0x7FFFFF max reading on disconnected line)
    if (newRaw != 0 && newRaw != 0x7FFFFF && newRaw != -1) {
      _rawReadout = newRaw;
      long netVal = _rawReadout - _zeroOffset; // Subtract tare offset
      if (_scaleFactor != 0.0f) {
        float calculatedGrams = (float)netVal / _scaleFactor; // Convert raw ADC counts to grams

        // Filter slight jitter near zero (deadband within +/- 0.2g)
        if (abs(calculatedGrams) < 0.2f) calculatedGrams = 0.0f;

        // Exponential moving average (EMA) filter for smooth, stable load cell readings:
        // Blends 75% previous weight with 25% new reading to dampen carriage vibration,
        // or jumps immediately if weight steps up from ~0g by more than 1g.
        if (abs(_currentWeightGrams) < 0.1f && abs(calculatedGrams) > 1.0f) {
          _currentWeightGrams = calculatedGrams; // Instant response on new load
        } else {
          _currentWeightGrams = (_currentWeightGrams * 0.75f) + (calculatedGrams * 0.25f);
        }
        return true;
      }
    }
  }
  return false;
}

// Reads multiple fresh samples from the load cell and averages them in RAM to set the current 0.0g point.
// Does NOT write to NVS flash because jigs have different weights and are tared at the start of each dip.
void ScaleManager::tare(uint8_t samples) {
  long sum = 0;
  uint8_t validSamples = 0;
  for (uint8_t i = 0; i < samples; i++) {
    long raw = readBlockingSample(150);
    if (raw != 0) {
      sum += raw;
      validSamples++;
    }
  }
  if (validSamples > 0) {
    _zeroOffset = sum / validSamples;
    _currentWeightGrams = 0.0f;
  }
}

// Calibrates the scale factor given a known reference mass (in grams) placed on the jig
void ScaleManager::calibrate(float knownWeightGrams, uint8_t samples) {
  if (knownWeightGrams <= 0.0f) return;

  long sum = 0;
  uint8_t validSamples = 0;
  for (uint8_t i = 0; i < samples; i++) {
    long raw = readBlockingSample(150);
    if (raw != 0) {
      sum += raw;
      validSamples++;
    }
  }

  if (validSamples > 0) {
    long avgRaw = sum / validSamples;
    long netRaw = avgRaw - _zeroOffset;
    if (netRaw != 0) {
      _scaleFactor = (float)netRaw / knownWeightGrams;
      _currentWeightGrams = knownWeightGrams;

      if (_prefs) {
        _prefs->putFloat("scale_fac", _scaleFactor);
      }
    }
  }
}

// Manually overrides the calibration factor and persists it to NVS
void ScaleManager::setScaleFactor(float factor) {
  if (factor != 0.0f) {
    _scaleFactor = factor;
    if (_prefs) {
      _prefs->putFloat("scale_fac", _scaleFactor);
    }
  }
}


