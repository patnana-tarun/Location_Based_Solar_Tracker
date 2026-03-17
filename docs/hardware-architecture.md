# Hardware Architecture

This document describes the hardware design of the solar tracker — what each component does, why it was chosen, and how the pieces connect together.

---

## Block Diagram

```
                        ┌──────────────────────────────┐
                        │   STM32 NUCLEO-F401RE         │
                        │   (Cortex-M4, 84 MHz)         │
                        │                              │
 [NEO-6M GPS] ──UART1──►│ PA10 (RX)                    │
                        │                              │
 [DS3231 RTC] ──I2C1───►│ PB8 (SCL) PB9 (SDA)          │
 [SSD1306 OLED]─I2C1───►│                              │
                        │                              │
 [Solar Panel V]─ADC───►│ PA4 (ADC1_CH4)               │──PWM──► [Servo 1 - Azimuth]
 [ACS712 I_out]─ADC───►│ PA5 (ADC1_CH5)               │──PWM──► [Servo 2 - Elevation]
                        │                              │
 [PC / ST-Link] ─UART2─►│ PA2 (TX), PA3 (RX)           │
 [Check Button] ─GPIO──►│ PC13                         │
                        └──────────────────────────────┘
```

---

## Components In Detail

### 1. STM32 NUCLEO-F401RE (Main Microcontroller)

- **Core:** ARM Cortex-M4 at 84 MHz
- **Flash:** 512 KB, **SRAM:** 96 KB
- **Why chosen:** Widely available, affordable (~$15), supported by free STM32CubeIDE, has enough timers and UART ports for all peripherals
- **Built-in features used:** TIM2 for PWM, ADC1, I2C1, USART1, USART2, GPIO

---

### 2. NEO-6M GPS Module

- **Protocol:** UART at 9600 baud
- **Output:** NMEA 0183 sentences (GPRMC, GPGGA)
- **Accuracy:** ±2.5m position, ±1s time (after cold fix)
- **Why used:** GPS provides exact latitude, longitude, and UTC time — all three are needed for the solar position algorithm
- **Connection:** TX pin → NUCLEO PA10 (USART1 RX)
- **Note:** Cold start fix takes 30–90 seconds. The firmware falls back to a hardcoded location until a GPS fix is acquired.

**NMEA Sentences Parsed:**
- `$GPRMC` — Position, velocity, date, valid flag
- `$GPGGA` — Position, fix quality, number of satellites

---

### 3. DS3231 RTC Module

- **Protocol:** I2C at 400 kHz
- **Address:** 0x68
- **Accuracy:** ±2 ppm (about ±1 minute per year)
- **Backup battery:** Holds time indefinitely when board is powered off
- **Why used:** The solar algorithm needs accurate time. GPS time can take minutes to acquire. The RTC provides instant time on boot.
- **Initialization:** On first flash, the firmware sets the RTC from the compiler's `__DATE__` and `__TIME__` macros

---

### 4. SSD1306 OLED Display (128×64, I2C)

- **Protocol:** I2C (shared bus with DS3231)
- **Driver library:** Custom `ssd1306.c` / `ssd1306.h`
- **Display layout (4 lines at 16px spacing):**
  - Line 0: Date and time (DD/MM/YYYY HH:MM)
  - Line 1: GPS coordinates (LAT, LON)
  - Line 2: Azimuth and elevation (Az: XX El: XX)
  - Line 3: Mode string (DEMO / NIGHT / GPS TRACK / CHECK MODE)
- **Why needed:** Allows the user to view system status without connecting to a PC

---

### 5. Servo Motors (×2)

- **Type:** Standard PWM servo (MG995 recommended for structural use, SG90 for lightweight)
- **Control signal:** 50 Hz PWM
  - 500µs pulse → 0°
  - 1500µs pulse → 90°
  - 2500µs pulse → 180°
- **Timer:** TIM2 on STM32 (PA0 = CH1 = Azimuth, PA1 = CH2 = Elevation)
- **Servo 1 (Azimuth):** Rotates the panel horizontally — covers east to west
- **Servo 2 (Elevation):** Tilts the panel vertically — covers horizon to zenith
- **Angle ranges:**
  - Azimuth: 0–180° (maps to compass 90°–270°)
  - Elevation: 0–90°

**Smooth Movement:** The firmware moves servos in 1° steps with 40ms between each step. This prevents mechanical shock and reduces current spikes.

---

### 6. ACS712 Current Sensor (5A range)

- **Output:** Analog voltage, centered at 2.5V (zero current)
- **Sensitivity:** 185 mV/A
- **Connection:** Output → PA5 (ADC1 Channel 5)
- **Calibration:** At startup, the firmware samples 50 readings × 50 averages to determine the zero-current offset, correcting for sensor bias
- **Formula:** `current = (adc_voltage - 2.5V) / 0.185`

---

### 7. Voltage Divider (Solar Panel Voltage)

- **Components:** Two resistors — 100 kΩ (top) + 20 kΩ (bottom)
- **Scale factor:** 5× (allows measuring up to ~16.5V with 3.3V ADC)
- **Connection:** Midpoint → PA4 (ADC1 Channel 4)
- **Formula:** `panel_voltage = adc_voltage × 5.0`
- **Sampling:** 100 ADC readings averaged to reduce noise

---

### 8. Push Button (Check Mode)

- **Connection:** PC13 → Ground (active low, internal pull-up)
- **Function:** Triggers the Check Mode servo test sequence
  - Press once → servos move through: 0/0 → 180/0 → 180/90 → 90/90
  - Press again → exit Check Mode
  - Auto-timeout: 30 seconds

---

## Power Requirements

| Component | Typical Current |
|---|---|
| STM32 NUCLEO | ~100 mA |
| NEO-6M GPS | ~45 mA (acquisition), ~20 mA (tracking) |
| DS3231 RTC | < 1 mA |
| SSD1306 OLED | ~10 mA |
| SG90 Servo (×2) | ~100–300 mA each under load |
| ACS712 Sensor | ~10 mA |
| **Total (peak)** | **~800 mA @ 5V** |

> ⚠️ **Use a 5V, 2A power supply.** USB power banks with fast-charge may work but standard PC USB ports (500mA) are not sufficient when servos are under load.

---

## Safety Considerations

- **Servo stall:** Avoid commanding a servo to push against a physical end-stop. Set software angle limits (`AZIMUTH_MAX_ANGLE`, `ELEVATION_MAX_ANGLE`) before running.
- **Current sense disconnection:** Always calibrate the current sensor before connecting the solar panel load. The firmware performs auto-calibration at power-on.
- **Short circuit:** The voltage divider only measures; it does not power anything. Ensure correct resistor values to avoid exceeding the ADC's 3.3V input limit.
- **Cable management:** Servo cables should have enough slack to rotate the full range without pulling connections loose.
