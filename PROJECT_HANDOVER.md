# Bee Mine Candle Dipper — Project Handover & Engineering Specification

**Document Version:** 1.0.0  
**Target Platform:** M5Stack CoreS3 SE (ESP32-S3 Dual-Core LX7, 8MB PSRAM, 16MB Flash)  
**Framework:** Arduino / ESP-IDF via PlatformIO (`-std=gnu++17`)  
**Project Objective:** Automated precision candle dipping machine with closed-loop carriage motion, liquid wax surface detection, real-time load cell monitoring, and double-buffered touch UI.

---

## 1. System & Hardware Specifications

### 1.1 Host Controller Architecture
* **Microcontroller:** Espressif ESP32-S3 (Dual Xtensa 32-bit LX7 @ 240 MHz).
* **Display:** 2.0-inch IPS LCD panel (ILI9342C, 320x240 RGB565) driven via SPI bus.
* **Touch Controller:** FocalTech FT6336U capacitive touch sensor on $I^2C$ bus.
* **Expansion Interface:** M5Stack 30-pin M-Bus header connected to an internal Proto Module for clean external wiring breakout.

### 1.2 Pin Mapping Table (M-Bus & GPIO Allocations)

All hardware signals are consolidated in `src/PinConfig.h`:

| Function | Pin Name in Code | ESP32-S3 GPIO | M-Bus Pin | Electrical Level / Interface | Circuit Details |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Wax Level Proximity** | `PIN_WAX_LEVEL_SENS` | **GPIO 6** | `G6` | 3.3V CMOS (Active-LOW) | PC817 Optocoupler with 10kΩ pull-up to +3.3V |
| **Top Carriage Limit** | `PIN_TOP_LIMIT_SW` | **GPIO 7** | `G7` | 3.3V CMOS (Active-HIGH) | Microswitch, Normally Closed (NC) to GND |
| **Stepper STEP** | `PIN_MOTOR_STEP` | **GPIO 8** | `G8` | 3.3V Logic Pulse | TMC2209 Step pin (hardware timer pulse) |
| **Stepper DIR** | `PIN_MOTOR_DIR` | **GPIO 9** | `G9` | 3.3V Logic Level | HIGH = UP (towards limit), LOW = DOWN |
| **Stepper ENABLE** | `PIN_MOTOR_ENABLE` | **GPIO 17** | `G17` | 3.3V Active-LOW | LOW = Energized coils, HIGH = Driver sleep |
| **Stepper UART** | `PIN_MOTOR_UART` | **GPIO 18** | `G18` | 3.3V Single-Wire Half-Duplex | `Serial2` @ 115200 baud with 1kΩ inline resistor |
| **Load Cell DT (Data)** | `PIN_HX711_DT` | **GPIO 5** | `G5` | 3.3V Serial Bit-Bang | HX711 24-bit ADC Data Out |
| **Load Cell SCK (Clock)** | `PIN_HX711_SCK` | **GPIO 13** | `G13` | 3.3V Serial Clock | Clock pulse train for 24-bit readout + Gain 128 |

---

### 1.3 Detailed Circuit Diagrams & Electrical Schematics

#### A. Wax Level Sensor & Optocoupler Circuit
* **Sensor Model:** Heschen `LJC38A4-20-Z/BX` (12–24V DC, 300mA, NPN Normally Open).
* **Isolation IC:** PC817 Optocoupler (4-pin DIP).
* **Behavior:** When the sensor detects molten wax, its internal open-collector NPN transistor conducts to GND (Blue wire), energizing the PC817 LED and pulling GPIO 6 to 0V.

```
                              +24V Power Supply (+)
                                       │
                                     ┌─┴─┐
                                     │   │  2.2kΩ – 3.3kΩ Resistor (1W)
                                     │   │  (Limits LED current to ~8-10mA)
                                     └─┬─┘
                                       │
              PC817 Optocoupler        │
             ┌─────────────────┐       │
   Pin 1 (A) │ [Dot] •       4 │───────┴─── Pin 4 (Collector)
             │   LED     Photot│           │
   Pin 2 (K) │                 3 │─────┐     ├────────► GPIO 6 (PIN_WAX_LEVEL_SENS)
             └─────────────────┘     │     │
               │                     │    ┌┴┐
               │                     │    │ │ 10kΩ Pull-Up Resistor
               │                     │    └┬┘
               │                     │     │
               │                     │    +3.3V (M5Stack Power Rail)
               │                     │
               │                     └────────► GND (M5Stack Ground Rail)
               │
               └──────────────────────────────► BLACK Wire (NPN Open-Collector Signal)
                                                ┌───────────────────────────────────┐
   BROWN Wire (+24V DC Power) ──────────────────┤ Heschen LJC38A4-20-Z/BX           │
   BLUE Wire  (24V DC Ground) ──────────────────┤ Capacitive Proximity Sensor       │
                                                └───────────────────────────────────┘
```

