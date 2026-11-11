#include "MotorManager.h"

// Standard 8mm lead screw T8x8 (8mm travel per revolution) with 200 full steps/rev motor
// and 1/8 microstepping = (200 * 8) / 8mm = 200 steps per mm
const float MotorManager::STEPS_PER_MM = 200.0f;

// Every move ramps at this rate. 300 mm/s^2 reaches 60 mm/s in 0.2 s; lower it if the motor still stalls
// with a heavy jig, raise it if moves feel sluggish.
const float MotorManager::ACCEL_MM_S2 = 300.0f;

// Upward moves slow down close to the top so the switch is always met gently
const float MotorManager::TOP_APPROACH_ZONE_MM = 12.0f;
const float MotorManager::TOP_APPROACH_SPEED_MMPS = 10.0f;
const float MotorManager::UNHOMED_MAX_SPEED_MMPS = 20.0f;
const float MotorManager::ABS_MAX_TRAVEL_MM = 1000.0f;

// The top switch / wax sensor must read active continuously for this long before a move reacts to it
static const uint32_t SENSOR_DEBOUNCE_US = 2000;

MotorManager::MotorManager()
  : _sensors(nullptr),
    _stepper(nullptr),
    _currentMode(MODE_STEALTHCHOP),
    _modeInitialized(false),
    _modeSetting(1),
    _thresholdMMps(30),
    _enabled(false),
    _homed(false),
    _runDir(0),
    _runSpeedHz(0),
    _maxSoftLimitMM(500.0f) {}

void MotorManager::begin(SensorManager* sensorMgr) {
  _sensors = sensorMgr;

  // Configure stepper control pins as outputs
  pinMode(PIN_MOTOR_DIR, OUTPUT);
  pinMode(PIN_MOTOR_ENABLE, OUTPUT);
  pinMode(PIN_MOTOR_UART, OUTPUT);
  digitalWrite(PIN_MOTOR_DIR, LOW);

  // Hardware-timed step pulse generator with acceleration ramps
  _engine.init();
  _stepper = _engine.stepperConnectToPin(PIN_MOTOR_STEP);
  if (_stepper) {
    // Position counts UP when moving DOWN. DIR HIGH = UP on this machine, so DIR HIGH counts down.
    _stepper->setDirectionPin(PIN_MOTOR_DIR, false);
    _stepper->setAcceleration((int32_t)(ACCEL_MM_S2 * STEPS_PER_MM));
    _stepper->setCurrentPosition(0);
  } else {
    Serial.println("MotorManager: could not attach step generator to PIN_MOTOR_STEP!");
  }

  // Enable driver immediately on boot (Active LOW: LOW = coils energized)
  digitalWrite(PIN_MOTOR_ENABLE, LOW);
  _enabled = true;

  // Initialize Hardware Serial2 once on GPIO 18 for single-wire TMC UART (both RX and TX mapped to PIN_MOTOR_UART)
  Serial2.begin(115200, SERIAL_8N1, PIN_MOTOR_UART, PIN_MOTOR_UART);
  delay(10);

  // Initialize TMC2209 driver registers over UART:
  // 1. IHOLD_IRUN (0x10): IHOLDDELAY=6, IRUN=16, IHOLD=8.
  //    With the standard 0.11 ohm sense resistors and vsense=0 this is roughly 0.9 A RMS while moving
  //    and 0.5 A RMS holding: I_rms = (IRUN+1)/32 * 0.325V / (0.11 + 0.02 ohm) / 1.414.
  //    If the motor runs hot, lower IRUN (bits 12..8), e.g. 0x00060A05 for about 0.6 A RMS.
  sendUARTCommand(0x10, 0x00061008);
  delay(5);

  // 2. CHOPCONF (0x6C): MRES=5 (1/8 Microsteps = 200 steps/mm on T8x8) with 256 Interpolation (intpol=1 -> 0x15000053)
  sendUARTCommand(0x6C, 0x15000053);
  delay(5);

  // 3. PWMCONF (0x70): StealthChop2 PWM autoscale configuration
  sendUARTCommand(0x70, 0xC40D001D);
  delay(5);

  // 4. Set initial TMC mode to StealthChop2 (applyConfigMode() then selects the user's saved mode)
  setTMCMode(MODE_STEALTHCHOP);
}

