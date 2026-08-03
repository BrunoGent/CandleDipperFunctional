#include "MotorManager.h"

// Standard 8mm lead screw T8x8 with 1/8 microstepping -> 200 steps per mm
const float MotorManager::STEPS_PER_MM = 200.0f;

MotorManager::MotorManager()
  : _sensors(nullptr), _currentMode(MODE_STEALTHCHOP), _enabled(false), _positionMM(0.0f), _maxSoftLimitMM(500.0f), _lastStepMicros(0) {}

static uint8_t calcTMC2209CRC(const uint8_t* data, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    uint8_t currentByte = data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if ((crc >> 7) ^ (currentByte & 0x01)) {
        crc = (crc << 1) ^ 0x07;
      } else {
        crc = (crc << 1);
      }
      currentByte >>= 1;
    }
  }
  return crc;
}

void MotorManager::sendUARTCommand(uint8_t reg, uint32_t val) {
  uint8_t msg[8];
  msg[0] = 0x05; // TMC2209 Sync byte
  msg[1] = 0x00; // Slave address 0
  msg[2] = reg | 0x80; // Write register
  msg[3] = (val >> 24) & 0xFF;
  msg[4] = (val >> 16) & 0xFF;
  msg[5] = (val >> 8) & 0xFF;
  msg[6] = val & 0xFF;
  msg[7] = calcTMC2209CRC(msg, 7);

  Serial2.write(msg, 8);
  Serial2.flush();
}

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

  // Initialize Serial2 for TMC2209 Single-Wire UART
  Serial2.begin(115200, SERIAL_8N1, -1, PIN_MOTOR_UART);
  delay(10);

  // Initialize TMC2209 driver registers over UART:
  // 1. IHOLD_IRUN (0x10): Run current = 16 (~0.6A RMS), Standby = 8, Hold delay = 6
  sendUARTCommand(0x10, 0x00061008);
  delay(5);

  // 2. CHOPCONF (0x6C): 16 Microsteps with 256 Interpolation (0x14000053)
  sendUARTCommand(0x6C, 0x14000053);
  delay(5);

  // 3. PWMCONF (0x70): StealthChop2 PWM autoscale configuration
  sendUARTCommand(0x70, 0xC40D001D);
  delay(5);

  // 4. Set TMC mode to StealthChop2
  setTMCMode(MODE_STEALTHCHOP);
}

void MotorManager::setTMCMode(TMCMode mode) {
  _currentMode = mode;
  // Send TMC2209 UART packet to set GCONF register (0x00)
  if (mode == MODE_STEALTHCHOP) {
    // StealthChop2 enabled (en_SpreadCycle = 0, mstep_reg_select = 1)
    sendUARTCommand(0x00, 0x00000080);
  } else {
    // SpreadCycle enabled (en_SpreadCycle = 1, mstep_reg_select = 1 for rapid travel)
    sendUARTCommand(0x00, 0x00000084);
  }
}

void MotorManager::setTMCCurrents(int irun, int ihold, int iholddelay) {
  uint32_t rRun = (uint32_t)constrain(irun, 0, 31);
  uint32_t rHold = (uint32_t)constrain(ihold, 0, 31);
  uint32_t rDelay = (uint32_t)constrain(iholddelay, 0, 15);
  uint32_t val = (rDelay << 16) | (rRun << 8) | rHold;
  sendUARTCommand(0x10, val);
}

void MotorManager::applyConfigMode(int modeSetting, float speedMMps, int thresholdMMps) {
  if (modeSetting == 0) {
    setTMCMode(MODE_STEALTHCHOP);
  } else if (modeSetting == 1) {
    setTMCMode(MODE_SPREADCYCLE);
  } else if (modeSetting == 2) {
    if (speedMMps >= (float)thresholdMMps) {
      setTMCMode(MODE_SPREADCYCLE);
    } else {
      setTMCMode(MODE_STEALTHCHOP);
    }
  }
}

void MotorManager::setMotorEnable(bool enable) {
  _enabled = enable;
  digitalWrite(PIN_MOTOR_ENABLE, enable ? LOW : HIGH);
}

void MotorManager::stopMotor() {
  digitalWrite(PIN_MOTOR_STEP, LOW);
}

