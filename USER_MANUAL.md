# Bee Mine Candle Dipper: User Manual

*User Manual for firmware as of November 2026. Controller: M5Stack CoreS3 SE.*

A print-ready Word version of this manual is in `docs/Bee Mine Candle Dipper - User Manual.docx`.

## 1. What this machine does

The Bee Mine Candle Dipper makes dipped beeswax candles automatically. A jig of wicks hangs from an arm. The machine lowers the jig into a pot of molten wax, holds it there, lifts it out, lets the new layer cool, and repeats until the candles are finished.

It decides when the candles are finished in one of two ways, which you choose before each run:

- Weight mode: it weighs the jig after every dip and stops when the added wax reaches the target weight.
- Dips mode: it stops after a set number of dips.

Two candle profiles, Slim and Standard, each have their own target weight and dip count.

## 2. Safety

> **Warning:** Molten wax causes serious burns and wax vapour can ignite. Never leave the wax heater unattended, and keep a lid or fire blanket within reach.

- Keep hands, hair, sleeves and cables away from the arm, the lead screw and the pot whenever the motor is powered. The arm can start moving by itself during a run (for example at the end of a cooling pause).
- Touching the screen anywhere during a run pauses it and lifts the jig out of the wax. This is the quickest way to stop.
- To cut motor power immediately, switch off the 24 V supply. The motor then no longer holds the arm, so a heavily loaded jig may drift; support it or lift it clear by hand.
- The machine cannot detect a person or an object under the arm unless it changes the weight on the scale. Do not rely on collision detection to protect hands.
- Set the Soft Limit (section 8.2) to a depth the arm can reach safely. It is the only protection against driving the jig into the bottom of the pot if the wax sensor fails.
- Let the wax sensor face and the jig cool before cleaning them.

## 3. Parts of the machine

| Part | What it does |
| --- | --- |
| Control box with touchscreen | M5Stack CoreS3 SE controller. Everything is operated from its 2.0 inch touchscreen. |
| Arm and carriage | Moves up and down on a linear rail, driven by a lead screw and stepper motor. A counterweight takes most of the load. |
| Top switch | Tells the machine where the top of travel is. All positions are measured downwards from here. |
| Wax sensor | A capacitive sensor on the arm. It stops the downward move when it reaches the wax surface, however full the pot is. |
| Load cell (scale) | Weighs the jig and candles. Used for the weight target and for collision detection. |
| Jig | The candle carousel that clips to the arm and holds the wicks. |
| 24 V power supply | Powers the motor and the wax sensor. A converter inside the box makes 5 V for the controller and the scale. |

### 3.1 How positions are measured

Position is shown in millimetres. 0 mm is the top rest position, 2 mm below the top switch. Larger numbers are further down. The machine only knows its position after it has found the top switch (homing). It homes by itself whenever it needs to, so you do not normally have to do it by hand.

- Wax level: the position at which the wax sensor last touched the wax. It is measured again on every dip, so it follows the level down as wax is used.
- Full tank level: the wax depth when the tank is full. You set it once (section 8.3). The arm always starts moving slowly when the bottom of the candles is 15 mm above this height.
- Jig length: the distance from the wax sensor down to the bottom of the candles. You enter this once (section 8.2).
- Base position: wax level minus jig length. This is where the bottoms of the candles meet the wax surface.

## 4. The screen

Every screen has a status bar at the top. The four main screens have a row of tabs at the bottom: DIP, MANUAL, TOOLS and SETUP. Inside the settings, the bottom bar becomes a BACK button.

| Tab | Use it for |
| --- | --- |
| DIP | Choosing mode and profile, and starting an automatic run. |
| MANUAL | Moving the arm by hand, homing, a single test dip, and zeroing the scale. |
| TOOLS | Melting candle bases flat and cleaning wax off the jig. |
| SETUP | All settings, in two groups: Recipe Params and Machine Setup. |

### 4.1 Changing a number

Tap a setting card to open the number pad. The current value is shown in grey until you type.