void MotorManager::setTMCMode(TMCMode mode) {
  if (_modeInitialized && _currentMode == mode) {
    return; // Avoid redundant UART writes when mode has not changed
  }
  _currentMode = mode;
  _modeInitialized = true;

  // Send TMC2209 UART packet to set GCONF register (0x00)
  // Bit 6 (0x40) = pdn_disable: 1 = PDN_UART pin is used for UART only
  // Bit 7 (0x80) = mstep_reg_select: 1 = microstep resolution comes from CHOPCONF.MRES instead of the MS1/MS2 pins
  // Bit 2 (0x04) = en_SpreadCycle: 1 = SpreadCycle, 0 = StealthChop2
  // Bit 0 (0x01) = I_scale_analog is left at 0, so motor current is set by IHOLD_IRUN and NOT by the VREF pot
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
  if (!enable) {
    stopMotor();
    _homed = false; // With the coils released the carriage can be moved by hand, so position is no longer trusted
  }
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

// ============================================================================
// Motion primitives
// ============================================================================

float MotorManager::getCurrentPositionMM() {
  if (!_stepper) return 0.0f;
  return (float)_stepper->getCurrentPosition() / STEPS_PER_MM;
}

void MotorManager::setCurrentPositionMM(float pos) {
  if (_stepper) _stepper->setCurrentPosition((int32_t)lroundf(pos * STEPS_PER_MM));
}

bool MotorManager::isMoving() {
  return _stepper && _stepper->isRunning();
}

void MotorManager::waitUntilStopped() {
  while (isMoving()) delay(1);
  _runDir = 0;
}

// Starts (or re-targets) a continuous ramped run. Only talks to the step generator when something changed,
// so it is cheap to call on every loop iteration.
void MotorManager::startRun(int dir, float speedMMps) {
  if (!_stepper || speedMMps <= 0.0f) return;
  if (!_enabled) setMotorEnable(true);

  uint32_t hz = (uint32_t)(speedMMps * STEPS_PER_MM);
  if (hz < 20) hz = 20;

  bool running = _stepper->isRunning();
  if (running && _runDir == dir && _runSpeedHz == hz) return;

  _stepper->setSpeedInHz(hz);
  if (running && _runDir == dir) {
    _stepper->applySpeedAcceleration(); // Same direction: ramp to the new speed
  } else {
    if (dir < 0) _stepper->runBackward();
    else         _stepper->runForward();
  }
  _runDir = dir;
  _runSpeedHz = hz;
}

void MotorManager::runUp(float speedMMps) {
  // Meet the top switch gently once we know where it is
  if (_homed && getCurrentPositionMM() < TOP_APPROACH_ZONE_MM && speedMMps > TOP_APPROACH_SPEED_MMPS) {
    speedMMps = TOP_APPROACH_SPEED_MMPS;
  }
  startRun(-1, speedMMps);
}

void MotorManager::runDown(float speedMMps) {
  startRun(+1, speedMMps);
}

void MotorManager::stopSmooth() {
  if (_stepper && _stepper->isRunning()) _stepper->stopMove();
  _runDir = 0;
}

void MotorManager::stopMotor() {
  if (_stepper && _stepper->isRunning()) {
    // Stops pulse generation at once and discards any queued steps
    _stepper->forceStopAndNewPosition(_stepper->getCurrentPosition());
    // Give the step generator a moment to report idle so a following move starts from a clean state
    unsigned long t0 = millis();
    while (_stepper->isRunning() && millis() - t0 < 50) delay(1);
  }
  _runDir = 0;
}

// Runs in one direction until the debounced condition is true, the user stops it, or maxTravelMM is used up.
// The motor is always stopped when this returns.
MoveResult MotorManager::runUntil(bool up, float speedMMps, StopCond cond, float maxTravelMM, bool (*stopCheck)()) {
  if (!_stepper) return MOVE_NO_STEPPER;
  if (!_sensors) return MOVE_TRAVEL_EXCEEDED;

  Debounce db;
  float startPos = getCurrentPositionMM();

  while (true) {
    bool raw = false;
    switch (cond) {
      case COND_TOP_HIT:   raw = _sensors->readRawTopLimit();  break;
      case COND_TOP_CLEAR: raw = !_sensors->readRawTopLimit(); break;
      case COND_CAP_HIT:   raw = _sensors->readRawCapSensor(); break;
    }
    if (db.update(raw, SENSOR_DEBOUNCE_US)) {
      stopMotor();
      return MOVE_REACHED;
    }
    if (fabsf(getCurrentPositionMM() - startPos) >= maxTravelMM) {
      stopMotor();
      return MOVE_TRAVEL_EXCEEDED;
    }
    if (stopCheck && stopCheck()) {
      stopMotor();
      return MOVE_USER_STOP;
    }

    if (up) runUp(speedMMps);
    else    runDown(speedMMps);
    delayMicroseconds(300);
  }
}

// Ramped relative move (positive = down). Blocks until finished.
MoveResult MotorManager::moveRelativeMM(float distanceMM, float speedMMps, bool (*stopCheck)()) {
  if (!_stepper) return MOVE_NO_STEPPER;
  if (speedMMps <= 0.0f) return MOVE_REACHED;
  if (!_enabled) setMotorEnable(true);
  waitUntilStopped();

  uint32_t hz = (uint32_t)(speedMMps * STEPS_PER_MM);
  if (hz < 20) hz = 20;
  _stepper->setSpeedInHz(hz);
  _stepper->move((int32_t)lroundf(distanceMM * STEPS_PER_MM));

  while (_stepper->isRunning()) {
    if (stopCheck && stopCheck()) {
      stopMotor();
      return MOVE_USER_STOP;
    }
    delay(1);
  }
  _runDir = 0;
  return MOVE_REACHED;
}

// Gently backs the carriage DOWN off the top mechanical limit switch by clearanceMM (default 2.0mm)
// and zeroes the position so the switch does not exert mechanical force on the load cell during weighing.
// Only call this when the carriage is genuinely at the switch: it marks the axis as homed.
void MotorManager::backoffFromTopLimit(float clearanceMM) {
  stopMotor();
  moveRelativeMM(clearanceMM, 5.0f); // 5 mm/s gentle relief move
  setCurrentPositionMM(0.0f);
  _homed = true;
}

// 4-Stage Precision Homing Sequence:
// Stage 1: Fast seek UP until the top limit switch is hit.
// Stage 2: Back off DOWN gently until switch clears, plus 1.0mm extra clearance.
// Stage 3: Creep UP slowly (3 mm/s) until switch triggers again for high repeatability.
// Stage 4: Creep DOWN very slowly (1 mm/s) until switch just releases, then move 2.0mm down and set 0.0mm.
// Every stage has a distance limit, so a broken or unplugged switch ends in a clean failure instead of
// driving the carriage into the frame.
bool MotorManager::performHoming(float speedMMps, bool (*stopCheck)()) {
  if (!_stepper || !_sensors) return false;
  setMotorEnable(true); // Ensure motor powered
  setTMCMode(MODE_STEALTHCHOP);
  _homed = false;

  if (speedMMps <= 0.0f) speedMMps = 40.0f;

  // Stage 1: Fast Seek UP towards top limit switch (from anywhere in the travel, so allow the full stroke)
  if (runUntil(true, speedMMps, COND_TOP_HIT, getMaxTravelMM() + 50.0f, stopCheck) != MOVE_REACHED) return false;
  delay(100);

  // Stage 2: Backoff DOWN gently until switch clears, plus 1.0mm clearance
  if (runUntil(false, 12.5f, COND_TOP_CLEAR, 10.0f, stopCheck) != MOVE_REACHED) return false;
  if (moveRelativeMM(1.0f, 12.5f, stopCheck) != MOVE_REACHED) return false;
  delay(100);

  // Stage 3: Slow precision approach UP (3 mm/s)
  if (runUntil(true, 3.0f, COND_TOP_HIT, 15.0f, stopCheck) != MOVE_REACHED) return false;
  delay(100);

  // Stage 4: Move DOWN slowly (1 mm/s) until switch JUST RELEASES
  if (runUntil(false, 1.0f, COND_TOP_CLEAR, 10.0f, stopCheck) != MOVE_REACHED) return false;

  // Back off an additional 2.0mm clearance so carriage sits comfortably off switch, and zero position
  backoffFromTopLimit(2.0f);
  return true;
}

// Raises the carriage back up to the top limit switch and backs off 2.0mm to the rest position (0.0mm).
// Used when a wax collision is detected or a submerged hold is aborted so the candle jig is never left stuck in molten wax.
// Returns false (carriage stopped, axis un-homed) if the switch is not found within the expected travel.
bool MotorManager::retractToTopRest(float upSpeedMMps) {
  if (!_stepper || !_sensors) return false;
  setMotorEnable(true);
  float speed = (upSpeedMMps > 0.0f) ? upSpeedMMps : 40.0f;
  applyConfigMode(_modeSetting, speed, _thresholdMMps);

  // When homed we know how far away the switch is; allow a margin for lost steps. Otherwise allow the full stroke.
  float pos = getCurrentPositionMM();
  float maxTravel = _homed ? (max(pos, 0.0f) + 30.0f) : (getMaxTravelMM() + 50.0f);

  if (runUntil(true, speed, COND_TOP_HIT, maxTravel, nullptr) != MOVE_REACHED) {
    _homed = false;
    return false;
  }
  backoffFromTopLimit(2.0f);
  return true;
}

// Waits holdTimeSec seconds. Returns false if the user touched the screen during the wait.
bool MotorManager::holdFor(int holdTimeSec, bool (*stopCheck)()) {
  if (holdTimeSec <= 0) return true;
  unsigned long holdStart = millis();
  unsigned long holdMs = (unsigned long)holdTimeSec * 1000UL;
  while (millis() - holdStart < holdMs) {
    if (stopCheck && stopCheck()) return false;
    delay(20);
  }
  return true;
}

// Executes a single manual test dip ("Dip Bot" button on the Manual Control screen):
// 1. Lowers carriage until the capacitive sensor touches the wax surface and records the wax level.
// 2. Holds in the wax for 'holdTimeSec' seconds.
// 3. Raises carriage back up to the top limit switch and backs off 2.0mm.
// Returns false if stopped by the user, if the wax was not found before the soft limit, or if the
// sensor already reads "wax" before the move starts (dirty or faulty sensor).
bool MotorManager::performDipBot(float downSpeedMMps, float upSpeedMMps, int holdTimeSec, float* outWaxLevelMM, bool (*stopCheck)()) {
  if (!_stepper || !_sensors) return false;
  setMotorEnable(true);

  // A sensor that is already triggered cannot find the wax surface
  if (_sensors->readRawCapSensor()) {
    delay(20);
    if (_sensors->readRawCapSensor()) return false;
  }

  if (downSpeedMMps <= 0.0f) downSpeedMMps = 25.0f;
  applyConfigMode(_modeSetting, downSpeedMMps, _thresholdMMps);

  // 1. Move DOWN until capacitive wax sensor triggers, never past the soft limit
  float remaining = getMaxTravelMM() - getCurrentPositionMM();
  if (remaining <= 0.0f) return false;
  MoveResult r = runUntil(false, downSpeedMMps, COND_CAP_HIT, remaining, stopCheck);
  if (r == MOVE_USER_STOP) return false; // Stay where the user stopped it
  if (r != MOVE_REACHED) {
    retractToTopRest(upSpeedMMps);       // Wax not found: do not hold in mid-air, just go back up
    return false;
  }

  // Record measured wax surface level (Z distance from top rest where capacitive sensor triggered)
  if (outWaxLevelMM) *outWaxLevelMM = getCurrentPositionMM();

  // 2. Hold in wax for holdTimeSec
  if (!holdFor(holdTimeSec, stopCheck)) {
    retractToTopRest(upSpeedMMps); // Do not leave jig sitting in molten wax on manual stop
    return false;
  }

  // 3. Move back UP to the top rest position
  return retractToTopRest(upSpeedMMps);
}

// Melts the bottom bases of finished candles to clear drips and create a flat base finish:
// 1. Lowers carriage down to targetDepthMM (lastWaxLevelMM - jigLengthMM).
// 2. Holds the candle bottoms submerged in hot wax for 'holdTimeSec' seconds.
// 3. Raises carriage back up to the top rest position (0.0mm).
bool MotorManager::performMeltBases(float targetDepthMM, float downSpeedMMps, float upSpeedMMps, int holdTimeSec, bool (*stopCheck)()) {
  if (!_stepper || targetDepthMM <= 0.0f) return false;
  setMotorEnable(true);
  if (downSpeedMMps <= 0.0f) downSpeedMMps = 25.0f;
  applyConfigMode(_modeSetting, downSpeedMMps, _thresholdMMps);
  waitUntilStopped();

  float target = min(targetDepthMM, getMaxTravelMM());
  Debounce capDb;

  // 1. Lower carriage until the position reaches the target (or the capacitive sensor reaches wax early)
  _stepper->setSpeedInHz((uint32_t)(downSpeedMMps * STEPS_PER_MM));
  _stepper->moveTo((int32_t)lroundf(target * STEPS_PER_MM));
  while (_stepper->isRunning()) {
    if (_sensors && capDb.update(_sensors->readRawCapSensor(), SENSOR_DEBOUNCE_US)) {
      stopMotor(); // Safety stop if wax surface reached early
      break;
    }
    if (stopCheck && stopCheck()) {
      stopMotor();
      retractToTopRest(upSpeedMMps);
      return false;
    }
    delayMicroseconds(300);
  }
  _runDir = 0;

  // 2. Hold candle bases in molten wax for holdTimeSec seconds
  if (!holdFor(holdTimeSec, stopCheck)) {
    retractToTopRest(upSpeedMMps); // Lift candles out of wax before stopping
    return false;
  }

  // 3. Raise back up to the top rest position
  return retractToTopRest(upSpeedMMps);
}

// Sinks the entire candle jig as deep as the capacitive sensor allows to melt and clean off old wax:
// 1. Lowers carriage until the capacitive sensor detects the wax surface (updating *outWaxLevelMM).
// 2. Holds the jig submerged in hot wax for 'holdTimeSec' seconds.
// 3. Raises carriage back up to the top rest position (0.0mm).
bool MotorManager::performCleanJig(float downSpeedMMps, float upSpeedMMps, int holdTimeSec, float* outWaxLevelMM, bool (*stopCheck)()) {
  return performDipBot(downSpeedMMps, upSpeedMMps, holdTimeSec, outWaxLevelMM, stopCheck);
}
