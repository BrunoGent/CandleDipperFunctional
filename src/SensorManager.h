#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include "PinConfig.h"

/**
 * ============================================================================
 * SENSOR MANAGER COMPONENT
 * ============================================================================
 * Handles input sensing, debouncing, and glitch filtering for:
 * 1. Top Mechanical Limit Switch (NC - Normally Closed switch to GND)
 *    - Unpressed: Pin connected to GND (LOW)
 *    - Pressed/Triggered: Circuit opens, internal pull-up pulls to 3.3V (HIGH)
 * 
 * 2. Bottom Wax Level Capacitive Proximity Sensor (24V NPN Optocoupled)
 *    - Untriggered (No wax): Optocoupler OFF, internal pull-up pulls to 3.3V (HIGH)
 *    - Triggered (Wax detected): Optocoupler conducts, pulls pin to GND (LOW)
 * ============================================================================
 */

// Time-qualified debounce for the motion loops: update() returns true only once the raw input has been
// seen active on every poll for at least holdUs microseconds. A single noise spike from the stepper
// wiring therefore cannot end a move or be recorded as the wax level.
class Debounce {
public:
  bool update(bool rawActive, uint32_t holdUs = 2000) {
    if (!rawActive) { _active = false; return false; }
    if (!_active) { _active = true; _sinceUs = micros(); return false; }
    return (micros() - _sinceUs) >= holdUs;
  }
  void reset() { _active = false; }
private:
  bool _active = false;
  uint32_t _sinceUs = 0;
};

class SensorManager {
public:
  SensorManager();

  // Configures GPIO pin modes for the top limit switch and wax capacitive sensor
  void begin(bool useInternalPullupForWaxSensor = false);

  // Non-blocking periodic debouncer (polls sensors every 10ms for UI status indicators)
  void update(); // Non-blocking periodic debouncer

  // Switch & Sensor Status Readers (Debounced values cached by update() for UI display)
  bool isTopLimitHit() const { return _topLimitHit; }
  bool isCapSensorTriggered() const { return _capSensorTriggered; }

  // Direct Pin Read Helpers (With 3-sample Hardware Glitch Filtering for real-time motor loops)
  bool readRawTopLimit();
  bool readRawCapSensor();

  // Inversion / Polarity Configuration (true = Active LOW, false = Active HIGH)
  void setCapSensorActiveLow(bool activeLow) { _capActiveLow = activeLow; }
  bool isCapSensorActiveLow() const { return _capActiveLow; }

  void setTopLimitActiveHigh(bool activeHigh) { _topLimitActiveHigh = activeHigh; }
  bool isTopLimitActiveHigh() const { return _topLimitActiveHigh; }

private:
  bool _topLimitHit;              // Cached debounced state of the top limit switch
  bool _capSensorTriggered;       // Cached debounced state of the capacitive wax sensor
  bool _capActiveLow;             // Polarity flag: standard NPN optocoupler pulls pin LOW when triggered
  bool _topLimitActiveHigh;       // Polarity flag: NC mechanical or active-high optical pulls HIGH when hit
  unsigned long _lastDebounceTime;// Timestamp (ms) of the last periodic update() poll
};

#endif // SENSOR_MANAGER_H

