#ifndef MOTOR_MANAGER_H
#define MOTOR_MANAGER_H

#include <Arduino.h>
#include <FastAccelStepper.h>
#include "PinConfig.h"
#include "SensorManager.h"

/**
 * ============================================================================
 * MOTOR MANAGER COMPONENT
 * ============================================================================
 * Controls the vertical lead-screw Z-axis carriage for dipping candle jigs.
 *
 * Hardware Architecture:
 * - Stepper Driver: Trinamic TMC2209 in SilentStepStick form factor
 * - Motor Control: Pulse (PIN_MOTOR_STEP), Direction (PIN_MOTOR_DIR), Enable (PIN_MOTOR_ENABLE)
 * - Single-Wire UART: GPIO 18 (Hardware Serial2 on ESP32) with 1k resistor for register writes
 * - Lead Screw Geometry: T8x8 (8mm lead per revolution) @ 1/8 microstepping -> 200 steps/mm
 *
 * Step Generation:
 * - Step pulses are produced by the FastAccelStepper library using ESP32 hardware timers, NOT by
 *   delayMicroseconds() loops. Every move ramps up and down at ACCEL_MM_S2, and pulse timing is no
 *   longer disturbed by touch polling, screen redraws, scale reads or sound playback.
 * - Position convention: 0.0 mm = top rest position (2 mm below the top switch), positive = downwards.
 *
 * Modes:
 *   1. StealthChop2 (silent, voltage-based chopper - less torque at high speed)
 *   2. SpreadCycle (cycle-by-cycle current control - more torque for rapid travel)
 * ============================================================================
 */

// Drive Modes for TMC2209
enum TMCMode {
  MODE_STEALTHCHOP = 0, // Silent operation for dipping and low/medium speeds
  MODE_SPREADCYCLE = 1  // High torque / rapid travel
};

// Result of a blocking move helper
enum MoveResult {
  MOVE_REACHED = 0,     // The target condition (switch hit / sensor triggered / distance done) was met
  MOVE_USER_STOP,       // stopCheck() returned true (user touched the screen)
  MOVE_TRAVEL_EXCEEDED, // Travelled the maximum allowed distance without meeting the condition (fault)
  MOVE_NO_STEPPER       // Step generator could not be created
};

class MotorManager {
public:
  MotorManager();

  // Initialization & Hardware Setup: sets GPIO directions, starts the step generator, enables the
  // TMC2209 and sends its initial UART registers
  void begin(SensorManager* sensorMgr);

  // --- Non-blocking motion primitives (safe to call every loop iteration) ---
  // Start or continue a ramped continuous move. Calling again with a new speed re-ramps smoothly.
  // runUp() automatically slows to TOP_APPROACH_SPEED_MMPS near the top switch once homed.
  void runUp(float speedMMps);
  void runDown(float speedMMps);
  void stopSmooth();        // Decelerate to a halt at ACCEL_MM_S2 (returns immediately)
  void stopMotor();         // Halt immediately without a ramp (sensor trips, faults, user stop)
  bool isMoving();
  void waitUntilStopped();  // Blocks until the current ramp-down / move has finished

  // --- Blocking helpers ---
  // Moves a relative distance (positive = down) at speedMMps with ramps, then returns
  MoveResult moveRelativeMM(float distanceMM, float speedMMps, bool (*stopCheck)() = nullptr);
  // Raises to the top switch and backs off to the 0.0 mm rest position. Returns false if the switch
  // was not found within the expected travel (carriage is stopped and marked un-homed).
  bool retractToTopRest(float upSpeedMMps);
  // Moves DOWN off the top switch by clearanceMM, then sets 0.0 mm and marks the axis homed
  void backoffFromTopLimit(float clearanceMM = 2.0f);

  // TMC2209 Dynamic Mode Switching via UART
  void setTMCMode(TMCMode mode); // Writes GCONF register to select StealthChop2 or SpreadCycle
  void applyConfigMode(int modeSetting, float speedMMps, int thresholdMMps); // Applies fixed or speed-adaptive driver mode
  TMCMode getTMCMode() const { return _currentMode; }

  // Power Enable / Disable (Active LOW on TMC2209: LOW = energized, HIGH = disabled/freewheel).
  // Disabling the coils also clears the homed flag because the carriage can then be moved by hand.
  void setMotorEnable(bool enable);
  bool isMotorEnabled() const { return _enabled; }

