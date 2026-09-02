#include "SensorManager.h"

SensorManager::SensorManager() 
  : _topLimitHit(false), _capSensorTriggered(false), _capActiveLow(true), _lastDebounceTime(0) {}

void SensorManager::begin() {
  // Mechanical top switch configured with internal pull-up
  pinMode(PIN_TOP_LIMIT_SW, INPUT_PULLUP);
  
  // Capacitive wax sensor (via 24V -> 3.3V Optocoupler)
  pinMode(PIN_WAX_LEVEL_SENS, INPUT_PULLUP);
  
  update();
}

bool SensorManager::readRawTopLimit() {
  // Mechanical Home Switch (NC Config):
  // Circuit is closed to GND when switch is NOT pressed -> Pin reads LOW (0V)
  // When switch is pressed/hit, circuit opens -> Internal pull-up pulls to 3.3V (HIGH)
  // Multi-sample 3-read glitch filter prevents EMI noise from stepper lines
  int highCount = 0;
  for (int i = 0; i < 3; i++) {
    if (digitalRead(PIN_TOP_LIMIT_SW) == HIGH) highCount++;
    delayMicroseconds(2);
  }
  return (highCount >= 2);
}

bool SensorManager::readRawCapSensor() {
  /**
   * Wax Level Capacitive Sensor (24V Industrial NPN with Optocoupler):
   * - At idle (No wax detected): Optocoupler is OFF -> Internal pull-up holds pin at 3.3V (HIGH)
   * - When triggered (Wax detected): Optocoupler transistor turns ON -> Pulls pin to GND (LOW)
   * 
   * Therefore, for standard NPN Optocoupler wiring (_capActiveLow == true):
   * Triggered = (digitalRead == LOW)
   * 
   * Multi-sample filtering (3 reads) rejects motor PWM noise in the drag chain.
   */
  int matchCount = 0;
  int targetLevel = _capActiveLow ? LOW : HIGH;

  for (int i = 0; i < 3; i++) {
    if (digitalRead(PIN_WAX_LEVEL_SENS) == targetLevel) matchCount++;
    delayMicroseconds(2);
  }
  return (matchCount >= 2);
}

void SensorManager::update() {
  // Non-blocking 10ms periodic debounce check for UI telemetry
  if (millis() - _lastDebounceTime >= 10) {
    _lastDebounceTime = millis();
    _topLimitHit = readRawTopLimit();
    _capSensorTriggered = readRawCapSensor();
  }
}

