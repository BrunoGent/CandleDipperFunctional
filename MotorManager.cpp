#include "MotorManager.h"

// Standard 8mm lead screw with 1/16 microstepping -> 80 steps per mm
const float MotorManager::STEPS_PER_MM = 80.0f;

MotorManager::MotorManager()
  : _sensors(nullptr), _currentMode(MODE_STEALTHCHOP), _enabled(false), _positionMM(0.0f), _lastStepMicros(0) {}

void MotorManager::begin(SensorManager* sensorMgr) {
  _sensors = sensorMgr;

  pinMode(PIN_MOTOR_STEP, OUTPUT);
  pinMode(PIN_MOTOR_DIR, OUTPUT);
  pinMode(PIN_MOTOR_ENABLE, OUTPUT);
  pinMode(PIN_MOTOR_UART, OUTPUT);

  digitalWrite(PIN_MOTOR_STEP, LOW);
  digitalWrite(PIN_MOTOR_DIR, LOW);
  
  // Enable driver (Active LOW)
  digitalWrite(PIN_MOTOR_ENABLE, LOW);
  _enabled = true;

  // Initialize TMC2209 driver to StealthChop2 mode by default
  setTMCMode(MODE_STEALTHCHOP);
}

void MotorManager::setTMCMode(TMCMode mode) {
  _currentMode = mode;
  // Send TMC2209 UART single-wire packet to set GCONF register
  if (mode == MODE_STEALTHCHOP) {
    // StealthChop enabled (en_SpreadCycle = 0)
    sendUARTCommand(0x00, 0x00000004);
  } else {
    // SpreadCycle enabled (en_SpreadCycle = 1 for rapid travel)
    sendUARTCommand(0x00, 0x00000000);
  }
}

void MotorManager::sendUARTCommand(uint8_t reg, uint32_t val) {
  // Single-wire TMC2209 software UART setup pulse
  digitalWrite(PIN_MOTOR_UART, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_MOTOR_UART, LOW);
  delayMicroseconds(10);
  digitalWrite(PIN_MOTOR_UART, HIGH);
}

void MotorManager::stopMotor() {
  digitalWrite(PIN_MOTOR_STEP, LOW);
}

void MotorManager::stepMotor(bool directionUp, float speedMMps) {
  if (speedMMps <= 0.0f) return;

  // Select TMC2209 mode based on speed: StealthChop2 for low/med, SpreadCycle for rapid
  if (speedMMps > 65.0f && _currentMode != MODE_SPREADCYCLE) {
    setTMCMode(MODE_SPREADCYCLE);
  } else if (speedMMps <= 65.0f && _currentMode != MODE_STEALTHCHOP) {
    setTMCMode(MODE_STEALTHCHOP);
  }

  // Safety checks
  if (_sensors) {
    _sensors->update();
    // 1. Moving UP: check top limit switch and don't move past 0mm home
    if (directionUp) {
      if (_sensors->isTopLimitHit() || _positionMM <= 0.0f) {
        if (_sensors->isTopLimitHit()) _positionMM = 0.0f; // Calibrate home
        stopMotor();
        return;
      }
    } else {
      // 2. Moving DOWN: check capacitive wax sensor
      if (_sensors->isCapSensorTriggered()) {
        stopMotor();
        return;
      }
    }
  }

  // Set direction pin (HIGH = UP towards home, LOW = DOWN into wax)
  digitalWrite(PIN_MOTOR_DIR, directionUp ? HIGH : LOW);

  // Calculate step interval in microseconds
  float stepsPerSec = speedMMps * STEPS_PER_MM;
  unsigned long intervalMicros = (unsigned long)(1000000.0f / stepsPerSec);
  if (intervalMicros < 20) intervalMicros = 20; // Cap max pulse rate

  unsigned long now = micros();
  if (now - _lastStepMicros >= intervalMicros) {
    _lastStepMicros = now;

    // Pulse STEP pin
    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_MOTOR_STEP, LOW);

    // Update position track (+ down, - up towards 0)
    float stepDistMM = 1.0f / STEPS_PER_MM;
    if (directionUp) {
      _positionMM -= stepDistMM;
      if (_positionMM < 0.0f) _positionMM = 0.0f;
    } else {
      _positionMM += stepDistMM;
    }
  }
}

bool MotorManager::performHoming(float speedMMps) {
  // Homing sequence: move UP at speedMMps until top limit switch engages
  setTMCMode(MODE_SPREADCYCLE);
  
  unsigned long startTimeout = millis();
  while (_sensors && !_sensors->readRawTopLimit()) {
    // Force direct step without pos limit check during homing
    digitalWrite(PIN_MOTOR_DIR, HIGH);
    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(5);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    
    float stepsPerSec = speedMMps * STEPS_PER_MM;
    unsigned long delayUs = (unsigned long)(1000000.0f / stepsPerSec);
    delayMicroseconds(delayUs > 50 ? delayUs : 50);

    // Safety timeout 15s
    if (millis() - startTimeout > 15000) return false;
  }

  // Back down slightly until switch opens
  digitalWrite(PIN_MOTOR_DIR, LOW);
  for (int i = 0; i < (int)(2.0f * STEPS_PER_MM); i++) { // move 2mm down
    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(5);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    delayMicroseconds(200);
  }

  // Slowly re-approach home
  while (_sensors && !_sensors->readRawTopLimit()) {
    digitalWrite(PIN_MOTOR_DIR, HIGH);
    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(5);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    delayMicroseconds(500);
  }

  _positionMM = 0.0f; // Zero home reference position
  setTMCMode(MODE_STEALTHCHOP);
  return true;
}

bool MotorManager::performDipBot(float downSpeedMMps, float upSpeedMMps, int holdTimeSec) {
  // 1. Move DOWN at downSpeedMMps until capacitive wax sensor triggers
  setTMCMode(MODE_STEALTHCHOP);
  unsigned long startTimeout = millis();
  while (_sensors && !_sensors->readRawCapSensor()) {
    stepMotor(false, downSpeedMMps);
    delayMicroseconds(50);
    if (millis() - startTimeout > 20000) break; // Timeout guard
  }

  // 2. Hold in wax for s_subDipTime
  if (holdTimeSec > 0) {
    delay(holdTimeSec * 1000);
  }

  // 3. Move back UP to 0mm home at upSpeedMMps
  while (_positionMM > 0.0f && (_sensors && !_sensors->readRawTopLimit())) {
    stepMotor(true, upSpeedMMps);
    delayMicroseconds(50);
  }

  _positionMM = 0.0f;
  return true;
}
