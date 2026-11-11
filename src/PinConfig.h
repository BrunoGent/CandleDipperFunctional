#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// =========================================================
// BEE MINE CANDLE DIPPER - HARDWARE PIN DEFINITIONS
// All connections routed via M5Stack Proto Module (M-Bus)
// =========================================================

// --- TMC2209 Stepper Driver ---
// Controls the NEMA 17 stepper motor on the vertical T8x8 lead screw (Z-axis).
#define PIN_MOTOR_STEP        8   // Step pulse line: each HIGH->LOW pulse advances the motor by 1 microstep
#define PIN_MOTOR_DIR         9   // Rotation direction: HIGH = UP (towards top limit), LOW = DOWN (towards wax)
#define PIN_MOTOR_ENABLE     17   // Driver enable (Active LOW): LOW = coils energized, HIGH = coils freewheel/off
#define PIN_MOTOR_UART       18   // Single-Wire UART (via 1k Ohm inline resistor) for configuring current & StealthChop/SpreadCycle

// --- Frame & Arm Sensors ---
// Safety and surface-detection inputs for closed-loop motion control.
#define PIN_TOP_LIMIT_SW      7   // Mechanical home switch (Normally Closed to GND): reads LOW normally, HIGH when hit or wire breaks
#define PIN_WAX_LEVEL_SENS    6   // Capacitive wax sensor (24V NPN via PC817 Optocoupler + 10k pull-up): reads HIGH when dry, LOW when touching wax

// --- HX711 Load Cell Amplifier (Drag Chain) ---
// 24-bit ADC measuring the weight of the candle jig to determine when target candle weight is reached.
#define PIN_HX711_DT          5   // Scale Serial Data Out (DOUT): goes LOW when a new 24-bit conversion is ready
// NOTE: GPIO 13 must NOT be used here. On the CoreS3 / CoreS3 SE it is the I2S audio data line to the
// built-in speaker amplifier, so the speaker driver takes the pin over and the HX711 never sees a clock.
#define PIN_HX711_SCK        10   // Scale Serial Clock (PD_SCK): pulsed 25 times by ESP32 to shift out 24 bits + set Gain 128

#endif // PIN_CONFIG_H