#### Truth Table:
| Machine Condition | Heschen Sensor State | PC817 LED (Pins 1–2) | Voltage at GPIO 6 (to GND) | ESP32 Logic | Action |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **No Wax (Idle / Lowering)** | Open Circuit / High-Z | 0.0V (Off) | **3.3V – 3.4V** | `HIGH` | Carriage continues lowering |
| **Wax Contact (Surface Hit)** | Closed to 24V GND | ~1.1V – 1.2V (On) | **0.05V – 0.15V** | `LOW` | **Stop lowering immediately & dwell** |

---

#### B. TMC2209 Stepper Driver (Z-Axis Lead Screw)
* **Lead Screw:** T8x8 (8mm lead per revolution).
* **Motor:** NEMA 17 (1.8° step angle = 200 full steps/rev).
* **Microstepping:** 1/8 microstep interpolation $\rightarrow$ **200 steps/mm**.
* **Communication:** Single-wire UART over GPIO 18 using an inline 1kΩ resistor to protect the ESP32 transmitter buffer when the TMC2209 drives the line during register read replies.

```
      M5Stack CoreS3 SE                            TMC2209 SilentStepStick
  ┌────────────────────────┐                      ┌────────────────────────┐
  │ GPIO 8 (STEP)          ├─────────────────────►│ STEP                   │
  │ GPIO 9 (DIR)           ├─────────────────────►│ DIR                    │
  │ GPIO 17 (ENABLE)       ├─────────────────────►│ EN (Active LOW)        │
  │ GPIO 18 (UART Tx/Rx)   ├───[ 1kΩ Resistor ]──►│ PDN_UART               │
  │ +3.3V Logic Supply     ├─────────────────────►│ VIO                    │
  │ GND Logic Return       ├─────────────────────►│ GND                    │
  └────────────────────────┘                      │ VMOT ◄─── +24V Supply  │
                                                  │ GND  ◄─── 24V Ground   │
                                                  └────────────────────────┘
```

---

#### C. Top Carriage Limit Switch (Fail-Safe NC)
* **Configuration:** Normally Closed (NC).
* **Safety Feature:** A wire break, loose crimp, or unplugged connector opens the circuit to 3.3V, registering as a limit hit and preventing carriage over-travel.
* One microswitch terminal $\rightarrow$ **CoreS3 GND**.
* Other terminal $\rightarrow$ **GPIO 7** (Software configured as `pinMode(PIN_TOP_LIMIT_SW, INPUT_PULLUP)`).

---

### 1.4 Display & UI Pipeline Implementation
* **Double-Buffered Graphics Pipeline:** An internal `M5Canvas` instance (320x240 @ 16-bit color depth) is allocated inside `DisplayManager`. All text, gauges, buttons, and telemetry are rendered off-screen into RAM, then pushed to the physical LCD in a single DMA transfer (`_canvas.pushSprite(0, 0)`), completely eliminating flicker.
* **Decoupled Touch Hit-Testing:** `DisplayManager::updateTouch()` queries the FT6336U touch controller, checks bounding boxes for on-screen buttons, and returns decoupled `UiEvent` enums (e.g., `EVENT_START_DIP`, `EVENT_STOP_DIP`, `EVENT_TARE_SCALE`). This keeps `main.cpp` non-blocking.
* **Multi-Theme Engine:** 
  1. *Bee Mine Gold:* Honey gold `#F59E0B` headers, charcoal containers, amber active states.
  2. *Classic Light:* High-contrast industrial display mode for bright workshop environments.
  3. *Obsidian Dark:* Low-power dark mode with neon emerald accents.

---

## 2. Completed Architecture & Module Breakdown

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                                  main.cpp                                   │
│      Non-blocking loop, automated dipping state machine & Wi-Fi portal      │
└──────────────┬──────────────────┬─────────────────┬─────────────────────────┘
               │                  │                 │
       ┌───────▼────────┐  ┌──────▼───────┐  ┌──────▼────────┐
       │  MotorManager  │  │ ScaleManager │  │ SensorManager │
       │  TMC2209 UART  │  │ HX711 24-bit │  │ Glitch-Filter │
       │  4-Stage Home  │  │ EMA Filter   │  │ Limit & Wax   │
       └────────────────┘  └──────────────┘  └───────────────┘
                                   │
                           ┌───────▼────────┐
                           │ DisplayManager │
                           │ M5Canvas 320x  │
                           │ 240 UI Engine  │
                           └────────────────┘