- Type the new value and tap OK to save it.
- DEL removes the last digit. CLR clears everything.
- CANCEL closes the pad without changing anything. Tapping OK with nothing typed also leaves the setting unchanged.
- For Col. Limit and Drip Offset a +/- button appears to switch the sign.

Settings are saved immediately and are kept when the power is off.

## 5. Before the first run

1. Check that nothing is in the path of the arm and that the pot is in position.
2. Switch on. The splash screen shows and the start-up tune plays.
3. Go to MANUAL and tap HOME. The arm rises to the top switch, backs off, approaches again slowly and settles at 0 mm.
4. Calibrate the scale (section 9) if this has not been done, or if the load cell or its wiring has changed.
5. Check the Jig Below Sens and Soft Limit settings match your jig and pot (section 8.2).
6. Set the Full Tank Level (section 8.3). Without it the arm moves at Immerse Speed the whole way down.
7. With the wax fully molten, clip on the empty jig and run a test dip with DIP BOT on the MANUAL screen (section 6.3). This also records the wax level.

## 6. Manual screen

The status bar shows the position (POS), the weight (WT) and, in the middle, the sensor state: TOP HIT when the top switch is pressed, WAX HIT when the wax sensor is triggered, otherwise TOP OPEN.

### 6.1 Moving the arm (UP and DOWN)

The arrow buttons latch. Tap once to start moving and tap again to stop. You do not hold them.

- Tap the UP or DOWN arrow: the arm starts moving and the button turns green.
- Tap the same arrow again, or any empty part of the screen, to stop. The arm slows to a halt.
- Tapping the opposite arrow reverses direction.
- The - and + buttons between the arrows set the speed from 5 to 100 mm/s in steps of 5.
- UP stops by itself at the top. DOWN stops by itself when the wax sensor touches wax or at the Soft Limit.
- Until the arm has been homed, upward speed is limited to 20 mm/s because the machine does not yet know how close the top switch is.

> **Note:** Because the arrows latch, the arm keeps moving until you stop it or it reaches a limit. Stay at the screen while it moves.

### 6.2 HOME

Finds the top switch and sets 0 mm. While homing, the button reads STOP; touching the screen stops the move. If the switch is not found within the full travel, the move stops and the alert tune plays. The HOME button is highlighted whenever the top switch is pressed.

### 6.3 DIP BOT (single test dip)

Lowers until the wax sensor touches the wax, records the wax level, holds for the '+ Dips Time', then returns to the top. Use it to check the sensor and to set the wax level before a run. Touching the screen stops it; if the jig is in the wax it is lifted out first. The DIP BOT button is highlighted whenever the wax sensor is triggered.

### 6.4 TARE

Sets the weight reading to zero with whatever is hanging on the arm. The zero is not kept when the power is switched off.

## 7. Running a batch (DIP tab)

1. Melt the wax fully and stir. Solid wax under the surface will be detected as a collision.
2. Hang the wicked jig on the arm.
3. On the DIP tab, tap the MODE button to choose WEIGHT or DIPS.
4. Tap MELT BASES if you want the candle bottoms melted flat at the end of the run (section 7.3).
5. Tap SLIM or STANDARD. Each card shows its target and an estimated run time.
6. Tap START DIP.

### 7.1 What happens during a run

| Stage | What the machine does |
| --- | --- |
| Prepare | Homes if it needs to, parks at the top and zeroes the scale, so that only added wax is weighed. |
| Lowering | Moves down at Down Speed. Just before the candle bottoms reach the wax it slows to Immerse Speed, and continues until the wax sensor touches the surface. The slow-down always starts 15 mm before the bottom of the candles reaches the Full Tank Level, however far the wax has dropped. If the Full Tank Level is not set, the last measured wax level is used instead; if neither is known, the arm moves at Immerse Speed the whole way down. |
| Holding | Stays in the wax for the 1st Dip Time on the first dip and the + Dips Time on later dips. |
| Extracting | Lifts out at Extract Speed. If a drip pause is set, it stops at the Drip Offset for the Drip Time. Once the candles are clear it rises at Up Speed, slowing for the last few millimetres. |
| Weighing | At the top it waits for the Weight Dwell, then weighs. |
| Cooling | Waits at the top for the cooling time, then starts the next dip. |
| Finish | When the target is reached it plays the finish tune and shows the total time. Tap FINISH. |

