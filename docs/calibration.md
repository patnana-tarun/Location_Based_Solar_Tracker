# Calibration Guide

Calibration ensures that the servo motors move to the correct physical angles and that the current sensor reads zero accurately. This should be done whenever you assemble a new unit or swap servos.

---

## Step 1 — Current Sensor Auto-Calibration (Automatic)

The firmware performs this automatically at every power-on.

1. Power on the board **without connecting any load** to the solar panel terminals
2. Wait for the OLED to show `Solar Tracker / v6.0 Ready`
3. The firmware takes **50 samples × 50 readings** over ~3 seconds
4. It computes and stores a `current_zero_offset` value
5. All future current readings subtract this offset

> 💡 If the current always reads non-zero with no load connected, check that the ACS712 VCC is exactly 5.0V. Fluctuating supply voltage causes calibration drift.

---

## Step 2 — Servo Mechanical Calibration (CALIBRATION_MODE)

This mode lets you visually verify that the servos move correctly before attaching the solar panel.

### How to Enable Calibration Mode

Open `firmware/src/main.c` and change this line:

```c
#define CALIBRATION_MODE 0    // Change to 1
```

**Re-build and flash** the firmware.

### What Calibration Mode Does

The servo runs through this automated sequence:

| Test | Action |
|---|---|
| **TEST 1: Azimuth 0→180°** | Steps azimuth servo from 0° to 180° in 10° increments |
| **TEST 2: Elevation 0→90°** | Steps elevation servo from 0° to 90° in 10° increments |
| **TEST 3: Sun Positions** | Moves to Sunrise → Morning → Noon → Afternoon → Sunset |
| **Return Home** | Returns to sunrise position (Az=30°, El=10°) |

At each step:
- The servo moves smoothly (0.5°/step, 40ms delay — very slow and gentle)
- A **2-second pause** occurs so you can observe and measure the angle
- The OLED displays the current angle
- UART prints the angle to your serial monitor

### What to Check During Calibration

For each position:
- ✅ Does the servo move to the expected physical angle?
- ✅ Is there any mechanical binding or grinding?
- ✅ Are the wires long enough to not pull at the extremes?
- ✅ No mechanical "end stop" hitting at 0° or 180°?

### After Calibration

If the mechanical angles look correct, set the mode back to normal:

```c
#define CALIBRATION_MODE 0
```

If the servo isn't at the expected angle, you may need to:
- Adjust the `SERVO_MIN_PULSE` / `SERVO_MAX_PULSE` values
- Trim the servo horn mounting angle
- Add a mechanical offset to your mount

---

## Step 3 — Check Mode (Runtime Test)

Without reflashing, you can test the servo range at any time using the **hardware button** on PC13.

### How to Use Check Mode

1. Power on the board normally (tracking mode)
2. Press the user button once
3. The OLED displays `CHECK MODE` and the servos run:
   - Move to **0°, 0°** (home)
   - Move azimuth to **180°**
   - Move elevation to **90°**
   - Return to **neutral (90°, 90°)**
4. The OLED shows the current position and GPS/time info
5. Press the button again (or wait 30 seconds) to exit back to tracking

---

## Configurable Parameters Summary

| Parameter | Default | File | Description |
|---|---|---|---|
| `DEMO_MODE` | `1` | `main.c` | `1` = simulation, `0` = real tracking |
| `CALIBRATION_MODE` | `0` | `main.c` | `1` = enable calibration sweep |
| `FIXED_LATITUDE` | `10.90` | `main.c` | Fallback latitude (decimal degrees) |
| `FIXED_LONGITUDE` | `76.90` | `main.c` | Fallback longitude (decimal degrees) |
| `DEFAULT_TIMEZONE` | `5.5` | `main.c` | UTC offset (India = 5.5) |
| `SUNRISE_HOUR` | `6` | `main.c` | Hour to exit night mode |
| `SUNSET_HOUR` | `18` | `main.c` | Hour to enter night mode |
| `SUNRISE_AZIMUTH_POS` | `30.0°` | `main.c` | Azimuth parked position at night |
| `SUNRISE_ELEVATION_POS` | `10.0°` | `main.c` | Elevation parked position at night |
| `SMOOTH_STEP_SIZE` | `1.0°` | `main.c` | Step size for smooth servo movement |
| `SMOOTH_STEP_DELAY` | `40 ms` | `main.c` | Delay between smooth steps |
| `SERVO_MIN_PULSE` | `500 µs` | `main.c` | PWM pulse for 0° position |
| `SERVO_MAX_PULSE` | `2500 µs` | `main.c` | PWM pulse for 180° position |
