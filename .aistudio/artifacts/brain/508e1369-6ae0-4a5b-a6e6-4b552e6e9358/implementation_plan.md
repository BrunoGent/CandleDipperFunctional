# Bee Mine Candle Dipper — Antigravity Export & Transition Plan

Deliver a clean, self-contained, and verified PlatformIO C++ firmware package and documentation suite for the Bee Mine Candle Dipper (M5Stack CoreS3 SE) so the project can be seamlessly imported, compiled, and developed within Google Antigravity.

### User Review & Critical Decisions

> [!IMPORTANT]
> The capacitive wax level sensor circuit is confirmed resolved with the new PC817 optocoupler and 10kΩ pull-up on GPIO 6 (Active-LOW sinking logic). Review the decisions below before executing the export package.

- **Confirmed Decision 1 (Optocoupler Status)**: New PC817 optocoupler is installed and verified on GPIO 6 with active-LOW detection (`readRawCapSensor() == LOW` indicates wax contact).
- **Confirmed Decision 2 (Target Environment)**: Project files and build configuration will be structured for Google Antigravity with PlatformIO (`espressif32` / `m5stack-cores3`) and Arduino framework compatibility.
- **Decision 3 (Export Format)**: The web companion will provide a direct 1-click "Download Project ZIP" / "Export All Files" utility alongside the existing in-browser code viewer, allowing immediate drag-and-drop into Antigravity.

---

### 1. Overview & Core Concept

- **What It Does**: Packages the full-stack system—the modular C++ firmware (`DisplayManager`, `MotorManager`, `SensorManager`, `ScaleManager`, `PinConfig.h`, `main.cpp`), the `platformio.ini` environment, hardware schematics, and the React/Tailwind web telemetry companion—into an export-ready bundle tailored for Google Antigravity's agentic workspace.
- **Target Audience / Persona**: Maker / embedded engineer continuing hardware firmware development, automated dipping calibration, and real-time debugging inside Google Antigravity.
- **Key Value**: Eliminates manual copy-pasting, ensures zero missing dependencies or header misconfigurations, and includes complete hardware wiring documentation for the PC817, TMC2209, and HX711.

---

### 2. User Experience & Visual Design

- **Key User Flows**:
  1. **Review & Verify in Web Companion**: The user navigates to the "Firmware & Export" tab of the running web app.
  2. **1-Click Antigravity Export**: The user clicks "Export to Antigravity (ZIP Bundle)" or downloads individual module files.
  3. **Antigravity Workspace Ingestion**: The user opens the exported folder in Antigravity; `platformio.ini` and standard directory layout (`src/`, `include/`) are immediately recognized.
  4. **Build & Flash**: Antigravity agents or the user execute `pio run` to flash the CoreS3 SE without configuration hurdles.

- **Visual Identity & Theme**:
  - *Aesthetic*: Modern dark industrial telemetry UI matching the M5Stack 320x240 LCD display.
  - *Color Palette*: Amber/honey `#F59E0B` and emerald `#10B981` accents over deep obsidian `#0A0A0A` background.
  - *Typography*: Clean monospace font for C++ code headers, terminal pinouts, and hardware register telemetry.

---

### 3. Key Product Decisions & Trade-Offs

- **Standard PlatformIO Directory Structure**:
  - *Chosen Approach*: Standardize on `src/` containing all C++ source and header files with unified `platformio.ini`.
  - *Why*: Antigravity agents parse standard PlatformIO projects natively, automatically identifying board definitions, flash offsets, and `lib_deps` without custom build scripts.
  - *Alternative Considered*: Raw Arduino IDE `.ino` sketch folder. Passed over because multi-file C++ classes with single-wire UART and double-buffered M5Canvas require proper C++17 compilation flags.

- **Client-Side Export Bundle (JSZip)**:
  - *Chosen Approach*: Integrate client-side zip generation in the web companion so the entire project can be exported as a single `.zip` file directly from the browser.
  - *Why*: Instant, zero-latency download that works in any browser without requiring external server storage.

---

### 4. Technical Architecture & Data Strategy *(Technical Reference)*

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       ANTIGRAVITY PROJECT STRUCTURE                         │
├─────────────────────────────────────────────────────────────────────────────┤
│  bee-mine-candle-dipper/                                                    │
│  ├── platformio.ini         (CoreS3 board, espressif32, M5Unified, C++17)   │
│  ├── README.md              (Hardware pinout, PC817 wiring, quickstart)     │
│  ├── src/                                                                   │
│  │   ├── main.cpp           (State machine, dipping cycle, web portal)      │
│  │   ├── PinConfig.h        (GPIO 5-9, 13, 17, 18 assignments)              │
│  │   ├── DisplayManager.h/.cpp (M5Canvas 320x240 double-buffered UI)        │
│  │   ├── MotorManager.h/.cpp   (TMC2209 UART, StealthChop2, 4-stage homing)│
│  │   ├── SensorManager.h/.cpp  (PC817 NPN Active-LOW, NC top switch)       │
│  │   └── ScaleManager.h/.cpp   (HX711 24-bit strain gauge EMA filter)       │
│  └── web-companion/         (React + Tailwind UI viewer and simulator)     │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### Hardware Pin Mapping Reference in Export:
- **GPIO 6**: Wax level capacitive sensor (PC817 optocoupler, active-LOW with 10kΩ pull-up to 3.3V).
- **GPIO 7**: Top carriage limit switch (mechanical microswitch, NC to GND, internal pull-up).
- **GPIO 8, 9, 17**: TMC2209 Step, Dir, and Enable lines.
- **GPIO 18**: TMC2209 Single-Wire UART (1kΩ inline resistor).
- **GPIO 5, 13**: HX711 Load Cell Data (DT) and Clock (SCK).

#### State Machine Flow (Embedded C++):
```
[IDLE / HOME] ──► [LOWERING] ──► [HOLD / DWELL] ──► [RAISING] ──► [COOLING]
      ▲                 │                                │
      └─────────────────┴────── (Top Limit Switch) ──────┘
```