The screen shows the current stage, the elapsed and estimated time, and the weight or dip count against the target. The estimate is the average of the last five completed runs for that mode and profile.

### 7.2 Pausing and stopping

Touch the screen anywhere during a run. The jig is lifted to the top and the RUN PAUSED screen appears.

- RESUME (green, left) continues the run. If the pause interrupted a dip, that dip is repeated.
- ABORT (red, right) ends the run. Aborted runs are not used for time estimates.

### 7.3 Melt Bases

When MELT BASES is ticked, the run ends with an extra stage: after a final cool, the candle bottoms are lowered just to the wax surface for the Base Melt Time so they melt flat, then lifted out. It needs a known wax level, so it is skipped if no dip has reached the wax yet.

## 8. Settings

Open SETUP and choose Recipe Params (how the candles are made) or Machine Setup (how the machine is built and behaves).

### 8.1 Recipe Params

| Setting | Default | What it does |
| --- | --- | --- |
| Slim Weight | 1500 g | Wax to add in Weight mode, Slim profile. |
| Std Weight | 2500 g | Wax to add in Weight mode, Standard profile. |
| Slim Dips | 15 | Number of dips in Dips mode, Slim profile. |
| Std Dips | 25 | Number of dips in Dips mode, Standard profile. |
| Max Dips | 60 | Safety limit for Weight mode. If the target weight has not been reached after this many dips, the run stops with an alert. |
| Up Speed | 60 mm/s | Lifting speed in open air. |
| Down Speed | 50 mm/s | Lowering speed in open air. |
| Immerse Speed | 25 mm/s | Speed from 15 mm above the wax until the sensor touches. 0 uses Down Speed. |
| Extract Speed | 20 mm/s | Speed while the candles are leaving the wax. 0 uses Up Speed. |
| 1st Dip Time | 10 s | Time in the wax on the first dip, to soak the wicks. |
| + Dips Time | 4 s | Time in the wax on every later dip. |
| Drip Offset | +15 mm | Where to pause on the way out, relative to the wax surface. Positive is above the wax, negative is still in it. 0 switches the pause off. |
| Drip Time | 3 s | Length of the drip pause. 0 switches it off. |
| Default Cool | 15 s | Cooling time at the top between dips. |
| 1st Cool Time | 0 | Cooling time after the first dip. 0 uses Default Cool. |
| Last Cool Time | 0 | Cooling time after the last dip. 0 uses Default Cool. If the first and last differ, the time changes steadily between them over the run. |

Speeds can be set from 1 to 150 mm/s. The specification for this machine is up to about 50 to 60 mm/s; test any higher value with an empty jig first.

### 8.2 Machine Setup

| Setting | Default | What it does |
| --- | --- | --- |
| Jig Below Sens | 240 mm | Distance from the wax sensor down to the bottom of the candles. Measure it on your jig. |
| Full Tank Level | 0 (not set) | Wax depth when the tank is full. Set on its own page (section 8.3). |
| Base Melt Time | 5 s | How long candle bottoms are held at the wax surface when melting bases. |
| Clean Jig Time | 30 s | How long the jig is held in the wax by Clean Whole Jig. |
| Soft Limit | 500 mm | The furthest the arm may travel down. If the wax has not been found by this depth, the run stops with a Low Wax alert. |
| Col. Limit | -50 g | Collision sensitivity: how far the weight may drop while lowering before the machine assumes the jig has hit something. 0 switches collision detection off. |
| Weight Dwell | 500 ms | Settling time at the top before weighing. |
| Load Cell Cal. |  | Opens the scale calibration (section 9). |
| Driver Mode | Spread | Stealth is quietest. Spread gives more force at speed. Adaptive uses Stealth below the Hybrid Speed and Spread above it. |
| Hybrid Speed | 30 mm/s | The changeover speed for Adaptive mode. |
| Motor Power | Enabled | Disabled releases the motor so the arm can be moved by hand. The machine homes again before its next automatic move. |
| Brightness | 50 % | Drag the bar. |
| Color Theme | Bee Mine | Tap to cycle: Ind. Slate, Bee Mine, Midnight. |
| Volume | 70 % | Drag the bar, or tap the card to step by 20 %. 0 is mute. |
| Startup / Finish / Alert Tune | Standard | Tap to cycle through the tunes. Each one plays as you select it. |
| Wi-Fi Portal |  | Sets up Wi-Fi for the clock (section 10). |