  // Automated Machine Routines
  // 4-stage precision homing to the top mechanical limit switch
  bool performHoming(float speedMMps, bool (*stopCheck)() = nullptr);
  // Manual "Dip Bot" single test dip cycle (lower to wax -> record wax level -> hold -> return to top rest)
  bool performDipBot(float downSpeedMMps, float upSpeedMMps, int holdTimeSec, float* outWaxLevelMM = nullptr, bool (*stopCheck)() = nullptr);
  // Melts the bottom bases of the candles at targetDepthMM (lastWaxLevelMM - jigLengthMM) for holdTimeSec, then returns to top rest
  bool performMeltBases(float targetDepthMM, float downSpeedMMps, float upSpeedMMps, int holdTimeSec, bool (*stopCheck)() = nullptr);
  // Sinks the entire jig as deep as the capacitive sensor allows for holdTimeSec to melt/clean wax off the jig, then returns to top rest
  bool performCleanJig(float downSpeedMMps, float upSpeedMMps, int holdTimeSec, float* outWaxLevelMM = nullptr, bool (*stopCheck)() = nullptr);

  // Position Tracking & Safety Soft Limits (0.0mm = top rest, positive = distance downwards in mm)
  float getCurrentPositionMM();
  void setCurrentPositionMM(float pos);
  void setMaxSoftLimitMM(float maxLimit) { _maxSoftLimitMM = maxLimit; }
  float getMaxSoftLimitMM() const { return _maxSoftLimitMM; }
  // Longest downward travel any routine may make from the top (soft limit, or ABS_MAX_TRAVEL_MM if it is off)
  float getMaxTravelMM() const { return (_maxSoftLimitMM > 0.0f) ? _maxSoftLimitMM : ABS_MAX_TRAVEL_MM; }

  // True once the top switch has been found since boot / since the coils were last released.
  // Until then the position value (and therefore the soft limit) cannot be trusted.
  bool isHomed() const { return _homed; }
  void clearHomed() { _homed = false; }

  static const float STEPS_PER_MM;            // 200.0 steps/mm
  static const float ACCEL_MM_S2;             // Ramp rate for every move
  static const float TOP_APPROACH_ZONE_MM;    // Within this distance of the top rest position...
  static const float TOP_APPROACH_SPEED_MMPS; // ...upward moves are limited to this speed
  static const float UNHOMED_MAX_SPEED_MMPS;  // Upward speed cap while the switch position is unknown (jogging)
  static const float ABS_MAX_TRAVEL_MM;       // Hard cap on any single move when no soft limit is set

private:
  SensorManager* _sensors;        // Pointer to sensor manager for real-time limit/wax checks during motion
  FastAccelStepperEngine _engine; // Hardware step pulse generator
  FastAccelStepper* _stepper;     // Z-axis stepper (nullptr if the step pin could not be attached)
  TMCMode _currentMode;           // Currently active TMC2209 chopper mode
  bool _modeInitialized;          // True once initial GCONF mode register has been written
  int _modeSetting;               // User driver mode setting (0=StealthChop, 1=SpreadCycle, 2=Adaptive)
  int _thresholdMMps;             // Speed threshold (mm/s) for Adaptive mode switching
  bool _enabled;                  // True if stepper driver coils are energized
  bool _homed;                    // True once the top switch position is known
  int _runDir;                    // Direction of the active continuous run: -1 = up, +1 = down, 0 = none
  uint32_t _runSpeedHz;           // Step rate last commanded for the active continuous run
  float _maxSoftLimitMM;          // Maximum downward travel allowed in mm before software safety stop

  enum StopCond { COND_TOP_HIT, COND_TOP_CLEAR, COND_CAP_HIT };

  void startRun(int dir, float speedMMps);
  // Runs continuously in one direction until the (debounced) condition is met
  MoveResult runUntil(bool up, float speedMMps, StopCond cond, float maxTravelMM, bool (*stopCheck)() = nullptr);
  bool holdFor(int holdTimeSec, bool (*stopCheck)());

  // Internal UART Register Writer (frames 64-bit TMC2209 datagram with CRC8 and sends over Serial2)
  void sendUARTCommand(uint8_t reg, uint32_t val);
};

#endif // MOTOR_MANAGER_H
