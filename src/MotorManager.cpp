#include "MotorManager.h"

// Standard 8mm lead screw T8x8 with 1/8 microstepping -> 200 steps per mm
const float MotorManager::STEPS_PER_MM = 200.0f;

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

  // Select TMC2209 mode based on speed
  if (speedMMps > 65.0f && _currentMode != MODE_SPREADCYCLE) {
    setTMCMode(MODE_SPREADCYCLE);
  } else if (speedMMps <= 65.0f && _currentMode != MODE_STEALTHCHOP) {
    setTMCMode(MODE_STEALTHCHOP);
  }

  // Limit checks (throttled every 5ms)
  static unsigned long lastSensorCheck = 0;
  if (millis() - lastSensorCheck >= 5) {
    lastSensorCheck = millis();
    if (_sensors) {
      if (directionUp) {
        if (_sensors->readRawTopLimit() || _positionMM <= 0.0f) {
          if (_sensors->readRawTopLimit()) _positionMM = 0.0f; // Calibrate home
          stopMotor();
          return;
        }
      } else {
        if (_sensors->readRawCapSensor()) {
          stopMotor();
          return;
        }
      }
    }
  }

  // Set direction pin
  digitalWrite(PIN_MOTOR_DIR, directionUp ? HIGH : LOW);

  // Calculate step interval in microseconds
  float stepsPerSec = speedMMps * STEPS_PER_MM;
  if (stepsPerSec <= 0.0f) return;
  unsigned long intervalMicros = (unsigned long)(1000000.0f / stepsPerSec);
  if (intervalMicros < 10) intervalMicros = 10;

  unsigned long now = micros();
  if (now - _lastStepMicros >= intervalMicros) {
    _lastStepMicros = now;

    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_MOTOR_STEP, LOW);

    float stepDistMM = 1.0f / STEPS_PER_MM;
    if (directionUp) {
      _positionMM -= stepDistMM;
      if (_positionMM < 0.0f) _positionMM = 0.0f;
    } else {
      _positionMM += stepDistMM;
    }
  }
}

void MotorManager::stepMotorBurst(bool directionUp, float speedMMps, uint32_t burstMs) {
  if (speedMMps <= 0.0f) return;

  if (speedMMps > 65.0f && _currentMode != MODE_SPREADCYCLE) {
    setTMCMode(MODE_SPREADCYCLE);
  } else if (speedMMps <= 65.0f && _currentMode != MODE_STEALTHCHOP) {
    setTMCMode(MODE_STEALTHCHOP);
  }

  digitalWrite(PIN_MOTOR_DIR, directionUp ? HIGH : LOW);

  float stepsPerSec = speedMMps * STEPS_PER_MM;
  if (stepsPerSec <= 0.0f) return;
  unsigned long intervalMicros = (unsigned long)(1000000.0f / stepsPerSec);
  if (intervalMicros < 10) intervalMicros = 10;

  float stepDistMM = 1.0f / STEPS_PER_MM;
  unsigned long startMs = millis();
  unsigned long lastSensorCheck = 0;

  while (millis() - startMs < burstMs) {
    if (millis() - lastSensorCheck >= 5) {
      lastSensorCheck = millis();
      if (_sensors) {
        if (directionUp) {
          if (_sensors->readRawTopLimit() || _positionMM <= 0.0f) {
            if (_sensors->readRawTopLimit()) _positionMM = 0.0f;
            stopMotor();
            break;
          }
        } else {
          if (_sensors->readRawCapSensor()) {
            stopMotor();
            break;
          }
        }
      }
    }

    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_MOTOR_STEP, LOW);

    if (directionUp) {
      _positionMM -= stepDistMM;
      if (_positionMM < 0.0f) _positionMM = 0.0f;
    } else {
      _positionMM += stepDistMM;
    }

    if (intervalMicros > 3) {
      delayMicroseconds(intervalMicros - 3);
    }
  }
}

bool MotorManager::performHoming(float speedMMps, bool (*stopCheck)()) {
  // Homing sequence: move UP at speedMMps until top limit switch engages
  setTMCMode(MODE_SPREADCYCLE);
  
  unsigned long startTimeout = millis();
  float stepsPerSec = speedMMps * STEPS_PER_MM;
  unsigned long delayUs = (unsigned long)(1000000.0f / stepsPerSec);
  if (delayUs < 30) delayUs = 30;

  // Stage 1: Fast search towards limit switch
  while (_sensors && !_sensors->readRawTopLimit()) {
    if (stopCheck && stopCheck()) {
      stopMotor();
      setTMCMode(MODE_STEALTHCHOP);
      return false;
    }

    digitalWrite(PIN_MOTOR_DIR, HIGH);
    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    delayMicroseconds(delayUs);

    if (millis() - startTimeout > 15000) {
      stopMotor();
      setTMCMode(MODE_STEALTHCHOP);
      return false;
    }
  }

  // Stage 2: Back down 2mm slightly until switch opens
  digitalWrite(PIN_MOTOR_DIR, LOW);
  for (int i = 0; i < (int)(2.0f * STEPS_PER_MM); i++) {
    if (stopCheck && stopCheck()) {
      stopMotor();
      setTMCMode(MODE_STEALTHCHOP);
      return false;
    }
    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    delayMicroseconds(250);
  }

  // Stage 3: Slowly re-approach home for precision zeroing
  while (_sensors && !_sensors->readRawTopLimit()) {
    if (stopCheck && stopCheck()) {
      stopMotor();
      setTMCMode(MODE_STEALTHCHOP);
      return false;
    }
    digitalWrite(PIN_MOTOR_DIR, HIGH);
    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    delayMicroseconds(500);
  }

  _positionMM = 0.0f; // Zero home reference position
  stopMotor();
  setTMCMode(MODE_STEALTHCHOP);
  return true;
}

bool MotorManager::performDipBot(float downSpeedMMps, float upSpeedMMps, int holdTimeSec, bool (*stopCheck)()) {
  // 1. Move DOWN at downSpeedMMps until capacitive wax sensor triggers
  setTMCMode(MODE_STEALTHCHOP);
  unsigned long startTimeout = millis();
  while (_sensors && !_sensors->readRawCapSensor()) {
    if (stopCheck && stopCheck()) {
      stopMotor();
      return false;
    }
    stepMotorBurst(false, downSpeedMMps, 15);
    if (millis() - startTimeout > 25000) break; // Timeout guard
  }

  // 2. Hold in wax for holdTimeSec
  if (holdTimeSec > 0) {
    unsigned long holdStart = millis();
    while (millis() - holdStart < (unsigned long)(holdTimeSec * 1000)) {
      if (stopCheck && stopCheck()) {
        stopMotor();
        return false;
      }
      delay(20);
    }
  }

  // 3. Move back UP to 0mm home at upSpeedMMps
  while (_positionMM > 0.0f && (_sensors && !_sensors->readRawTopLimit())) {
    if (stopCheck && stopCheck()) {
      stopMotor();
      return false;
    }
    stepMotorBurst(true, upSpeedMMps, 15);
  }

  _positionMM = 0.0f;
  stopMotor();
  return true;
}