> **Note:** Collision detection allows for buoyancy. Candles floating in molten wax weigh less on the scale, so once the candles enter the wax the allowed drop grows by the weight of wax already on them. In open air the limit is exactly the Col. Limit value. If thick or cool wax still gives false alerts, make the Col. Limit more negative.

### 8.3 Full Tank Level (Machine Setup, Wax Level)

This is the depth of the wax surface, in millimetres below the top rest position, when the tank is full. It decides where the arm slows down on the way into the wax: on every dip the arm moves quickly through the air and slows to Immerse Speed when the bottom of the candles is 15 mm above this height. The slow-down always starts at the same place, so every dip enters the wax in the same gentle way.

The machine still measures the real wax level on every dip. That measured level is used for lifting the candles out of the wax, the drip pause, Melt Bases, Clean Whole Jig and the low wax warning. It is not used to decide where to slow down.

The Wax Level page has two options:

- ENTER BY HAND: type the depth in millimetres.
- MEASURE WITH SENSOR: the arm lowers slowly (15 mm/s) until the wax sensor triggers, stores that depth, and returns to the top. With a full tank it finds the wax surface. If the tank is not full, hold a sheet of paper, card or your hand at the height the full surface would be, below the sensor and clear of the jig, and the sensor will trigger on that instead.

> **Warning:** While measuring, the arm moves down without stopping until the sensor triggers or the Soft Limit is reached. Keep hands clear of the jig and the candles. Touch the screen at any time to stop; the arm then rises to the top. A capacitive sensor reacts best to water-containing objects such as a hand or a damp cloth and may not react to dry paper; if nothing is detected, try a damp cloth or increase the sensor's sensitivity.

If the sensor already reads WAX HIT before the move starts, or nothing is found before the Soft Limit, the alert tune plays and nothing is stored.

- Set this to the HIGHEST level the wax will reach (a full tank). If the real wax is ever higher than the value, the candles reach it at full Down Speed.
- As the wax is used up and its level drops, the candles spend longer at Immerse Speed on the way down. For example, if the wax has dropped 100 mm and Immerse Speed is 25 mm/s, every dip takes about 4 seconds longer. Top up the tank, or remeasure the value if you change the tank, the pot or the wax amount.
- If the Full Tank Level is not set, the machine falls back to the last measured wax level. If no wax level has ever been measured either, the arm moves at Immerse Speed from the start. That is safe but slower.

## 9. Calibrating the scale

You need an object of known weight, ideally 500 g to 2 kg. Calibration is kept when the power is off.

1. Open SETUP, Machine Setup, Scale & Collision, Load Cell Cal.
2. Leave the arm as it will be used but with the test weight removed, and wait for the Live reading to settle.
3. Tap 1. ZERO UNLOADED SCALE. The reading becomes 0.0 g.
4. Hang the known weight on the arm and wait for the reading to settle.
5. Tap 2. SET KNOWN WEIGHT, type the weight in grams and tap OK.
6. The Live reading should now show the known weight. Remove it and check the reading returns to about 0.

If the reading does not change at all when weight is added, see Troubleshooting (section 12).

## 10. Clock and Wi-Fi

