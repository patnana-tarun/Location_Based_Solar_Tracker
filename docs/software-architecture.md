# Software Architecture

This document describes how the firmware and PC dashboard software are structured, what each module does, and how data flows through the system.

---

## Overview

The software consists of two main parts:

1. **Firmware** — C code running on the STM32 microcontroller (bare-metal HAL)
2. **Dashboard** — Python backend + HTML/JS frontend running on a PC

```
┌──────────────────────────────────────────────────────┐
│                   FIRMWARE (STM32)                    │
│                                                      │
│  main()                                              │
│    ├── Initialization                                │
│    │     ├── HAL_Init()                              │
│    │     ├── Servo_Init()                            │
│    │     ├── ADC_Init()                              │
│    │     ├── SSD1306_Init()                          │
│    │     ├── DS3231_SetTime()                        │
│    │     └── Current sensor calibration             │
│    │                                                │
│    └── Main Loop (while 1)                          │
│          ├── CheckMode_CheckButton()                 │
│          ├── [if GPS mode] parse NMEA               │
│          ├── DS3231_GetTime()                        │
│          ├── calculateSolarPosition()                │
│          ├── Servo_SmoothMove() x2                  │
│          ├── ADC_ReadVoltage() + ADC_ReadCurrent()   │
│          ├── UART_SendTelemetry()                    │
│          └── OLED_DisplayUnified()                  │
└──────────────────────────────────────────────────────┘
                         │ UART $TELEM packets
                         ▼
┌──────────────────────────────────────────────────────┐
│              PC DASHBOARD (Python + HTML)             │
│                                                      │
│  server.py                                           │
│    ├── Reads UART (pyserial)                         │
│    ├── Parses $TELEM packets                         │
│    ├── Calls SPA executable (spa_calc.exe) for ref   │
│    ├── Calculates accuracy delta                     │
│    └── Broadcasts JSON over WebSocket (8765)         │
│                                                      │
│  index.html (browser)                               │
│    └── Receives WebSocket JSON → updates UI          │
└──────────────────────────────────────────────────────┘
```

---

## Firmware Modules

### `main.c` — Entry Point and Main Loop

The entire firmware is in a single `main.c` file for simplicity. It is divided into clearly labeled sections:

| Section | Purpose |
|---|---|
| Configuration macros | `DEMO_MODE`, `USE_GPS_LOCATION`, lat/lon constants |
| Structures | `DateTime`, `SolarPosition`, `RTC_Time` |
| DS3231 driver | `DS3231_GetTime()`, `DS3231_SetTime()`, `ParseCompileTime()` |
| UART helpers | `DBG()` for debug, `UART_SendTelemetry()` for structured output |
| OLED display | `OLED_DisplayUnified()` — single 4-line display function |
| Servo control | `Servo_Init()`, `Servo_SetAngle()`, `Servo_SmoothMove()` |
| ADC | `ADC_Init()`, `ADC_ReadVoltage()`, `ADC_ReadCurrent()` |
| GPS parser | `split_csv()`, `nmea_to_deg()`, `parse_rmc()`, `parse_gga()` |
| Solar algorithm | `getJulianDay()`, `calculateSolarPosition()` |
| Check mode | `CheckMode_CheckButton()`, `CheckMode_Process()` |
| `main()` | Init sequence + main tracking loop |

---

### Solar Position Algorithm

This is the mathematical heart of the firmware. It calculates where the sun is in the sky at any given time and location.

**Inputs:**
- `DateTime` struct (year, month, day, hour, minute, second)
- Latitude (degrees, positive north)
- Longitude (degrees, positive east)
- UTC timezone offset (e.g., India = +5.5)

**Outputs:**
- `azimuth` — compass direction of the sun (0° = North, 90° = East, 180° = South, 270° = West)
- `elevation` — angle above the horizon (0° = horizon, 90° = directly overhead)
- `zenith` — 90° − elevation

**Key Steps:**
1. Convert date/time to **Julian Day (JD)** — a continuous day count from a fixed epoch
2. Compute the sun's **geometric mean longitude (L)** and **mean anomaly (M)**
3. Compute the **equation of center (C)** — the difference between the mean and true position
4. Calculate sun's **right ascension (RA)** and **declination (δ)**
5. Compute the observer's **hour angle (HA)** using Greenwich Sidereal Time (GMST)
6. Compute **elevation** from spherical coordinate geometry
7. Apply **atmospheric refraction correction** (especially important near the horizon)
8. Compute **azimuth** from the elevation and hour angle

