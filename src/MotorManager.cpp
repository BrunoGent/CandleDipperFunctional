#include "MotorManager.h"

// Standard 8mm lead screw T8x8 (8mm travel per revolution) with 200 full steps/rev motor
// and 1/8 microstepping = (200 * 8) / 8mm = 200 steps per mm
const float MotorManager::STEPS_PER_MM = 200.0f;

MotorManager::MotorManager()
  : _sensors(nullptr),
    _currentMode(MODE_STEALTHCHOP),
    _modeInitialized(false),
    _modeSetting(1),
    _thresholdMMps(30),
    _enabled(false),
    _positionMM(0.0f),
    _maxSoftLimitMM(500.0f) {}

void MotorManager::begin(SensorManager* sensorMgr) {
  _sensors = sensorMgr;

  // Configure stepper control pins as outputs
  pinMode(PIN_MOTOR_STEP, OUTPUT);
  pinMode(PIN_MOTOR_DIR, OUTPUT);
  pinMode(PIN_MOTOR_ENABLE, OUTPUT);
  pinMode(PIN_MOTOR_UART, OUTPUT);

  digitalWrite(PIN_MOTOR_STEP, LOW);
  digitalWrite(PIN_MOTOR_DIR, LOW);
  
  // Enable driver immediately on boot (Active LOW: LOW = coils energized)
  digitalWrite(PIN_MOTOR_ENABLE, LOW);
  _enabled = true;

  // Initialize Hardware Serial2 once on GPIO 18 for single-wire TMC UART (both RX and TX mapped to PIN_MOTOR_UART)
  Serial2.begin(115200, SERIAL_8N1, PIN_MOTOR_UART, PIN_MOTOR_UART);
  delay(10);

  // Initialize TMC2209 driver registers over UART:
  // 1. IHOLD_IRUN (0x10): IRUN=16 (~600mA RMS), IHOLD=8, IHOLDDELAY=6
  sendUARTCommand(0x10, 0x00061008);
  delay(5);

  // 2. CHOPCONF (0x6C): MRES=5 (1/8 Microsteps = 200 steps/mm on T8x8) with 256 Interpolation (intpol=1 -> 0x15000053)
  sendUARTCommand(0x6C, 0x15000053);
  delay(5);

  // 3. PWMCONF (0x70): StealthChop2 PWM autoscale configuration
  sendUARTCommand(0x70, 0xC40D001D);
  delay(5);

  // 4. Set initial TMC mode to StealthChop2 (GCONF mstep_reg_select = 1)
  setTMCMode(MODE_STEALTHCHOP);
}

void MotorManager::setTMCMode(TMCMode mode) {
  if (_modeInitialized && _currentMode == mode) {
    return; // Avoid redundant UART writes when mode has not changed
  }
  _currentMode = mode;
  _modeInitialized = true;

  // Send TMC2209 UART packet to set GCONF register (0x00)
  // Bit 6 (0x40) = mstep_reg_select: 1 = Use register CHOPCONF for microsteps instead of MS1/MS2 pins
  // Bit 7 (0x80) = multistep_filt: 1 = enable step pulse filter
  // Bit 2 (0x04) = en_SpreadCycle: 1 = SpreadCycle, 0 = StealthChop2
  if (mode == MODE_STEALTHCHOP) {
    sendUARTCommand(0x00, 0x000000C0); // StealthChop2 enabled (silent operation)
  } else {
    sendUARTCommand(0x00, 0x000000C4); // SpreadCycle enabled (high torque)
  }
}

