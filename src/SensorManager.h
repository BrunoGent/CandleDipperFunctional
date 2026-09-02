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

class SensorManager {
public:
  SensorManager();

  void begin();
  void update(); // Non-blocking periodic debouncer

  // Switch & Sensor Status Readers (Debounced)
  bool isTopLimitHit() const { return _topLimitHit; }
  bool isCapSensorTriggered() const { return _capSensorTriggered; }

  // Direct Pin Read Helpers (With Hardware Glitch Filtering)
  bool readRawTopLimit();
  bool readRawCapSensor();

  // Inversion / Polarity Configuration (true = Active LOW, false = Active HIGH)
  void setCapSensorActiveLow(bool activeLow) { _capActiveLow = activeLow; }
  bool isCapSensorActiveLow() const { return _capActiveLow; }

private:
  bool _topLimitHit;
  bool _capSensorTriggered;
  bool _capActiveLow; // Standard NPN optocoupler pulls LOW when triggered
  unsigned long _lastDebounceTime;
};

#endif // SENSOR_MANAGER_H

