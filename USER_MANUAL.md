# Bee Mine — Automated Candle Dipper User & Operations Manual

**Hardware Platform:** M5Stack CoreS3 SE (ESP32-S3) · DM542T Stepper Driver · HX711 Load Cell · Capacitive Wax Level Sensor  
**Standard Dipping Basket:** Kerzenkiste 36-Candle Stainless Steel Dipping Jig (*Tauchkorb für 36 Kerzen – Groß*, Art.Nr. 8517) with Adapted Load-Cell Mounting Handle  
**Firmware Version:** v2.5 (2-Bucket Recipe/Machine Setup · 3-Card Vertical Scroll UI · Two-Stage Speeds · Tapered Cooling · Drip-Off · Base Melt & Jig Clean)

---

## Executive Summary

The **Bee Mine Automated Candle Dipper** is a precision electromechanical dipping system engineered to automate artisanal beeswax and taper candle production using the **Kerzenkiste 36-Candle Stainless Steel Dipping Basket (*Tauchkorb für 36 Kerzen – Groß*)**.

Instead of relying on fixed mechanical bottom stops that shift as molten wax is consumed from the vat, the system combines three real-time feedback loops:
1. **Capacitive Surface Sensing (`G13`)**: Detects the exact top surface of the molten wax (`Last Wax Level`) on every immersion cycle, automatically compensating as the wax level drops throughout a production batch.
2. **Real-Time Gravimetric Weighing (`HX711 Load Cell`)**: Automatically tares (zeroes out) the physical weight of the stainless-steel dipping jig and wicks at the start of every batch (`Auto-Tare`), tracks pure accumulated wax weight in grams, and continuously monitors for downward mechanical collisions (`Collision Limit`) if the candle tips hit solid or unmelted wax.
3. **Multi-Stage Motion & Thermal Profiling (`DM542T Stepper`)**: Executes fast Z-axis air travel, decelerates smoothly `15 mm` before the candle tips enter the molten wax to prevent splashing (`Immersion Speed`), lifts smoothly at a dedicated `Extraction Speed` for uniform coat thickness, pauses near the surface to shed the hanging wax droplet (`Drip-Off`), linearly scales cooling times from thin core layers to thick outer layers (`Tapered Cooling Curve`), and optionally levels candle bottoms at the end of the run (`Auto-Melt Bases`).

---

## Table of Contents / Index