```

### Module Status Summary

| File | Purpose | Verification Status |
| :--- | :--- | :--- |
| `src/PinConfig.h` | Master GPIO definitions and hardware register constants | ✅ Verified |
| `src/SensorManager.h/.cpp` | 3-sample hardware debouncer for limit switch & PC817 optocoupler | ✅ Verified (Active-LOW default set) |
| `src/MotorManager.h/.cpp` | TMC2209 UART driver, StealthChop2 switching, 4-stage homing & dipping | ✅ Verified |
| `src/ScaleManager.h/.cpp` | Non-blocking 10Hz HX711 bit-bang reader, EMA vibration filter, NVS tare | ✅ Verified |
| `src/DisplayManager.h/.cpp`| Double-buffered M5Canvas UI engine, touch hit-testing & themes | ✅ Verified |
| `src/main.cpp` | State machine router, dipping cycle coordinator, and captive portal | ✅ Verified |
| `platformio.ini` | PlatformIO configuration for CoreS3 SE (`M5Unified`, C++17) | ✅ Verified |
| `src/App.tsx` | Web companion, CoreS3 simulator, and 1-click Antigravity ZIP exporter | ✅ Verified & Compiling |

---

## 3. Known Issues, Blockers & Unfixed Errors

### 3.1 Resolved During Prior Sessions
1. **4N35 Optocoupler Incomplete Saturation (1.03V Drop):**
   * *Cause:* The older 4N35 had a low Current Transfer Ratio (CTR ~40%). An under-driven 24V LED side prevented the phototransistor from sinking through a 10kΩ pull-up, holding GPIO 6 at 1.03V. Because ESP32-S3 $V_{IL} \le 0.825\text{V}$, the CPU could not register a digital `LOW`.
   * *Resolution:* Replaced with a high-CTR **PC817 optocoupler** (CTR 100%–300%) with verified 10kΩ pull-up on GPIO 6 and $2.7\text{k}\Omega$ on the 24V LED side.
2. **Reverse Diode Burnout:**
   * *Cause:* A previously tested optocoupler had its Pins 1 and 2 open-circuit due to exceeding maximum reverse voltage ($V_R > 6\text{V}$) during bench testing.
   * *Resolution:* Proper polarity confirmed with multimeter diode test; series resistor prevents overcurrent.
3. **Internal Pull-Up Contention:**
   * *Cause:* `pinMode(PIN_WAX_LEVEL_SENS, INPUT_PULLUP)` placed the internal 45kΩ pull-up in parallel with external 10kΩ resistors.
   * *Resolution:* Changed to `pinMode(PIN_WAX_LEVEL_SENS, INPUT)` (pure high-impedance).

### 3.2 Items Requiring Physical Bench Validation in Antigravity
1. **PC817 Voltage Verification Under Load:**
   * Before running the motor, confirm with a multimeter that GPIO 6 reads **3.3V at idle** and drops cleanly to **$\le 0.15\text{V}$** when touching the capacitive sensor face.
2. **Top Limit Backoff Distance Tuning:**
   * In `MotorManager::performHoming()`, Stage 4 applies a 2.0 mm downward back-off to release physical switch pressure. If your physical switch has a long actuator travel, verify 2.0 mm fully opens the contacts without leaving the carriage loose.
3. **HX711 Bit-Bang Interrupt Window:**
   * `ScaleManager::readRawBitbang()` disables interrupts (`noInterrupts()`) for ~100 microseconds during 24-bit clocking. If Wi-Fi web requests occur simultaneously, verify that packet throughput is smooth. If jitter occurs, move scale reads to FreeRTOS Core 1.
4. **TMC2209 Current Threshold:**
   * Default run current is set to 800mA RMS (`IRUN = 20`, `IHOLD = 10`). Check motor heat after continuous dipping; adjust down to 600mA if the motor gets warm.

---

## 4. Next Implementation Steps (Roadmap for Antigravity)

### Step 1: Import into Google Antigravity
1. Download the complete ZIP via the web companion (`bee-mine-candle-dipper-antigravity.zip`) or clone the repository directly into Antigravity.
2. Verify that `platformio.ini` is detected by the PlatformIO / Antigravity workspace extension.

### Step 2: Bench Multimeter Sanity Check
Before applying motor power:
```bash
# 1. Compile firmware in Antigravity terminal
pio run

# 2. Flash CoreS3 via USB-C
pio run -t upload

# 3. Open Serial Telemetry
pio device monitor -b 115200
```
- Tap the capacitive switch: ensure serial monitor logs `Wax Sensor: TRIGGERED`.
- Click the top limit switch: ensure serial monitor logs `Top Limit: HIT`.

### Step 3: Homing Calibration Test
- Place the carriage in the middle of travel.
- Trigger **Home** from the CoreS3 screen or serial terminal.
- Verify:
  - Fast travel upwards.
  - Limit hit detection.
  - Gentle backoff.
  - Slow creep.
  - Zero reference set at `0.0 mm` with 2.0 mm relief clearance.

### Step 4: First Automated Dipping Cycle
- Verify parameters on CoreS3 display (Down speed: 30 mm/s, Dwell: 3s, Up speed: 15 mm/s, Cool: 10s).
- Initiate test dip: verify carriage lowers, stops instantly on wax touch, dwells, raises to top limit, and cools.
