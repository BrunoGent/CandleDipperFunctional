#include "MotorManager.h"

// Standard 8mm lead screw T8x8 with 1/8 microstepping -> 200 steps per mm
const float MotorManager::STEPS_PER_MM = 200.0f;

MotorManager::MotorManager()
  : _sensors(nullptr), _currentMode(MODE_STEALTHCHOP), _enabled(false), _positionMM(0.0f), _maxSoftLimitMM(500.0f), _lastStepMicros(0) {}

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
  if (_currentMode == mode) return; // Do not resend UART packets if driver is already in requested mode
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

static uint8_t calcTMC2209CRC(const uint8_t* data, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    uint8_t currentByte = data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if ((crc ^ currentByte) & 0x01) {
        crc = (crc >> 1) ^ 0x8C;
      } else {
        crc = (crc >> 1);
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

  // Method 1: Hardware Serial2 on ESP32
  Serial2.begin(115200, SERIAL_8N1, -1, PIN_MOTOR_UART);
  delayMicroseconds(200);
  Serial2.write(msg, 8);
  Serial2.flush();
  Serial2.end();

  // Method 2: Software Bit-Bang UART as backup for single-wire PIN 18
  pinMode(PIN_MOTOR_UART, OUTPUT);
  digitalWrite(PIN_MOTOR_UART, HIGH);
  delayMicroseconds(100);

  for (int b = 0; b < 8; b++) {
    uint8_t byteVal = msg[b];
    digitalWrite(PIN_MOTOR_UART, LOW); // Start bit
    delayMicroseconds(8);
    for (int i = 0; i < 8; i++) {
      digitalWrite(PIN_MOTOR_UART, (byteVal & (1 << i)) ? HIGH : LOW);
      delayMicroseconds(8);
    }
    digitalWrite(PIN_MOTOR_UART, HIGH); // Stop bit
    delayMicroseconds(9);
  }
  delayMicroseconds(100);
}

void MotorManager::stopMotor() {
  digitalWrite(PIN_MOTOR_STEP, LOW);
}

void MotorManager::stepMotor(bool directionUp, float speedMMps) {
  if (speedMMps <= 0.0f) return;

  // Keep StealthChop2 enabled for whisper-quiet motor operation up to 80 mm/s
  if (speedMMps > 80.0f && _currentMode != MODE_SPREADCYCLE) {
    setTMCMode(MODE_SPREADCYCLE);
  } else if (speedMMps <= 80.0f && _currentMode != MODE_STEALTHCHOP) {
    setTMCMode(MODE_STEALTHCHOP);
  }

  // Limit checks (throttled every 5ms)
  static unsigned long lastSensorCheck = 0;
  if (millis() - lastSensorCheck >= 5) {
    lastSensorCheck = millis();
    if (_sensors) {
      if (directionUp) {
        if ((_sensors && _sensors->readRawTopLimit()) || _positionMM <= 0.0f) {
          if (_sensors && _sensors->readRawTopLimit()) _positionMM = 0.0f; // Calibrate home
          stopMotor();
          return;
        }
      } else {
        if ((_sensors && _sensors->readRawCapSensor()) || (_maxSoftLimitMM > 0.0f && _positionMM >= _maxSoftLimitMM)) {
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
          if ((_sensors && isTopLimitActiveDebounced(_sensors)) || _positionMM <= 0.0f) {
            if (_sensors && isTopLimitActiveDebounced(_sensors)) _positionMM = 0.0f;
            stopMotor();
            break;
          }
        } else {
          if ((_sensors && _sensors->readRawCapSensor()) || (_maxSoftLimitMM > 0.0f && _positionMM >= _maxSoftLimitMM)) {
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

static bool isTopLimitActiveDebounced(SensorManager* sensors) {
  if (!sensors) return false;
  if (!sensors->readRawTopLimit()) return false;
  // Verify over 5 consecutive samples spaced 400us apart to filter EMI noise pulses
  for (int i = 0; i < 5; i++) {
    delayMicroseconds(400);
    if (!sensors->readRawTopLimit()) return false;
  }
  return true;
}

bool MotorManager::performHoming(float speedMMps, bool (*stopCheck)()) {
  // Homing sequence: move UP at speedMMps until top limit switch engages
  if (speedMMps <= 80.0f) {
    setTMCMode(MODE_STEALTHCHOP);
  } else {
    setTMCMode(MODE_SPREADCYCLE);
  }
  
  unsigned long startTimeout = millis();
  float stepsPerSec = speedMMps * STEPS_PER_MM;
  unsigned long delayUs = (unsigned long)(1000000.0f / stepsPerSec);
  if (delayUs < 30) delayUs = 30;

  // Stage 1: Fast search towards limit switch
  while (_sensors && !isTopLimitActiveDebounced(_sensors)) {
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

    if (millis() - startTimeout > 60000) {
      stopMotor();
      setTMCMode(MODE_STEALTHCHOP);
      return false;
    }
  }

  // Stage 2: Back down 3mm slightly until switch opens
  digitalWrite(PIN_MOTOR_DIR, LOW);
  int backoffSteps = (int)(3.0f * STEPS_PER_MM);
  for (int i = 0; i < backoffSteps; i++) {
    if (stopCheck && stopCheck()) {
      stopMotor();
      setTMCMode(MODE_STEALTHCHOP);
      return false;
    }
    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    delayMicroseconds(300);

    if (!isTopLimitActiveDebounced(_sensors) && i >= (int)(1.5f * STEPS_PER_MM)) {
      break;
    }
  }

  // Stage 3: Slowly re-approach home for precision zeroing
  while (_sensors && !isTopLimitActiveDebounced(_sensors)) {
    if (stopCheck && stopCheck()) {
      stopMotor();
      setTMCMode(MODE_STEALTHCHOP);
      return false;
    }
    digitalWrite(PIN_MOTOR_DIR, HIGH);
    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    delayMicroseconds(600);
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
