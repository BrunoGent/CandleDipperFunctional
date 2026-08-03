#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// =========================================================
// BEE MINE CANDLE DIPPER - HARDWARE PIN DEFINITIONS
// All connections routed via M5Stack Proto Module (M-Bus)
// =========================================================

// --- TMC2209 Stepper Driver ---
#define PIN_MOTOR_STEP        8   // Hardware PWM pulse line
#define PIN_MOTOR_DIR         9   // Motor rotation direction
#define PIN_MOTOR_ENABLE     17   // Active LOW enable line
#define PIN_MOTOR_UART       18   // Single-Wire UART (1k Ohm inline resistor)

// --- Frame & Arm Sensors ---
#define PIN_TOP_LIMIT_SW      7   // Mechanical home switch (NC config)
#define PIN_WAX_LEVEL_SENS    6   // Arm Capacitive sensor (via 24V -> 3.3V Optocoupler)

// --- HX711 Load Cell Amplifier (Drag Chain) ---
#define PIN_HX711_DT          5   // Scale Serial Data line
#define PIN_HX711_SCK        13   // Scale Serial Clock line

#endif // PIN_CONFIG_H