// Selects StealthChop2 or SpreadCycle based on user setting:
// modeSetting 0 = Always StealthChop, 1 = Always SpreadCycle, 2 = Adaptive (switches to SpreadCycle at >= thresholdMMps)
void MotorManager::applyConfigMode(int modeSetting, float speedMMps, int thresholdMMps) {
  _modeSetting = modeSetting;
  _thresholdMMps = thresholdMMps;
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

// Toggles power to the stepper motor coils (LOW = locked/enabled, HIGH = freewheel/disabled)
void MotorManager::setMotorEnable(bool enable) {
  _enabled = enable;
  digitalWrite(PIN_MOTOR_ENABLE, enable ? LOW : HIGH);
}

// Computes the 8-bit CRC checksum required at the end of every TMC2209 UART datagram (polynomial 0x8C)
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

// Sends an 8-byte write datagram to a TMC2209 register over single-wire Serial2
void MotorManager::sendUARTCommand(uint8_t reg, uint32_t val) {
  uint8_t msg[8];
  msg[0] = 0x05; // TMC2209 Sync byte
  msg[1] = 0x00; // Slave address 0
  msg[2] = reg | 0x80; // Write register (bit 7 = 1 indicates write operation)
  msg[3] = (val >> 24) & 0xFF; // 32-bit payload (MSB first)
  msg[4] = (val >> 16) & 0xFF;
  msg[5] = (val >> 8) & 0xFF;
  msg[6] = val & 0xFF;
  msg[7] = calcTMC2209CRC(msg, 7); // CRC8 byte

  Serial2.write(msg, 8);
  Serial2.flush(); // Wait until all 8 bytes have finished transmitting
}

void MotorManager::stopMotor() {
  digitalWrite(PIN_MOTOR_STEP, LOW);
}

// Gently backs the carriage DOWN off the top mechanical limit switch by clearanceMM (default 2.0mm)
// and zeroes _positionMM so the switch does not exert mechanical force on the load cell during weighing.
void MotorManager::backoffFromTopLimit(float clearanceMM) {
  _positionMM = 0.0f;
  digitalWrite(PIN_MOTOR_DIR, LOW); // DOWN
  delayMicroseconds(50);
  int steps = (int)(clearanceMM * STEPS_PER_MM);
  for (int i = 0; i < steps; i++) {
    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    delayMicroseconds(1000); // 5 mm/s gentle relief move
  }
  _positionMM = 0.0f;
  stopMotor();
}

// Generates a smooth train of step pulses for 'burstMs' milliseconds before returning to loop()
// Used by Manual Jogging (UP/DOWN buttons) and Dip Bot so UI/touch checks can run between bursts
void MotorManager::stepMotorBurst(bool directionUp, float speedMMps, uint32_t burstMs) {
  if (speedMMps <= 0.0f) return;
  setMotorEnable(true);

  // Apply configured TMC2209 chopper mode (StealthChop, SpreadCycle, or Adaptive at _thresholdMMps)
  applyConfigMode(_modeSetting, speedMMps, _thresholdMMps);

  digitalWrite(PIN_MOTOR_DIR, directionUp ? HIGH : LOW);

  // Convert linear speed (mm/s) to microsecond delay between step pulses
  float stepsPerSec = speedMMps * STEPS_PER_MM;
  if (stepsPerSec <= 0.0f) return;
  unsigned long intervalMicros = (unsigned long)(1000000.0f / stepsPerSec);
  if (intervalMicros < 10) intervalMicros = 10;

  float stepDistMM = 1.0f / STEPS_PER_MM;
  unsigned long startMs = millis();
  unsigned long lastSensorCheck = 0;

  // Run tight stepping loop for the duration of burstMs (e.g. 15ms or 50ms)
  while (millis() - startMs < burstMs) {
    // Check hardware limit/wax sensors every 5ms inside the burst
    if (millis() - lastSensorCheck >= 5) {
      lastSensorCheck = millis();
      if (_sensors) {
        if (directionUp) {
          if (_sensors->readRawTopLimit()) {
            _positionMM = 0.0f;
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
    }

    // Emit a 3-microsecond HIGH step pulse
    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_MOTOR_STEP, LOW);

    // Update tracked position in millimeters
    if (directionUp) {
      _positionMM -= stepDistMM;
      if (_positionMM < 0.0f) _positionMM = 0.0f;
    } else {
      _positionMM += stepDistMM;
    }

    // Wait remaining interval before next step pulse
    if (intervalMicros > 3) {
      delayMicroseconds(intervalMicros - 3);
    }
  }
}

// --- Fast Sensor Helpers for Homing ---
// Double-checks the top limit switch with a 2us delay to ensure a genuine trigger edge
static inline bool checkTopLimitFast(SensorManager* sensors) {
  if (!sensors) return false;
  if (!sensors->readRawTopLimit()) return false;
  delayMicroseconds(2);
  return sensors->readRawTopLimit();
}

// Double-checks that the top limit switch has returned to closed (unpressed) state
static inline bool checkTopLimitCleared(SensorManager* sensors) {
  if (!sensors) return true;
  if (sensors->readRawTopLimit()) return false;
  delayMicroseconds(2);
  return !sensors->readRawTopLimit();
}

// 4-Stage Precision Homing Sequence:
// Stage 1: Fast seek UP until the top limit switch is hit.
// Stage 2: Back off DOWN gently until switch clears, plus 1.0mm extra clearance.
// Stage 3: Creep UP slowly (3 mm/s) until switch triggers again for high repeatability.
// Stage 4: Creep DOWN very slowly (1 mm/s) until switch just releases, set 0.0mm, then move 2.0mm down for switch relief.
bool MotorManager::performHoming(float speedMMps, bool (*stopCheck)()) {
  setMotorEnable(true); // Ensure motor powered
  setTMCMode(MODE_STEALTHCHOP);
  
  unsigned long startTimeout = millis();
  float stepsPerSec = speedMMps * STEPS_PER_MM;
  if (stepsPerSec <= 0.0f) stepsPerSec = 40.0f * STEPS_PER_MM;
  unsigned long delayUs = (unsigned long)(1000000.0f / stepsPerSec);
  if (delayUs < 40) delayUs = 40;

  // Stage 1: Fast Seek UP towards top limit switch
  digitalWrite(PIN_MOTOR_DIR, HIGH); // UP
  delayMicroseconds(50);

  while (_sensors && !checkTopLimitFast(_sensors)) {
    if (stopCheck && stopCheck()) {
      stopMotor();
      return false;
    }

    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    delayMicroseconds(delayUs);

    if (millis() - startTimeout > 15000) { // 15-second safety timeout
      stopMotor();
      return false;
    }
  }

  stopMotor();
  delay(100);

  // Stage 2: Backoff DOWN gently until switch clears
  digitalWrite(PIN_MOTOR_DIR, LOW); // DOWN
  delayMicroseconds(50);

  int maxBackoffSteps = (int)(10.0f * STEPS_PER_MM); // up to 10mm backoff
  int stepsBack = 0;
  while (_sensors && !checkTopLimitCleared(_sensors) && stepsBack < maxBackoffSteps) {
    if (stopCheck && stopCheck()) {
      stopMotor();
      return false;
    }
    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    delayMicroseconds(400); // 12.5 mm/s gentle backoff
    stepsBack++;
  }

  // Extra 1.0mm clearance backoff
  for (int i = 0; i < (int)(1.0f * STEPS_PER_MM); i++) {
    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    delayMicroseconds(400);
  }

  stopMotor();
  delay(100);

  // Stage 3: Slow precision approach UP (3 mm/s)
  digitalWrite(PIN_MOTOR_DIR, HIGH); // UP
  delayMicroseconds(50);

  while (_sensors && !checkTopLimitFast(_sensors)) {
    if (stopCheck && stopCheck()) {
      stopMotor();
      return false;
    }
    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    delayMicroseconds(1666); // 3 mm/s precision approach
  }

  stopMotor();
  delay(100);

  // Stage 4 (Standard Practice): Move DOWN slowly (1 mm/s) until switch JUST RELEASES!
  digitalWrite(PIN_MOTOR_DIR, LOW); // DOWN
  delayMicroseconds(50);

  while (_sensors && !checkTopLimitCleared(_sensors)) {
    if (stopCheck && stopCheck()) {
      stopMotor();
      return false;
    }
    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    delayMicroseconds(5000); // 1 mm/s release creep
  }

  // Back off an additional 2.0mm clearance so carriage sits comfortably off switch and zero position
  backoffFromTopLimit(2.0f);
  return true;
}

// Immediately raises the carriage back up to the top limit switch and backs off 2.0mm to the rest position (0.0mm).
// Used when a wax collision is detected or a submerged hold is aborted so the candle jig is never left stuck in molten wax.
void MotorManager::retractToTopRest(float upSpeedMMps) {
  setMotorEnable(true);
  float speed = (upSpeedMMps > 0.0f) ? upSpeedMMps : 40.0f;
  applyConfigMode(_modeSetting, speed, _thresholdMMps);

  digitalWrite(PIN_MOTOR_DIR, HIGH); // UP
  delayMicroseconds(50);

  float stepsPerSec = speed * STEPS_PER_MM;
  unsigned long delayUs = (unsigned long)(1000000.0f / stepsPerSec);
  if (delayUs < 20) delayUs = 20;
  float stepDistMM = 1.0f / STEPS_PER_MM;

  unsigned long startTimeout = millis();
  while (_sensors && !_sensors->readRawTopLimit()) {
    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_MOTOR_STEP, LOW);

    _positionMM -= stepDistMM;
    if (_positionMM < 0.0f) _positionMM = 0.0f;

    delayMicroseconds(delayUs);
    if (millis() - startTimeout > 35000) break; // 35-second safety timeout guard
  }

  stopMotor();
  backoffFromTopLimit(2.0f);
}

// Executes a single manual test dip ("Dip Bot" button on the Manual Control screen):
// 1. Lowers carriage until the capacitive sensor touches the wax surface (or 35s timeout) and records the wax level.
// 2. Holds in the wax for 'holdTimeSec' seconds.
// 3. Raises carriage back up to the top limit switch and backs off 2.0mm.
bool MotorManager::performDipBot(float downSpeedMMps, float upSpeedMMps, int holdTimeSec, float* outWaxLevelMM, bool (*stopCheck)()) {
  setMotorEnable(true);
  
  // 1. Move DOWN at downSpeedMMps until capacitive wax sensor triggers
  unsigned long startTimeout = millis();
  while (_sensors && !_sensors->readRawCapSensor()) {
    if (stopCheck && stopCheck()) {
      stopMotor();
      return false;
    }
    stepMotorBurst(false, downSpeedMMps, 15);
    if (_maxSoftLimitMM > 0.0f && _positionMM >= _maxSoftLimitMM) break;
    if (millis() - startTimeout > 35000) break; // Timeout guard
  }

  stopMotor();

  // Record measured wax surface level (Z distance from top home where capacitive sensor triggered)
  if (outWaxLevelMM && _sensors && _sensors->readRawCapSensor()) {
    *outWaxLevelMM = _positionMM;
  }

  // 2. Hold in wax for holdTimeSec
  if (holdTimeSec > 0) {
    unsigned long holdStart = millis();
    while (millis() - holdStart < (unsigned long)(holdTimeSec * 1000)) {
      if (stopCheck && stopCheck()) {
        retractToTopRest(upSpeedMMps); // Do not leave jig sitting in molten wax on manual stop
        return false;
      }
      delay(20);
    }
  }

  // 3. Move back UP to top limit switch at upSpeedMMps
  unsigned long upStartTimeout = millis();
  while (_sensors && !_sensors->readRawTopLimit()) {
    if (stopCheck && stopCheck()) {
      stopMotor();
      return false;
    }
    stepMotorBurst(true, upSpeedMMps, 15);
    if (millis() - upStartTimeout > 35000) break;
  }

  // Relieve switch pressure with gentle 2mm clearance backoff
  backoffFromTopLimit(2.0f);
  return true;
}

// Melts the bottom bases of finished candles to clear drips and create a flat base finish:
// 1. Lowers carriage from top home (0.0mm) down to targetDepthMM (lastWaxLevelMM - jigLengthMM).
// 2. Holds the candle bottoms submerged in hot wax for 'holdTimeSec' seconds.
// 3. Raises carriage back up to the top rest position (0.0mm).
bool MotorManager::performMeltBases(float targetDepthMM, float downSpeedMMps, float upSpeedMMps, int holdTimeSec, bool (*stopCheck)()) {
  if (targetDepthMM <= 0.0f) return false;
  setMotorEnable(true);

  // 1. Lower carriage until _positionMM reaches targetDepthMM (or capacitive sensor / soft limit triggers)
  unsigned long startTimeout = millis();
  while (_positionMM < targetDepthMM) {
    if (_sensors && _sensors->readRawCapSensor()) break; // Safety stop if wax surface reached early
    if (_maxSoftLimitMM > 0.0f && _positionMM >= _maxSoftLimitMM) break;
    if (stopCheck && stopCheck()) {
      retractToTopRest(upSpeedMMps);
      return false;
    }
    stepMotorBurst(false, downSpeedMMps, 15);
    if (millis() - startTimeout > 35000) break;
  }

  stopMotor();

  // 2. Hold candle bases in molten wax for holdTimeSec seconds
  if (holdTimeSec > 0) {
    unsigned long holdStart = millis();
    while (millis() - holdStart < (unsigned long)(holdTimeSec * 1000)) {
      if (stopCheck && stopCheck()) {
        retractToTopRest(upSpeedMMps); // Lift candles out of wax before stopping
        return false;
      }
      delay(20);
    }
  }

  // 3. Raise back up to the top rest position
  retractToTopRest(upSpeedMMps);
  return true;
}

// Sinks the entire candle jig as deep as the capacitive sensor allows to melt and clean off old wax:
// 1. Lowers carriage until the capacitive sensor detects the wax surface (updating *outWaxLevelMM).
// 2. Holds the jig submerged in hot wax for 'holdTimeSec' seconds.
// 3. Raises carriage back up to the top rest position (0.0mm).
bool MotorManager::performCleanJig(float downSpeedMMps, float upSpeedMMps, int holdTimeSec, float* outWaxLevelMM, bool (*stopCheck)()) {
  return performDipBot(downSpeedMMps, upSpeedMMps, holdTimeSec, outWaxLevelMM, stopCheck);
}





