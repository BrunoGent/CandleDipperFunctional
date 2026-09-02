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

  // Initialization & Hardware Setup
  void begin(SensorManager* sensorMgr);
  
  // Motion Execution Commands
  void stepMotor(bool directionUp, float speedMMps);
  void stepMotorBurst(bool directionUp, float speedMMps, uint32_t burstMs = 20);
  void stopMotor();

  // TMC2209 Dynamic Mode Switching via UART
  void setTMCMode(TMCMode mode);
  void applyConfigMode(int modeSetting, float speedMMps, int thresholdMMps);
  TMCMode getTMCMode() const { return _currentMode; }

  // Power Enable / Disable (Active LOW on TMC2209)
  void setMotorEnable(bool enable);
  bool isMotorEnabled() const { return _enabled; }

  // Automated Machine Routines
  bool performHoming(float speedMMps, bool (*stopCheck)() = nullptr);
  bool performDipBot(float downSpeedMMps, float upSpeedMMps, int holdTimeSec, bool (*stopCheck)() = nullptr);

  // Position Tracking & Safety Soft Limits
  float getCurrentPositionMM() const { return _positionMM; }
  void setCurrentPositionMM(float pos) { _positionMM = pos; }
  void setMaxSoftLimitMM(float maxLimit) { _maxSoftLimitMM = maxLimit; }
  float getMaxSoftLimitMM() const { return _maxSoftLimitMM; }

  // Lead screw step conversion constant
  static const float STEPS_PER_MM; // 200.0 steps/mm

private:
  SensorManager* _sensors;
  TMCMode _currentMode;
  bool _enabled;
  float _positionMM;
  float _maxSoftLimitMM;
  unsigned long _lastStepMicros;

  // Internal UART Register Writer
  void sendUARTCommand(uint8_t reg, uint32_t val);
};

#endif // MOTOR_MANAGER_H

