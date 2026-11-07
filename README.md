# Bee Mine Candle Dipper — M5Stack CoreS3 Firmware

Industrial-grade automated candle dipping machine firmware for the **M5Stack CoreS3 SE** (ESP32-S3), built with object-oriented modular C++ and PlatformIO.

---

## 1. Hardware Architecture & Pin Assignments

All external sensors and actuators are routed through the M5Stack Proto Module using the M-Bus header:

| Peripheral | CoreS3 GPIO | M-Bus Pin | Logic Level | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Wax Level Sensor** | **GPIO 6** | `G6` | 3.3V (Active-LOW) | PC817 Optocoupler from 24V NPN sensor |
| **Top Limit Switch** | **GPIO 7** | `G7` | 3.3V (NC to GND) | Mechanical home switch (Open = HIGH = Hit) |
| **Motor STEP** | **GPIO 8** | `G8` | 3.3V Pulse | TMC2209 Step pulse signal |
| **Motor DIR** | **GPIO 9** | `G9` | 3.3V Level | TMC2209 Direction signal |
| **Motor ENABLE** | **GPIO 17** | `G17` | 3.3V (Active-LOW) | TMC2209 Driver enable line |
| **Motor UART** | **GPIO 18** | `G18` | 3.3V Single-Wire | TMC2209 UART (with 1kΩ inline resistor) |
| **Load Cell DT** | **GPIO 5** | `G5` | 3.3V Serial Data | HX711 24-bit ADC Data line |
| **Load Cell SCK** | **GPIO 13** | `G13` | 3.3V Serial Clock | HX711 24-bit ADC Clock line |

---

## 2. Circuit Wiring Diagrams

### A. Wax Level Capacitive Sensor (Heschen LJC38A4-20-Z/BX) + PC817 Optocoupler
The capacitive sensor is a 12–24V DC **NPN Normally Open (NO)** sensor that switches its Black signal wire to Ground when wax is detected. It is isolated from the 3.3V ESP32 via a PC817 optocoupler:

```
               +24V Power Supply (+)
                     │
                   ┌─┴─┐
                   │   │ 2.2kΩ – 3.3kΩ Resistor (1W)
                   └─┬─┘
                     │
           Pin 1 ┌───┴───┐ Pin 4 ─────► GPIO 6 & 10kΩ Pull-Up to +3.3V
         [ Dot ] │ •     │ PC817
           Pin 2 └───┬───┘ Pin 3 ─────► CoreS3 GND (0V)
                     │
                     └────────► BLACK Wire (Signal) of Heschen NPN Sensor

   BROWN Wire of Sensor ──────► +24V Power Supply (+)
   BLUE  Wire of Sensor ──────► 24V Power Supply Ground (-)
```

- **Idle (No Wax):** Black wire is floating $\rightarrow$ PC817 LED is OFF $\rightarrow$ 10kΩ resistor pulls **GPIO 6 to 3.3V (`HIGH`)**.
- **Active (Wax Contact):** Sensor pulls Black wire to 0V $\rightarrow$ PC817 LED is ON $\rightarrow$ Phototransistor conducts $\rightarrow$ **GPIO 6 drops to 0.0V (`LOW`)**.

---

### B. Trinamic TMC2209 Stepper Driver (Z-Axis Lead Screw)
The carriage is driven by a NEMA 17 stepper motor on a T8x8 lead screw (pitch: 8mm/rev, 200 steps/mm at 1/8 microstepping):

```
       M5Stack CoreS3                      TMC2209 SilentStepStick
   ┌────────────────────┐                   ┌────────────────────┐
   │ GPIO 8 (STEP)      ├──────────────────►│ STEP               │
   │ GPIO 9 (DIR)       ├──────────────────►│ DIR                │
   │ GPIO 17 (ENABLE)   ├──────────────────►│ EN (Active LOW)    │
   │ GPIO 18 (UART)     ├───[ 1kΩ Inline ]──┤ PDN / UART         │
   │ +3.3V              ├──────────────────►│ VIO                │
   │ GND                ├──────────────────►│ GND                │
   └────────────────────┘                   │ VMOT ◄── +24V PSU  │
                                            │ GND  ◄── 24V GND   │
                                            └────────────────────┘
```

---

### C. Top Limit Switch (NC Fail-Safe)
The carriage home switch uses a microswitch wired in **Normally Closed (NC)** configuration:
- One terminal connected to **CoreS3 GND**.
- Other terminal connected to **GPIO 7**.
- Firmware activates internal pull-up (`INPUT_PULLUP`).
- **Normal Travel:** Switch is closed to GND $\rightarrow$ **0V (`LOW`)**.
- **Home Reached or Wire Break:** Switch opens $\rightarrow$ **3.3V (`HIGH`)**.

---

## 3. Firmware Architecture

```
┌────────────────────────────────────────────────────────────────────────┐
│                               main.cpp                                 │
│         (Non-blocking loop, state machine & Web Captive Portal)        │
├─────────────────┬─────────────────┬────────────────┬───────────────────┤
│  MotorManager   │  SensorManager  │  ScaleManager  │  DisplayManager   │
│  TMC2209 UART   │ Glitch-Filtered │ HX711 24-bit   │ Double-buffered   │
│  4-Stage Homing │ Limit & Wax     │ Load Cell EMA  │ M5Canvas 320x240  │
└─────────────────┴─────────────────┴────────────────┴───────────────────┘
```

### Modular Components
1. **`MotorManager`**:
   - Single-wire UART driver configuration via `Serial2`.
   - Real-time switching between **StealthChop2** (silent dipping) and **SpreadCycle** (high-torque rapid travel).
   - 4-stage homing calibration (Fast seek $\rightarrow$ Backoff $\rightarrow$ Slow creep $\rightarrow$ 0.0mm reference + 2mm stress relief).
2. **`SensorManager`**:
   - Multi-sample hardware glitch filtering (3 consecutive reads with $2\mu\text{s}$ spacing).
   - Active-LOW detection for PC817 optocoupler wax level detection.
3. **`ScaleManager`**:
   - Non-blocking 10Hz bitbang reading for the HX711 ADC.
   - Exponential Moving Average (EMA) vibration filtering.
   - Persistent tare offset and scale factor in ESP32 `Preferences` NVS.
4. **`DisplayManager`**:
   - High-performance double-buffered graphics pipeline rendering to `M5Canvas`.
   - Touch hit-testing with non-blocking decoupled `UiEvent` queue.
   - Multiple color palettes (Bee Mine Gold, Classic, High-Contrast Dark).

---

## 4. Building & Flashing with PlatformIO

### Prerequisites
- [PlatformIO Core (CLI)](https://docs.platformio.org/en/latest/core/index.html) or PlatformIO IDE extension for VS Code / Antigravity.

### Project Setup
The included `platformio.ini` is pre-configured for the M5Stack CoreS3:

```ini
[platformio]
src_dir = src
include_dir = src

[env:m5stack-cores3]
platform = espressif32
board = m5stack-cores3
framework = arduino
monitor_speed = 115200

lib_deps =
    m5stack/M5Unified@^0.1.16

build_flags =
    -std=gnu++17
```

### Commands
```bash
# 1. Compile the firmware
pio run

# 2. Flash to M5Stack CoreS3 SE via USB-C
pio run -t upload

# 3. Open Serial Telemetry Monitor
pio device monitor -b 115200
```