1. [Executive Summary](#executive-summary)
2. [Section 1 — System Hardware & Kerzenkiste 36-Candle Jig Specification](#section-1--system-hardware--kerzenkiste-36-candle-jig-specification)
   - 1.1 Kerzenkiste 36-Candle Dipping Basket (*Tauchkorb für 36 Kerzen – Groß*)
   - 1.2 Z-Axis Coordinate System & Sensor Geometry
   - 1.3 Pinout & Electrical Connections (`M5Stack CoreS3 SE`)
3. [Section 2 — Navigation & Touchscreen Architecture](#section-2--navigation--touchscreen-architecture)
   - 2.1 Persistent 4-Tab Bottom Navigation Bar
   - 2.2 2-Bucket Setup Menu & 3-Card Vertical Scroll Layout
   - 2.3 Parameter Terminology (Setpoints vs. Recipe Parameters vs. Calibration Constants)
4. [Section 3 — Quick-Reference Speed & Parameter Glossary](#section-3--quick-reference-speed--parameter-glossary)
   - 3.1 Master Speed Glossary (At-a-Glance)
   - 3.2 Complete Parameter Quick-Reference Table (All 24 Settings)
5. [Section 4 — Detailed Parameter & Configuration Reference](#section-4--detailed-parameter--configuration-reference)
   - 4.1 Bucket 1: `RECIPE PARAMS` (Production Process Settings)
   - 4.2 Bucket 2: `MACHINE SETUP` (Hardware & Calibration Constants)
6. [Section 5 — Step-by-Step Operating Procedures](#section-5--step-by-step-operating-procedures)
   - 5.1 Pre-Start & Z-Axis Homing (`MANUAL -> HOME`)
   - 5.2 Loading the 36-Candle Jig & Running a Dipping Batch (`AUTO`)
   - 5.3 Pause / Resume & Emergency Stop Behavior
   - 5.4 Finishing & Maintenance Cycles (`TOOLS`)
7. [Section 6 — Load Cell Calibration & Safety Interlocks](#section-6--load-cell-calibration--safety-interlocks)
   - 6.1 2-Step Load Cell Calibration Wizard (`TARE (0g)` + `CAL WT`)
   - 6.2 Collision Detection & Auto-Retract Recovery
   - 6.3 Low-Wax Level Cutoff (`Wax Cutoff`)

---

## Section 1 — System Hardware & Kerzenkiste 36-Candle Jig Specification

### 1.1 Kerzenkiste 36-Candle Dipping Basket (*Tauchkorb für 36 Kerzen – Groß*)
The machine is specifically dimensioned around the **Kerzenkiste 36-Candle Stainless Steel Dipping Basket** equipped with an adapted top mounting handle that couples directly to the load cell on the Z-axis carriage:

| Specification | Value / Rating | Operational Notes |
| :--- | :--- | :--- |
| **Manufacturer / Model** | Kerzenkiste — *Tauchkorb für 36 Kerzen (Groß)* | Item No. `8517` (`GTIN 4029225085179`) |
| **Material** | Rustproof Stainless Steel (*Rostfreier Edelstahl*) | Resistant to hot beeswax, paraffin, and stearin |
| **Candle Capacity** | **36 individual candles** per batch (18 wick pairs) | Tensioned via lower hook frame for straight tapers |
| **Max Candle Length** | Up to **500 mm (`50 cm`)** | Dependent on vat depth and `Jig Length` setting |
| **Max Candle Diameter** | Up to **24 mm** (at full 36-candle occupancy) | Thicker candles possible with half-occupancy spacing |
| **Basket Dimensions** | **220 × 220 × 620 mm** (`22 × 22 × 62 cm`) | Square footprint with adjustable bottom tension rack |
| **Empty Basket Weight** | **~720 g (`0.72 kg`)** + Adapted Mounting Handle | Automatically zeroed out (`Auto-Tare`) at batch start |
| **Typical Net Wax Target** | **1,500 g** (*Slim*) to **2,500 g** (*Standard*) | `1,500 g ÷ 36 = ~41.7 g/candle` · `2,500 g ÷ 36 = ~69.4 g/candle` |

### 1.2 Z-Axis Coordinate System & Sensor Geometry
All vertical positions are measured in **millimeters (`mm`) downward from the Top Optical Homing Switch (`0.0 mm`)**:

```
Z = 0.0 mm  ───────────────────────────────────────────────  [Top Optical Home Switch (G7)]
                 │  ▲
                 │  │  Fast Air Travel (Up Speed / Down Speed)
                 │  ▼
                 │     ┌─────────────────────────┐
                 │     │  Adapted Mount Handle   │
                 │     ├─────────────────────────┤  <-- Capacitive Sensor (G13) mounted on frame/carriage
                 │     │                         │      triggers at Z = Last Wax Level (mm)
                 │     │  Kerzenkiste 36-Candle  │      |
                 │     │  Stainless Dipping Jig  │      |  Jig Length (mm): Vertical distance from
                 │     │   (Up to 500mm Tapers)  │      |  Capacitive Sensor down to Candle Bases
                 ▼     │                         │      ▼
Z = Base Touch - 15mm ─┼─────────────────────────┼─  [Deceleration Point: 15 mm above molten wax]
                       │   ||   ||   ||   ||     │   Switches from fast Down Speed -> gentle Immerse Speed
Z = Base Touch (mm) ───┴───▼▼───▼▼───▼▼───▼▼─────┴─  [Candle Tips Touch Wax Surface]
                                                     Base Touch = Last Wax Level - Jig Length
                       ~~~~~~~~~~~~~~~~~~~~~~~~~~~
Z = Last Wax Level ────~~~~~~~~~~~~~~~~~~~~~~~~~~~─  [Molten Wax Surface (Full Dip Depth)]
                       ~~~~~~~~~~~~~~~~~~~~~~~~~~~
Z = Soft Limit (mm) ───────────────────────────────  [Maximum Physical Z-Axis Bottom Travel]
```

* **`Last Wax Level (mm)`**: Recorded automatically every time the capacitive sensor (`G13`) detects the molten wax surface during a dip.
* **`Jig Length (mm)`**: The fixed physical distance from the capacitive sensor trigger point down to the bottom tips of the candles on the Kerzenkiste basket.
* **`Base Touch Position (mm)`**: Calculated automatically by the controller as:
  $$\text{Base Touch Position} = \text{Last Wax Level} - \text{Jig Length}$$

### 1.3 Pinout & Electrical Connections (`M5Stack CoreS3 SE`)

| Subsystem | M5Stack CoreS3 SE Pin | Connector | Direction | Function |
| :--- | :--- | :--- | :--- | :--- |
| **Stepper `PUL+`** | `GPIO 6` | Internal Header | Output | Step pulse train (`200 steps/mm` calibration) |
| **Stepper `DIR+`** | `GPIO 5` | Internal Header | Output | Direction (`HIGH` = Down, `LOW` = Up) |
| **Stepper `ENA+`** | `GPIO 10` | Internal Header | Output | Driver Enable (`LOW` = Energized, `HIGH` = Free) |
| **Optical Home Switch** | `GPIO 7` | Internal Header | Input (`PULLUP`) | Top `0.0 mm` reference (`HIGH` when interrupted) |
| **Capacitive Wax Sensor** | `GPIO 13` | Internal Header | Input (`PULLUP`) | Molten wax surface detector |
| **HX711 Load Cell `DOUT`**| `GPIO 2` | Port A (Red) | Input | 24-bit ADC serial data line |
| **HX711 Load Cell `SCK`** | `GPIO 1` | Port A (Red) | Output | 24-bit ADC serial clock line |

---

## Section 2 — Navigation & Touchscreen Architecture

### 2.1 Persistent 4-Tab Bottom Navigation Bar
The bottom `40px` (`y = 200..240`) of the `320×240` touchscreen houses a permanent 4-tab navigation bar:
1. **`AUTO`**: Main production screen. Select Mode (`Weight` vs. `Dips`), Profile (`Slim` vs. `Standard`), toggle `Melt Bases`, and press `START`, `PAUSE`, or `STOP`.
2. **`MANUAL`**: Hardware diagnostics, live sensor readouts (`Top Limit`, `Wax Sensor`, `Z-Position`), hold-to-jog `UP`/`DOWN` buttons, `HOME`, and `TEST DIP`.
3. **`TOOLS`**: Dedicated finishing and maintenance utilities (`MELT CANDLE BASES` and `CLEAN WHOLE JIG`).
4. **`SETUP`**: Organized into **2 Top-Level Configuration Buckets** (`1. RECIPE PARAMS` and `2. MACHINE SETUP`).

### 2.2 2-Bucket Setup Menu & 3-Card Vertical Scroll Layout
To keep every button large, workshop-friendly, and readable while supporting 24 configuration items:
* Pressing **`SETUP`** presents **2 Top-Level Buckets**:
  1. **`1. RECIPE PARAMS >`**: Settings you adjust when changing candle styles, wax temperature, or batch targets.
  2. **`2. MACHINE SETUP >`**: Hardware limits, jig geometry, load-cell calibration, motor speeds, screen themes, and Wi-Fi.
* Inside every list screen, the display renders **3 full-width vertical cards (`50px` tall each)** showing **both the Parameter Name + Current Value and a 1-line Subtitle Hint** explaining what the setting does.
* When a list has more than 3 items, a **`45px`-wide Scroll Column (`▲` / `Page 1/2` / `▼`)** appears on the right edge (`x = 269..314`).

### 2.3 Parameter Terminology
In industrial automation, your configurable values fall into three categories:
* **Setpoints (Target Values)**: The goal for the current batch (e.g., `Slim Weight = 1500g`, `Std Dips = 25`).
* **Recipe / Process Parameters**: Motion speeds, dwell times, and cooling intervals that govern wax quality (e.g., `Immerse Speed`, `Extract Speed`, `Start Cool`, `End Cool`).
* **Machine / Calibration Constants**: Physical measurements of your hardware that only change if you modify the frame or swap the basket (e.g., `Jig Length`, `Soft Limit`, `Scale Factor`).

---

## Section 3 — Quick-Reference Speed & Parameter Glossary

### 3.1 Master Speed Glossary (At-a-Glance)
Use this quick-reference table to verify which speed parameter governs each phase of carriage movement:

| Speed Parameter | Menu Location | Default | Range | Active Zone & Purpose |
| :--- | :--- | :---: | :---: | :--- |
| **`Down Speed`** | `RECIPE -> 3. Travel & Wax Speeds` | **`50%`** | `1–100%` | **Fast Air Descent:** Used from Top Home (`0.0 mm`) down until the candle tips are `15 mm` above the molten wax surface (`Base Touch - 15 mm`). Saves cycle time while moving through air. |
| **`Immerse Speed`** | `RECIPE -> 3. Travel & Wax Speeds` | **`25%`** | `1–100%` | **Gentle Wax Entry:** Automatically engages `15 mm` before the candle tips touch the wax and continues down to full depth (`Last Wax Level`). Prevents wax splashing, air bubbles, and wick sway on the 36-candle basket. |
| **`Extract Speed`** | `RECIPE -> 3. Travel & Wax Speeds` | **`20%`** | `1–100%` | **Smooth Wax Withdrawal:** Used when lifting the candles out of the molten wax from full depth (`Last Wax Level`) until the candle tips clear the surface (`Base Touch`). Slower extraction produces smoother, more uniform candle layers without ripples. |
| **`Up Speed`** | `RECIPE -> 3. Travel & Wax Speeds` | **`60%`** | `1–100%` | **Fast Air Return:** Engages automatically once the candle tips clear the molten wax surface (`Base Touch`) and carries the jig up to Top Home (`0.0 mm`). Also used for emergency collision retraction. |
| **`Manual Speed`** | `MACHINE -> 3. Stepper Motor` | **`50%`** | `1–100%` | **Manual Jog & Homing Speed:** Governs the `HOLD UP` / `HOLD DOWN` buttons on the `MANUAL` tab and the initial high-speed approach during `HOME`. |

---

### 3.2 Complete Parameter Quick-Reference Table (All 24 Settings)

| ID | Parameter Name | Bucket & Category | Default | Unit | One-Line Summary |
| :-: | :--- | :--- | :---: | :---: | :--- |
| `0` | **`Slim Weight`** | `RECIPE -> 1. Target Weights` | `1500` | `g` | Total net wax weight target for Slim profile (36 candles) |
| `1` | **`Std Weight`** | `RECIPE -> 1. Target Weights` | `2500` | `g` | Total net wax weight target for Standard profile (36 candles) |
| `2` | **`Slim Dips`** | `RECIPE -> 2. Target Dip Counts` | `15` | `dips` | Total number of dip cycles for Slim profile in Dips mode |
| `3` | **`Std Dips`** | `RECIPE -> 2. Target Dip Counts` | `25` | `dips` | Total number of dip cycles for Standard profile in Dips mode |
| `6` | **`Up Speed`** | `RECIPE -> 3. Travel & Wax Speeds` | `60` | `%` | Fast upward travel speed in air (once tips clear wax) |
| `7` | **`Down Speed`** | `RECIPE -> 3. Travel & Wax Speeds` | `50` | `%` | Fast downward travel speed in air (until 15mm above wax) |
| `19`| **`Immerse Speed`** | `RECIPE -> 3. Travel & Wax Speeds` | `25` | `%` | Gentle entry speed starting 15mm before wax surface |
| `20`| **`Extract Speed`** | `RECIPE -> 3. Travel & Wax Speeds` | `20` | `%` | Smooth withdrawal speed while lifting candles out of wax |
| `4` | **`Dip 1 Time`** | `RECIPE -> 4. Dip & Drip Timing` | `10` | `sec` | Bottom soak duration on Dip #1 to saturate dry cotton wicks |
| `5` | **`Sub Dip Time`** | `RECIPE -> 4. Dip & Drip Timing` | `4` | `sec` | Bottom immersion duration on Dip #2 and all subsequent dips |
| `21`| **`Drip Dist`** | `RECIPE -> 4. Dip & Drip Timing` | `15` | `mm` | Pause offset near wax (`+` above surface, `-` tips submerged, `0` off) |
| `22`| **`Drip Time`** | `RECIPE -> 4. Dip & Drip Timing` | `5` | `sec` | Pause duration at `Drip Dist` before returning to Top Home |
| `10`| **`Rest Time`** | `RECIPE -> 5. Cooling & Finishing` | `25` | `sec` | Default top cooling pause when `Start Cool` / `End Cool` are `0` |
| `17`| **`Start Cool`** | `RECIPE -> 5. Cooling & Finishing` | `0` | `sec` | Cooling pause after Dip #1 (`0` = use `Rest Time`) |
| `18`| **`End Cool`** | `RECIPE -> 5. Cooling & Finishing` | `0` | `sec` | Cooling pause on final dip (`0` = use `Rest Time`; ramps linearly) |
| `13`| **`Melt Time`** | `RECIPE -> 5. Cooling & Finishing` | `5` | `sec` | Bottom tip immersion time during `Melt Candle Bases` |
| `14`| **`Melt Depth`** | `RECIPE -> 5. Cooling & Finishing` | `8` | `mm` | How deep candle tips sink into wax during `Melt Candle Bases` |
| `9` | **`Soft Limit`** | `MACHINE -> 1. Jig & Wax Geometry`| `500` | `mm` | Absolute maximum downward Z-axis travel distance |
| `15`| **`Jig Length`** | `MACHINE -> 1. Jig & Wax Geometry`| `200` | `mm` | Distance from capacitive sensor down to bottom of candle tips |
| `23`| **`Wax Cutoff`** | `MACHINE -> 1. Jig & Wax Geometry`| `0` | `mm` | Minimum safe wax level (`Z > Cutoff` pauses batch & alerts operator) |
| `16`| **`Clean Time`** | `MACHINE -> 1. Jig & Wax Geometry`| `60` | `sec` | Full-depth soak duration during `Clean Whole Jig` maintenance |
| `8` | **`Col. Limit`** | `MACHINE -> 2. Scale & Collision` | `-50` | `g` | Upward collision force threshold (triggers immediate auto-retract) |
| `11`| **`Manual Speed`** | `MACHINE -> 3. Stepper Motor` | `50` | `%` | Carriage speed during manual jogging and homing |
| `12`| **`Brightness`** | `MACHINE -> 4. Screen & Theme` | `50` | `%` | LCD backlight brightness (`1%` to `100%`) |
| `—` | **`Volume`** | `MACHINE -> 6. Sound & Tunes` | `70` | `%` | Built-in I2S speaker volume (`0% MUTE` to `100%`) |
| `—` | **`Startup Tune`** | `MACHINE -> 6. Sound & Tunes` | `Standard` | `tune` | Boot melody (`Off`, `Standard`, `Dota 2 Ready`, `SpongeBob`, `Adv. Time`, `Classical`, `Heavy Metal`, `Mario 1-Up`) |
| `—` | **`Finish Tune`** | `MACHINE -> 6. Sound & Tunes` | `Standard` | `tune` | Batch complete melody (`Off`, `Standard`, `Dota 2 Ready`, `SpongeBob`, `Adv. Time`, `Classical`, `Heavy Metal`, `Kitchen Timer`, `iPhone Ding`, `Bird Song`) |
| `—` | **`Alert Tune`** | `MACHINE -> 6. Sound & Tunes` | `Standard` | `tune` | Collision / low-wax / error alarm (`Off`, `Standard`, `Mario Death`, `Dog Bark`, `Bird Chirp`, `Timer Ring`, `iPhone Ding`, `Metal Doom`, `Retro Fail`) |

---

## Section 4 — Detailed Parameter & Configuration Reference

### 4.1 Bucket 1: `RECIPE PARAMS` (Production Process Settings)

#### Category 1: `Target Weights` (`Slim Weight` & `Std Weight`)
* **What it does:** Sets the net wax accumulation target (in grams) for the entire 36-candle Kerzenkiste basket when operating in **`Weight` mode**.
* **How it works:** When you press `START` on the `AUTO` screen, the machine automatically tares (`0 g`) the scale with the dry jig and wicks hanging at Top Home (`0.0 mm`). After every dip and drip-off cycle, once the carriage returns to `0.0 mm` and settles for `400 ms`, the controller compares the live net wax weight against your selected target (`Slim Weight` or `Std Weight`). Once `Weight >= Target`, the batch finishes (or proceeds to `Auto-Melt Bases` if checked).
* **Sizing for 36 Candles:** Divide your total target weight by `36` to find the net wax weight per candle:
  $$\text{Weight per Candle (g)} = \frac{\text{Target Weight (g)}}{36}$$
  For example, `1,500 g` yields **`41.7 g` per candle**, while `2,500 g` yields **`69.4 g` per candle**.

#### Category 2: `Target Dip Counts` (`Slim Dips` & `Std Dips`)
* **What it does:** Sets the exact number of immersion cycles for `Slim` or `Standard` profiles when operating in **`Dips` mode**.
* **When to use `Dips` mode vs. `Weight` mode:** Use `Dips` mode when you want an exact number of concentric wax layers regardless of slight temperature variations, or use `Weight` mode when producing candles sold by guaranteed minimum weight.

#### Category 3: `Travel & Wax Speeds` (`Up Speed`, `Down Speed`, `Immerse Speed`, `Extract Speed`)
* **Why four speeds are used:** Moving the 36-candle basket through open air above the vat can be done rapidly (`Down Speed = 50%`, `Up Speed = 60%`) to minimize wasted travel time. However, entering and leaving the molten wax at high speed causes wave ripples, air bubbles trapped against the wick, and candle sway.
* **Automatic Two-Stage Downward Transition:**
  1. From `0.0 mm` down to `Base Touch - 15 mm`, the carriage travels at **`Down Speed`**.
  2. Exactly **`15 mm` before the candle tips touch the molten wax**, the controller smoothly shifts on-the-fly to **`Immerse Speed`** (`25%` default) until the capacitive sensor (`G13`) detects the wax surface.
  *(Note: On the very first dip after power-up before `Last Wax Level` has been learned, the carriage descends at `Immerse Speed` for the entire stroke so the first entry is always gentle).*
* **Automatic Two-Stage Upward Transition:**
  1. From full immersion depth (`Last Wax Level`) up to `Base Touch` (where the candle tips exit the liquid wax), the carriage lifts at **`Extract Speed`** (`20%` default).
  2. If **`Drip-Off`** is enabled (`Drip Dist != 0`), it pauses at `Base Touch - Drip Dist` for `Drip Time` seconds.
  3. Once the tips clear the wax (and finish dripping), the carriage accelerates to **`Up Speed`** (`60%` default) all the way back to Top Home (`0.0 mm`).

#### Category 4: `Dip & Drip Timing` (`Dip 1 Time`, `Sub Dip Time`, `Drip Dist`, `Drip Time`)
* **`Dip 1 Time` (Default `10 sec`):** On the very first dip (`Dip #1`), raw braided cotton wicks require extra time submerged in hot wax to expel trapped air micro-bubbles and absorb wax to their core.
* **`Sub Dip Time` (Default `4 sec`):** On `Dip #2` and all subsequent dips, the immersion dwell must be short enough that the hot liquid wax bonds to the previous layer without remelting it off the candle.
* **`Drip Dist` (Default `+15 mm`, supports `-` and `+` values) & `Drip Time` (Default `5 sec`):**
  Controls the **Drip-Off Pause** during extraction to eliminate heavy bulbous wax teardrops at the bottom of the candles:
  * **Positive `Drip Dist` (e.g., `+15 mm`)**: Pauses with the candle tips suspended **`15 mm` in the warm air above the molten wax surface** (`Z = Base Touch - 15 mm`) for `Drip Time` seconds so excess wax drips cleanly back into the vat before the carriage accelerates upward.
  * **Negative `Drip Dist` (e.g., `-5 mm`)**: Pauses with the bottom **`5 mm` of the candle tips still touching the molten wax surface** (`Z = Base Touch + 5 mm`) for `Drip Time` seconds — exploiting liquid surface tension to pull the hanging wax droplet off the candle bases!
  * **`0 mm`**: Disables the Drip-Off stage completely.

#### Category 5: `Cooling & Finishing` (`Rest Time`, `Start Cool`, `End Cool`, `Melt Time`, `Melt Depth`)
* **Tapered Cooling Curve (`Rest Time`, `Start Cool`, `End Cool`):**
  Early dips deposit a thin layer of wax over a cool wick and solidify quickly; late dips deposit wax onto a thick, warm candle body that holds substantial latent heat.
  * If both `Start Cool` and `End Cool` are left at **`0`**, every dip uses the constant **`Rest Time`** (default `25 sec`).
  * If you configure **`Start Cool`** (e.g., `15 sec` after Dip #1) and **`End Cool`** (e.g., `45 sec` on the final dip), the firmware automatically interpolates the cooling pause between each dip based on batch progress (`0.0` to `1.0` by dip count or accumulated weight):
    $$T_{\text{cool}}(p) = T_{\text{start}} + p \times (T_{\text{end}} - T_{\text{start}})$$
* **`Melt Time` (Default `5 sec`) & `Melt Depth` (Default `8 mm`):**
  Configures the **Candle Base Melting** stage (triggered either automatically at the end of a batch when `Melt Bases: ON` is checked on the `AUTO` screen, or manually from the `TOOLS` tab). The carriage lowers the finished candles until the bottom tips sink `Melt Depth` (`8 mm`) below the molten wax surface (`Z = Base Touch + Melt Depth`), holds for `Melt Time` (`5 sec`) to melt away the rough bottom wax tails, and lifts smoothly at `Extract Speed`.

---

### 4.2 Bucket 2: `MACHINE SETUP` (Hardware & Calibration Constants)

#### Category 1: `Jig & Wax Geometry` (`Soft Limit`, `Jig Length`, `Wax Cutoff`, `Clean Time`)
* **`Soft Limit` (Default `500 mm`):** Maximum physical downward travel from `0.0 mm`. If the carriage reaches `Soft Limit` during a dip without the capacitive sensor triggering, it stops descending and begins the bottom dwell so the carriage never bottoms out against the frame.
* **`Jig Length` (Default `200 mm` — adjust to your candle length):** The vertical distance (in `mm`) from the capacitive sensor's sensing face down to the bottom tips of the candles hanging on the Kerzenkiste basket. For example, if your wicks hang `350 mm` below the capacitive sensor trigger point, set `Jig Length = 350`.
* **`Wax Cutoff` (Default `0 mm` = Disabled):** Minimum safe molten wax level in the vat. Because `Z` increases downward from `0.0 mm` at the top, as wax is consumed by the candles, `Last Wax Level (mm)` increases. If you set `Wax Cutoff = 460 mm` and the wax surface drops below `460 mm` (`Last Wax Level > 460 mm`), the machine immediately aborts the dip, retracts to `0.0 mm` at `Up Speed`, and displays a **`LOW WAX LEVEL`** warning modal so you can top up the vat before producing short candles.
* **`Clean Time` (Default `60 sec`):** Dwell time for the **`CLEAN WHOLE JIG`** utility in the `TOOLS` tab, which submerges the entire Kerzenkiste basket down to the capacitive sensor (`Last Wax Level`) to melt off residual buildup.

#### Category 2: `Scale & Collision` (`Col. Limit` & 2-Step Load Cell Calibration)
* **`Col. Limit` (Default `-50 g`):** Upward push/unloading threshold on the load cell during downward motion. Because the jig hangs in tension from the load cell, if the descending candles hit unmelted wax or an obstacle, the downward force on the scale drops sharply (producing a negative delta relative to the tared reading). If `Weight <= Col. Limit` (e.g., `-50 g`), the safety interlock triggers immediately.
  * *Tip for Thick/Cool Wax:* If viscous beeswax resistance gives false collision alerts on 36 candles, make `Col. Limit` more negative (e.g., `-120 g` or `-150 g`).

#### Category 3: `Stepper Motor` (`Manual Speed`, `Enable`/`Disable`, `HOME`, `TEST DIP`)
* **`Manual Speed` (Default `50%`):** Controls the speed of the carriage when holding `UP` or `DOWN` on the `MANUAL` tab and during the initial homing seek.
* **`MOTOR: ENABLED / DISABLED`:** De-energizes the DM542T stepper driver (`ENA+ HIGH`) so the leadscrew or belt can be moved by hand during maintenance.

#### Category 4: `Screen & Theme` (`Brightness` & 3 Workshop Color Palettes)
* **`Brightness` (`1–100%`):** Adjusts the M5Stack CoreS3 SE LCD backlight PWM level and saves to NVS flash.
* **`Theme` Selector:** Cycles through 3 curated workshop color palettes:
  1. **`Ind. Slate`**: Deep midnight navy background (`#0D131A`), industrial teal header (`#0F766E`), steel blue primary cards (`#1E407C`), warm terracotta secondary cards (`#7F2D3A`), and emerald action buttons (`#10B981`).
  2. **`Bee Mine` (Default)**: Warm espresso background (`#1C1612`), golden honeycomb header (`#F5B023`), warm beeswax gold primary cards (`#FCD34D`), botanical meadow sage secondary cards (`#6ECDA5`), and clover green start button (`#4ADE80`).
  3. **`Midnight`**: High-contrast OLED obsidian (`#0A0E14`), charcoal-slate header (`#1E293B`), gunmetal indigo primary cards (`#253858`), deep teal-slate secondary cards (`#1F444C`), and bright emerald CTA (`#22C55E`).

#### Category 5: `Sound & Tunes` (`Volume`, `Startup Tune`, `Finish Tune`, `Alert Tune`)
* **Hardware Architecture:** Uses the M5Stack CoreS3 SE's onboard **1W I2S Speaker (AW88298 16-bit I2S amplifier)** via a dedicated non-blocking sound engine (`SoundManager.h` / `SoundManager.cpp`). All melodies are stored as compact Flash-resident `{freqHz, durationMs, gapMs}` note tables (`< 2.5 KB` total Flash, `0 B` RAM).
* **`Volume` (`0% MUTE` to `100%`, Default `70%`):** Drag across the volume bar or tap to step in `20%` increments. Plays a short confirmation chime at the newly selected volume and saves to NVS flash.
* **Live Preview on Selection:** Tapping **`Startup Tune`**, **`Finish Tune`**, or **`Alert Tune`** cycles to the next option, saves it to NVS flash, and **immediately plays the melody** so you can audition every tune directly from the setup screen:
  * **`Startup Tune` (8 Options):** `0: Off (Silent)`, `1: Standard`, `2: Dota 2 Ready` (Match Ready horn + anvil chime), `3: SpongeBob` (sea-shanty flute riff), `4: Adv. Time` (*Adventure Time* theme), `5: Classical` (Beethoven's *Ode to Joy*), `6: Heavy Metal` (palm-mute gallop + power chords), `7: Mario 1-Up`.
  * **`Finish Tune` (10 Options):** `0: Off (Silent)`, `1: Standard`, `2: Dota 2 Ready`, `3: SpongeBob`, `4: Adv. Time`, `5: Classical` (Beethoven's *Für Elise*), `6: Heavy Metal`, `7: Kitchen Timer` (double-bell ring), `8: iPhone Ding` (tri-tone crystal chime), `9: Bird Song` (robin/canary warble).
  * **`Alert Tune` (9 Options):** `0: Off (Silent)`, `1: Standard`, `2: Mario Death` (classic Super Mario Bros. "lost a life" jingle), `3: Dog Bark` ("Woof! Woof!"), `4: Bird Chirp` (startled bird alarm), `5: Timer Ring`, `6: iPhone Ding`, `7: Metal Doom` (tritone), `8: Retro Fail`.

---

## Section 5 — Step-by-Step Operating Procedures

### 5.1 Pre-Start & Z-Axis Homing (`MANUAL -> HOME`)
1. Switch on the main power supply and M5Stack CoreS3 SE controller.
2. Verify that the vat wax is fully melted and at your target dipping temperature (typically `62–65 °C` for pure beeswax).
3. Tap the **`MANUAL`** tab on the bottom navigation bar and press **`HOME`**.
4. The carriage will:
   - Rise at `Manual Speed` until the top optical switch (`G7`) is triggered,
   - Back off `10 mm` downward, and
   - Creep slowly upward at `10%` speed to establish a repeatable **`0.0 mm`** top reference point.

### 5.2 Loading the 36-Candle Jig & Running a Dipping Batch (`AUTO`)
1. Hang the threaded **Kerzenkiste 36-Candle Dipping Basket** onto the load-cell mounting hook at Top Home (`0.0 mm`).
2. Tap the **`AUTO`** tab:
   - Tap the **Top-Left Card (`Mode`)** to toggle between **`Weight`** (stops at target net grams of wax) and **`Dips`** (stops at target dip count).
   - Tap the **Top-Right Card (`Profile`)** to toggle between **`Slim`** and **`Standard`**.
   - Tap the **Middle Checkbox Bar (`Melt Bases: ON/OFF`)** to enable or disable automatic candle bottom leveling after the final dip.
3. Press **`START`**.
4. **Automatic Sequence:**
   - **Step 0 (`HOMING & AUTO-TARE`)**: If not already at the top switch, the carriage homes to `0.0 mm`, pauses `500 ms` for the 36-candle basket to stop swinging, and **automatically zeroes (`0 g`) the load cell** so the weight of the stainless-steel basket (`~720 g` + handle + dry wicks) is completely excluded.
   - **Step 1 (`MOVING DOWN`)**: Descends at `Down Speed` until `15 mm` above the wax (`Base Touch - 15 mm`), then slows to `Immerse Speed` until the capacitive sensor (`G13`) detects the molten wax surface and updates `Last Wax Level (mm)`.
   - **Step 2 (`IMMERSING`)**: Holds at full depth for `Dip 1 Time` (on Dip #1) or `Sub Dip Time` (on subsequent dips).
   - **Step 3 (`LIFTING & DRIP-OFF`)**: Withdraws the candles smoothly through the wax at `Extract Speed`. If `Drip Dist != 0`, pauses for `Drip Time` seconds to shed the hanging wax drop, then accelerates to `Up Speed` back to `0.0 mm`.
   - **Step 4 (`COOLING AT TOP`)**: Settles for `400 ms`, measures net accumulated wax weight, checks whether the target (`Weight` or `Dips`) has been reached, and counts down the interpolated cooling pause (`Start Cool` -> `End Cool` or `Rest Time`).
   - **Step 5 (`MELTING BASES` — Optional)**: Once the final dip cooling finishes, if `Melt Bases: ON` is enabled, lowers the candle tips `Melt Depth` (`8 mm`) into the wax for `Melt Time` (`5 sec`) and lifts smoothly back to `0.0 mm`.
   - **Step 6 (`PROCESS COMPLETE`)**: Displays final Dip Count and Net Wax Weight (`g`). Tap `OK / FINISH` to return to `AUTO`.

### 5.3 Pause / Resume & Emergency Stop Behavior
* **`PAUSE` (Amber Button)**: Pauses the active countdown timer or halts carriage travel immediately. While paused, tap **`RESUME`** to continue from the exact same stage or **`ABORT`** to retract to `0.0 mm` and cancel the batch.
* **`STOP` (Red Button)**: Immediately cancels the active cycle and **automatically retracts the carriage to `0.0 mm` Top Home** so the candles are never left submerged in hot wax.

### 5.4 Finishing & Maintenance Cycles (`TOOLS`)
Tap **`TOOLS`** on the bottom navigation bar for standalone operations:
1. **`MELT CANDLE BASES`**: Runs the base-melting stage independently (`Z = (Last Wax Level - Jig Length) + Melt Depth` for `Melt Time` seconds). Note: Requires a valid `Last Wax Level` greater than `Jig Length` (either from a previous dip or a `TEST DIP`).
2. **`CLEAN WHOLE JIG`**: Submerges the entire Kerzenkiste basket down until the capacitive sensor triggers (`Last Wax Level`), holds submerged for `Clean Time` (default `60 sec`) to melt off old wax buildup, and lifts back to `0.0 mm`.

---

## Section 6 — Load Cell Calibration & Safety Interlocks

### 6.1 2-Step Load Cell Calibration Wizard (`TARE (0g)` + `CAL WT`)
Your load cell only needs calibration once during initial machine commissioning (or if you replace the load cell hardware). The calibration factor (`Scale Factor`) is stored permanently in non-volatile flash memory.

1. Go to **`SETUP -> 2. MACHINE SETUP -> 2. Scale & Collision`**.
2. **Step 1 — Zero Offset (`TARE (0g)`)**:
   - Hang the empty mounting hook (or empty jig) so it is free-hanging and stationary.
   - Tap **`TARE (0g)`**. The live reading will reset to `0 g`.
3. **Step 2 — Known Reference Weight (`CAL WT`)**:
   - Place or hang an accurately weighed calibration mass (for example, a **`1000 g`** calibration weight or a weighed `1000 g` spool) on the jig.
   - Tap **`CAL WT`**, enter **`1000`** on the on-screen numpad, and tap **`OK`**.
   - The firmware reads a 10-sample average from the HX711, computes the exact `Scale Factor` (`rawDelta / 1000`), saves it to flash (`sfac`), and confirms `Calibrated! Factor saved`.

### 6.2 Collision Detection & Auto-Retract Recovery
If the load cell detects an upward push (`Weight <= Col. Limit`, default `-50 g`) while moving downward:
1. The stepper motor **immediately reverses direction** and retracts the carriage at `Up Speed` (`60%`) all the way back to the top optical home switch (`0.0 mm`) so the jig is never trapped in the wax vat.
2. The screen switches to a high-visibility red **`COLLISION DETECTED`** recovery dialog showing the status `Retracted to 0.0mm` and live scale reading.
3. Clear the obstruction or inspect the vat temperature, then choose:
   - **`CONTINUE`**: Resumes the current dip cleanly from the top, or
   - **`ABORT`**: Cancels the batch and returns to the `AUTO` dashboard.

### 6.3 Low-Wax Level Cutoff (`Wax Cutoff`)
If `Wax Cutoff` is set above `0 mm` (e.g., `460 mm`) and the capacitive sensor detects that the molten wax surface has dropped below that height (`Last Wax Level > Wax Cutoff`):
1. The carriage immediately aborts the immersion and retracts to `0.0 mm` at `Up Speed`.
2. The screen displays **`LOW WAX LEVEL`** (`Wax at XXXmm > Cutoff YYYmm`).
3. Refill the wax vat with molten wax and tap **`CONTINUE`** to re-dip at the new wax height, or tap **`ABORT`** to end the batch.
