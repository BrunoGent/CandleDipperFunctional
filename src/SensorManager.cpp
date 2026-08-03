#include "SensorManager.h"

SensorManager::SensorManager() 
  : _topLimitHit(false), _capSensorTriggered(false), _lastDebounceTime(0) {}

void SensorManager::begin() {
  // Configured with internal pull-ups if needed, hardware NC config typically pulls LOW or HIGH
  pinMode(PIN_TOP_LIMIT_SW, INPUT_PULLUP);
  pinMode(PIN_WAX_LEVEL_SENS, INPUT_PULLUP);
  update();
}

bool SensorManager::readRawTopLimit() {
  // Mechanical Home Switch (NC config): Pin reads HIGH when limit switch is pressed/triggered (circuit opens)
  return (digitalRead(PIN_TOP_LIMIT_SW) == HIGH);
}

bool SensorManager::readRawCapSensor() {
  // Capacitive Sensor: Active HIGH when wax/object detected
  return (digitalRead(PIN_WAX_LEVEL_SENS) == HIGH);
}

void SensorManager::update() {
  // Non-blocking 10ms debounce check
  if (millis() - _lastDebounceTime >= 10) {
    _lastDebounceTime = millis();
    _topLimitHit = readRawTopLimit();
    _capSensorTriggered = readRawCapSensor();
  }
}
