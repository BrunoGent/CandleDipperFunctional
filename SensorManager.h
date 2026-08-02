#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include "PinConfig.h"

class SensorManager {
public:
  SensorManager();

  void begin();
  void update();

  // Switch & Sensor Status Readers
  bool isTopLimitHit() const { return _topLimitHit; }
  bool isCapSensorTriggered() const { return _capSensorTriggered; }

  // Direct Pin Read Helper
  bool readRawTopLimit();
  bool readRawCapSensor();

private:
  bool _topLimitHit;
  bool _capSensorTriggered;
  unsigned long _lastDebounceTime;
};

#endif // SENSOR_MANAGER_H