Wi-Fi is used only to set the clock in the status bar. The machine works fully without it.

1. Open SETUP, Machine Setup, Wi-Fi Portal. The screen shows the network name BeeMine-Setup.
2. On a phone, join the Wi-Fi network BeeMine-Setup. A setup page should open by itself; if it does not, browse to http://192.168.4.1
3. Enter your Wi-Fi name and password (leave the password empty for an open network) and tap Save & Connect.
4. The machine joins your network and sets the clock within a few seconds of connecting. UK time and summer time are applied automatically.

To leave the portal without changing anything, tap the bottom bar.

## 11. Tools tab

The status bar shows the last measured wax level and the base position. WAX: UNSET means no dip has reached the wax yet.

### 11.1 Melt Candle Bases

Lowers the candle bottoms to the wax surface, holds for the Base Melt Time and lifts out. It needs a known wax level and a wax level deeper than the jig length; otherwise the alert tune plays and nothing moves.

### 11.2 Clean Whole Jig

Lowers until the wax sensor touches the wax, holds the jig submerged for the Clean Jig Time to melt off old wax, then lifts out.

Both tools can be stopped by touching the screen. If the jig is in the wax it is lifted out first.

## 12. Messages and troubleshooting

### 12.1 Alert screens during a run

In each case the jig has already been lifted to the top where that was possible. Fix the cause, then tap RESUME or ABORT.

| Message | Meaning | What to do |
| --- | --- | --- |
| COLLISION ALERT | The weight dropped more than the Col. Limit while lowering. | Look for unmelted wax, a snagged cable or a misaligned arm. If nothing is wrong, make the Col. Limit more negative. |
| LOW WAX ALERT | The Soft Limit was reached without finding wax. | Top up the pot, or check that the wax sensor still detects wax. |
| Wax sensor ON at top: clean it | The wax sensor reads 'wax' before the move starts. | Clean wax film off the sensor face. If it is clean, check the sensor adjustment and wiring. |
| Top switch not found! | The arm travelled further up than expected without reaching the top switch. | Check the top switch and its wiring, and that the arm is not jammed. RESUME homes again. |
| Homing failed: check top switch | Homing could not complete. | As above. |
| MAX DIPS REACHED | Weight mode reached the Max Dips limit before the target weight. | Check the scale reads correctly (section 9), that the pot is not too hot for wax to build up, and that the target is realistic. |

The status bar on the DIP tab also shows '! LOW WAX' when the wax level is within 20 mm of the Soft Limit.

### 12.2 Other problems

| Problem | Likely cause and fix |
| --- | --- |
| Weight always reads 0.0 g and calibration does nothing | The scale is not being read. Check the two scale signal wires and its 5 V supply. The scale clock must be on GPIO 10, not GPIO 13 (see the Project Explanation). |
| Motor buzzes or stalls when starting | Lower the speed. If it persists, the acceleration or motor current in the firmware needs adjusting. |
| Arm does not move | Check 24 V is on and Motor Power is Enabled. |
| UP arrow does nothing | The arm is already at the top (TOP HIT), or at 0 mm. |
| DOWN arrow stops at once | The wax sensor is triggered (WAX HIT) or the Soft Limit has been reached. |
| Run holds in mid-air | The wax sensor triggered early. Clean the sensor face and check its sensitivity adjustment. |
| Every dip is slow on the way down | The wax has dropped well below the Full Tank Level, or the Full Tank Level is not set (the whole descent then runs at Immerse Speed). Remeasure the Full Tank Level (section 8.3) after topping up, or lower it to match your normal wax level. |
| Clock shows 01/01/26 | The clock has never been set. Set up Wi-Fi (section 10). |
| No sound | Check Volume is above 0 and the tune is not set to Off. |

## 13. Care

- Wipe wax off the wax sensor face after each session, once it has cooled. A film of wax is the most common cause of false triggers.
- Keep the lead screw and rail free of wax drips.
- Check the top switch operates freely.
- Re-check the scale with a known weight from time to time.
