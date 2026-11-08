#include "SensorManager.h"

// Initialize sensor states to false (not triggered), default wax sensor to Active-LOW, and top limit to Active-HIGH
SensorManager::SensorManager() 
  : _topLimitHit(false), _capSensorTriggered(false), _capActiveLow(true), _topLimitActiveHigh(true), _lastDebounceTime(0) {}

void SensorManager::begin(bool useInternalPullupForWaxSensor) {
  // Top switch configured with internal pull-up (NC microswitch or optical endstop)
  pinMode(PIN_TOP_LIMIT_SW, INPUT_PULLUP);
  
  // Capacitive wax sensor (via 24V -> 3.3V Optocoupler):
  // Since a physical 10k pull-up resistor is fitted on the circuit board,
  // we set pure INPUT (High-Z) so the internal 45k pull-up does not fight the optocoupler.
  if (useInternalPullupForWaxSensor) {
    pinMode(PIN_WAX_LEVEL_SENS, INPUT_PULLUP);
  } else {
    pinMode(PIN_WAX_LEVEL_SENS, INPUT);
  }
  
  // Take an initial reading immediately on boot
  update();
}

bool SensorManager::readRawTopLimit() {
  // Top Limit Switch / Optical Endstop:
  // - Default Active-HIGH (_topLimitActiveHigh == true):
  //   Unpressed/unblocked beam = LOW; Triggered/beam blocked = HIGH
  // Multi-sample 3-read glitch filter (2-out-of-3 majority vote) prevents EMI noise from stepper lines
  int matchCount = 0;
  int targetLevel = _topLimitActiveHigh ? HIGH : LOW;
  for (int i = 0; i < 3; i++) {
    if (digitalRead(PIN_TOP_LIMIT_SW) == targetLevel) matchCount++;
    delayMicroseconds(2);
  }
  return (matchCount >= 2);
}

bool SensorManager::readRawCapSensor() {
  /**
   * Wax Level Capacitive Sensor (24V Industrial NPN with Optocoupler):
   * - At idle (No wax detected): Optocoupler is OFF -> External 10k pull-up holds pin at 3.3V (HIGH)
   * - When triggered (Wax detected): Optocoupler transistor turns ON -> Pulls pin to GND (LOW)
   * 
   * Therefore, for standard NPN Optocoupler wiring (_capActiveLow == true):
   * Triggered = (digitalRead == LOW)
   * 
   * Multi-sample filtering (3 reads with 2us spacing, requiring >=2 matches) rejects motor PWM noise in the drag chain.
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
  // Non-blocking 10ms (100Hz) periodic debounce check used to update UI status indicators in loop()
  if (millis() - _lastDebounceTime >= 10) {
    _lastDebounceTime = millis();
    _topLimitHit = readRawTopLimit();
    _capSensorTriggered = readRawCapSensor();
  }
}