This algorithm is adapted from the NREL SPA and produces results accurate to within ±0.001° over a multi-century range.

---

### Servo Control

Servo movement is controlled via **TIM2 in PWM mode**:

```
TIM2 Configuration:
  Prescaler = 15 (÷16 from 16 MHz HSI → 1 MHz timer clock)
  Period = 19999 (20 ms period → 50 Hz servo frequency)

Pulse mapping:
  0°   → 500  µs → CCR = 500
  90°  → 1500 µs → CCR = 1500
  180° → 2500 µs → CCR = 2500
```

`Servo_SmoothMove()` increments the current angle by 1° per step, with a 40ms HAL_Delay between steps. This creates smooth motion and protects the mechanical assembly.

---

### GPS NMEA Parser

The GPS module sends NMEA sentences as ASCII text at 9600 baud. The parser uses interrupt-driven UART receive (`HAL_UART_Receive_IT`) to collect characters into a buffer.

When a newline is detected, the buffer is checked for:
- `$GPRMC` — extracts latitude, longitude, and validity flag
- `$GPGGA` — extracts latitude, longitude, and fix quality

The `nmea_to_deg()` function converts the NMEA `DDMM.MMMM` format to decimal degrees.

---

### Operating Modes

| Mode | Set By | Behavior |
|---|---|---|
| `DEMO_MODE 1` | `#define` | Simulates 24-hour cycle, fixed location, no GPS needed |
| `DEMO_MODE 0` + `USE_GPS_LOCATION 1` | `#define` | Waits for real GPS fix, uses live coordinates |
| `DEMO_MODE 0` + `USE_FIXED_LOCATION 1` | `#define` | Uses hardcoded `FIXED_LATITUDE/LONGITUDE` |
| `CALIBRATION_MODE 1` | `#define` | Steps servos 0→180° in 10° steps with 2s pauses |
| Check Mode | Button press | Tests full servo range, shows position on OLED |
| Night Mode | Auto (elevation < 0) | Returns to sunrise position, OLED shows "NIGHT" |

---

### Telemetry Format

Every tracking cycle, the firmware sends a telemetry line over UART2 (ST-Link virtual COM port). Format:

```
$TELEM,YYYY-MM-DD,HH:MM:SS,LAT(N/S),LON(E/W),AZ,EL,SERVO_AZ,SERVO_EL,V,A,MODE*
```

Example:
```
$TELEM,2026-01-07,10:30:00,10.900000N,76.900000E,147.23,54.88,113.9,54.9,5.12,0.340,DEMO*
```

| Field | Unit | Description |
|---|---|---|
| Date | YYYY-MM-DD | Current date |
| Time | HH:MM:SS | Current time |
| LAT | °N/S | GPS latitude |
| LON | °E/W | GPS longitude |
| AZ | ° | Calculated sun azimuth |
| EL | ° | Calculated sun elevation |
| SERVO_AZ | ° | Actual azimuth servo angle |
| SERVO_EL | ° | Actual elevation servo angle |
| V | V | Solar panel voltage |
| A | A | Solar panel current |
| MODE | string | Operating mode name |

---

## Dashboard Software

### `server.py` — Python WebSocket Server

The server does three things simultaneously:
1. **Reads** the STM32 UART output via `pyserial`
2. **Validates** the firmware's solar position against the NREL SPA C executable
3. **Broadcasts** a JSON payload to all connected browser clients over WebSocket (port 8765)

**Accuracy comparison:**
- If `|delta| < 0.5°` → "excellent"
- If `|delta| < 1.0°` → "good"
- Else → "poor"

### `index.html` — Browser Dashboard

A single-file HTML dashboard that:
- Connects to `ws://localhost:8765`
- Displays all telemetry fields in a formatted UI
- Updates in real time as packets arrive
- Shows SPA comparison and accuracy rating

### `spa.c` / `spa.h` — NREL SPA Reference Library

The official NREL Solar Position Algorithm in C. Used by the dashboard to provide a ground-truth solar position for comparison with the firmware's calculation. An executable wrapper (`main.c` → `spa_calc.exe`) accepts command-line arguments and prints results in `KEY=VALUE` format.