void MotorManager::stepMotor(bool directionUp, float speedMMps) {
  if (speedMMps <= 0.0f) return;

  // Immediate limit checks - NO throttling
  if (_sensors) {
    if (directionUp) {
      if (_sensors->readRawTopLimit() || _positionMM <= 0.0f) {
        if (_sensors->readRawTopLimit()) _positionMM = 0.0f;
        stopMotor();
        return;
      }
    } else {
      if (_sensors->readRawCapSensor() || (_maxSoftLimitMM > 0.0f && _positionMM >= _maxSoftLimitMM)) {
        stopMotor();
        return;
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

  digitalWrite(PIN_MOTOR_DIR, directionUp ? HIGH : LOW);

  float stepsPerSec = speedMMps * STEPS_PER_MM;
  if (stepsPerSec <= 0.0f) return;
  unsigned long intervalMicros = (unsigned long)(1000000.0f / stepsPerSec);
  if (intervalMicros < 10) intervalMicros = 10;

  float stepDistMM = 1.0f / STEPS_PER_MM;
  unsigned long startMs = millis();

  while (millis() - startMs < burstMs) {
    if (_sensors) {
      if (directionUp) {
        if (_sensors->readRawTopLimit() || _positionMM <= 0.0f) {
          if (_sensors->readRawTopLimit()) _positionMM = 0.0f;
          stopMotor();
          break;
        }
      } else {
        if (_sensors->readRawCapSensor() || (_maxSoftLimitMM > 0.0f && _positionMM >= _maxSoftLimitMM)) {
          stopMotor();
          break;
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
  if (speedMMps <= 0.0f) speedMMps = 50.0f;

  setTMCMode(MODE_STEALTHCHOP);

  // Pre-check: If switch is ALREADY active when homing starts (e.g. carriage at top):
  // Move DOWN gently until switch opens
  if (_sensors && _sensors->readRawTopLimit()) {
    digitalWrite(PIN_MOTOR_DIR, LOW); // DOWN
    delayMicroseconds(20);
    int maxSteps = (int)(10.0f * STEPS_PER_MM);
    for (int i = 0; i < maxSteps; i++) {
      if (stopCheck && stopCheck()) { stopMotor(); return false; }
      if (!_sensors->readRawTopLimit()) break;
      digitalWrite(PIN_MOTOR_STEP, HIGH);
      delayMicroseconds(3);
      digitalWrite(PIN_MOTOR_STEP, LOW);
      delayMicroseconds(250); // ~20 mm/s
    }
    stopMotor();
    delay(50);
  }

  // Stage 1: Search UP towards limit switch
  digitalWrite(PIN_MOTOR_DIR, HIGH); // UP
  delayMicroseconds(20);
  float stepsPerSec = speedMMps * STEPS_PER_MM;
  unsigned long delayUs = (unsigned long)(1000000.0f / stepsPerSec);
  if (delayUs < 30) delayUs = 30;

  unsigned long startTime = millis();
  while (_sensors && !_sensors->readRawTopLimit()) {
    if (stopCheck && stopCheck()) { stopMotor(); return false; }
    if (millis() - startTime > 30000) { stopMotor(); return false; }

    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    delayMicroseconds(delayUs);
  }

  stopMotor();
  delay(50);

  // Stage 2: Back DOWN 2mm
  digitalWrite(PIN_MOTOR_DIR, LOW); // DOWN
  delayMicroseconds(20);
  int backoffSteps = (int)(2.0f * STEPS_PER_MM);
  for (int i = 0; i < backoffSteps; i++) {
    if (stopCheck && stopCheck()) { stopMotor(); return false; }
    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    delayMicroseconds(300); // ~16 mm/s
  }

  stopMotor();
  delay(50);

  // Stage 3: Re-approach UP slowly (~15 mm/s) for precision zeroing
  digitalWrite(PIN_MOTOR_DIR, HIGH); // UP
  delayMicroseconds(20);
  while (_sensors && !_sensors->readRawTopLimit()) {
    if (stopCheck && stopCheck()) { stopMotor(); return false; }

    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    delayMicroseconds(330); // ~15 mm/s
  }

  _positionMM = 0.0f;
  stopMotor();
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

