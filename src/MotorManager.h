#ifndef MOTOR_MANAGER_H
#define MOTOR_MANAGER_H

#include <Arduino.h>
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
 * - Single-Wire UART: GPIO 18 (Hardware Serial2 on ESP32) with 1k resistor for live register access
 * - Modes:
 *   1. StealthChop2 (Silent, voltage-based chopper, whisper-quiet up to 80 mm/s)
 *   2. SpreadCycle (Cycle-by-cycle current control, high-torque for rapid travel >80 mm/s)
 * - Lead Screw Geometry: T8x8 (8mm pitch per revolution) @ 1/8 microstepping -> 200 steps/mm
 * ============================================================================
 */

// Drive Modes for TMC2209
enum TMCMode {
  MODE_STEALTHCHOP = 0, // Silent operation for dipping and low/medium speeds
  MODE_SPREADCYCLE = 1  // High torque / rapid travel
};

class MotorManager {
public:
  MotorManager();

  // Initialization & Hardware Setup: sets GPIO directions, enables TMC2209, and sends initial UART registers
  void begin(SensorManager* sensorMgr);
  
  // Motion Execution Commands
  void stepMotorBurst(bool directionUp, float speedMMps, uint32_t burstMs = 20); // Steps continuously for burstMs milliseconds
  void stopMotor(); // Pulls STEP pin LOW to halt pulse generation
  void backoffFromTopLimit(float clearanceMM = 2.0f); // Moves carriage gently DOWN off the top limit switch and zeroes position
  void retractToTopRest(float upSpeedMMps); // Immediately raises carriage out of wax to the top rest position (0.0mm)

  // TMC2209 Dynamic Mode Switching via UART
  void setTMCMode(TMCMode mode); // Writes GCONF register to select StealthChop2 or SpreadCycle
  void applyConfigMode(int modeSetting, float speedMMps, int thresholdMMps); // Applies fixed or speed-adaptive driver mode
  TMCMode getTMCMode() const { return _currentMode; }

  // Power Enable / Disable (Active LOW on TMC2209: LOW = energized, HIGH = disabled/freewheel)
  void setMotorEnable(bool enable);
  bool isMotorEnabled() const { return _enabled; }

  // Automated Machine Routines
  // 4-stage precision homing to the top mechanical limit switch
  bool performHoming(float speedMMps, bool (*stopCheck)() = nullptr);
  // Manual "Dip Bot" single test dip cycle (lower to wax -> record wax level -> hold -> return to top limit)
  bool performDipBot(float downSpeedMMps, float upSpeedMMps, int holdTimeSec, float* outWaxLevelMM = nullptr, bool (*stopCheck)() = nullptr);
  // Melts the bottom bases of the candles at targetDepthMM (lastWaxLevelMM - jigLengthMM) for holdTimeSec, then returns to top rest
  bool performMeltBases(float targetDepthMM, float downSpeedMMps, float upSpeedMMps, int holdTimeSec, bool (*stopCheck)() = nullptr);
  // Sinks the entire jig as deep as the capacitive sensor allows for holdTimeSec to melt/clean wax off the jig, then returns to top rest
  bool performCleanJig(float downSpeedMMps, float upSpeedMMps, int holdTimeSec, float* outWaxLevelMM = nullptr, bool (*stopCheck)() = nullptr);

  // Position Tracking & Safety Soft Limits (0.0mm = Home at top, positive = distance downwards in mm)
  float getCurrentPositionMM() const { return _positionMM; }
  void setCurrentPositionMM(float pos) { _positionMM = pos; }
  void setMaxSoftLimitMM(float maxLimit) { _maxSoftLimitMM = maxLimit; }
  float getMaxSoftLimitMM() const { return _maxSoftLimitMM; }

  // Lead screw step conversion constant
  static const float STEPS_PER_MM; // 200.0 steps/mm

private:
  SensorManager* _sensors;        // Pointer to sensor manager for real-time limit/wax checks during motion
  TMCMode _currentMode;           // Currently active TMC2209 chopper mode
  bool _modeInitialized;          // True once initial GCONF mode register has been written
  int _modeSetting;               // User driver mode setting (0=StealthChop, 1=SpreadCycle, 2=Adaptive)
  int _thresholdMMps;             // Speed threshold (mm/s) for Adaptive mode switching
  bool _enabled;                  // True if stepper driver coils are energized
  float _positionMM;              // Tracked Z-axis carriage position in mm from top home (0.0mm)
  float _maxSoftLimitMM;          // Maximum downward travel allowed in mm before software safety stop

  // Internal UART Register Writer (frames 64-bit TMC2209 datagram with CRC8 and sends over Serial2)
  void sendUARTCommand(uint8_t reg, uint32_t val);
};

#endif // MOTOR_MANAGER_H


