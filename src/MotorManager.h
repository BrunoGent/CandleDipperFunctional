#ifndef MOTOR_MANAGER_H
#define MOTOR_MANAGER_H

#include <Arduino.h>
#include "PinConfig.h"
#include "SensorManager.h"

// Drive Modes for TMC2209
enum TMCMode {
  MODE_STEALTHCHOP = 0, // Silent operation for low/medium speed
  MODE_SPREADCYCLE = 1  // High torque / rapid travel
};

class MotorManager {
public:
  MotorManager();

  void begin(SensorManager* sensorMgr);
  
  // Motion Commands
  void stepMotor(bool directionUp, float speedMMps);
  void stepMotorBurst(bool directionUp, float speedMMps, uint32_t burstMs = 20);
  void stopMotor();

  // Mode Configuration
  void setTMCMode(TMCMode mode);
  void applyConfigMode(int modeSetting, float speedMMps, int thresholdMMps);
  TMCMode getTMCMode() const { return _currentMode; }

  // Power Enable / Disable
  void setMotorEnable(bool enable);
  bool isMotorEnabled() const { return _enabled; }

  // Homing & Dipping Routines
  bool performHoming(float speedMMps, bool (*stopCheck)() = nullptr);
  bool performDipBot(float downSpeedMMps, float upSpeedMMps, int holdTimeSec, bool (*stopCheck)() = nullptr);

  // Position & Limit Management
  float getCurrentPositionMM() const { return _positionMM; }
  void setCurrentPositionMM(float pos) { _positionMM = pos; }
  void setMaxSoftLimitMM(float maxLimit) { _maxSoftLimitMM = maxLimit; }
  float getMaxSoftLimitMM() const { return _maxSoftLimitMM; }

  // Steps to MM conversion parameters
  static const float STEPS_PER_MM; // e.g. 200.0 steps/mm (Lead screw T8x8)

private:
  SensorManager* _sensors;
  TMCMode _currentMode;
  bool _enabled;
  float _positionMM;
  float _maxSoftLimitMM;
  unsigned long _lastStepMicros;

  void sendUARTCommand(uint8_t reg, uint32_t val);
};

#endif // MOTOR_MANAGER_H
