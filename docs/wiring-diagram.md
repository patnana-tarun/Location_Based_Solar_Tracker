# Wiring Guide

This document provides the complete pin-by-pin wiring table for the solar tracker, plus safety tips.

---

## Quick Reference Pin Table

| Component | Component Pin | NUCLEO-F401RE Pin | Notes |
|---|---|---|---|
| **NEO-6M GPS** | TX | PA10 (USART1 RX) | 9600 baud |
| **NEO-6M GPS** | GND | GND | |
| **NEO-6M GPS** | VCC | 3.3V or 5V | Check your module's rating |
| **DS3231 RTC** | SCL | PB8 (I2C1_SCL) | 4.7kΩ pull-up to 3.3V |
| **DS3231 RTC** | SDA | PB9 (I2C1_SDA) | 4.7kΩ pull-up to 3.3V |
| **DS3231 RTC** | VCC | 3.3V | |
| **DS3231 RTC** | GND | GND | |
| **SSD1306 OLED** | SCL | PB8 (I2C1_SCL) | Shared bus with RTC |
| **SSD1306 OLED** | SDA | PB9 (I2C1_SDA) | Shared bus with RTC |
| **SSD1306 OLED** | VCC | 3.3V | |
| **SSD1306 OLED** | GND | GND | |
| **Servo 1 (Azimuth)** | Signal | PA0 (TIM2_CH1) | 50 Hz PWM |
| **Servo 1 (Azimuth)** | VCC | External 5V | Do NOT power from NUCLEO |
| **Servo 1 (Azimuth)** | GND | GND (common) | |
| **Servo 2 (Elevation)** | Signal | PA1 (TIM2_CH2) | 50 Hz PWM |
| **Servo 2 (Elevation)** | VCC | External 5V | Do NOT power from NUCLEO |
| **Servo 2 (Elevation)** | GND | GND (common) | |
| **ACS712 Current** | OUT | PA5 (ADC1_CH5) | Analog voltage output |
| **ACS712 Current** | VCC | 5V | |
| **ACS712 Current** | GND | GND | |
| **Voltage Divider** | Midpoint | PA4 (ADC1_CH4) | 100kΩ + 20kΩ to panel |
| **Push Button** | Pin 1 | PC13 | |
| **Push Button** | Pin 2 | GND | Uses internal pull-up |
| **PC (Debug/Telemetry)** | USB | CN1 (ST-Link USB) | UART2 on PA2/PA3 |

---

## I2C Bus Note

The DS3231 RTC and SSD1306 OLED share the same I2C1 bus (PB8/PB9). This works because they have different I2C addresses:
- DS3231: `0x68`
- SSD1306: `0x3C`

Both devices must have **4.7kΩ pull-up resistors** from SDA and SCL to 3.3V. Many breakout boards already include these. If your OLED or RTC module does not have them built in, add them externally.

---

## Servo Power Note

> ⚠️ **Critical:** Servos draw significant current under load (100–500mA each). Do NOT power them from the NUCLEO's 5V or 3.3V pins. Use a dedicated 5V power supply.

Connect the power supply and the NUCLEO to a **common ground**. The servo signal wire connects to the STM32 GPIO pin only (signal is 3.3V logic, which works with standard servos).

---

## Voltage Divider for Panel Voltage Measurement

Build a simple voltage divider with two resistors:

```
Solar Panel (+) ─── 100kΩ ─── PA4 (ADC) ─── 20kΩ ─── GND
```

This divides the panel voltage by 5. So a 15V panel appears as 3.0V at the ADC (within the 3.3V limit).

Maximum measurable voltage = 3.3V × 5 = **16.5V**

---

## GPS Module Note

The NEO-6M GPS module requires a **clear view of the sky** for a valid fix. Indoors, you will likely not get a fix. The firmware handles this gracefully — it uses the hardcoded fallback location until a valid GPS reading arrives.

After power-on, allow **30–90 seconds** for the first fix. Subsequent fixes (warm start) are much faster.

---

## Wiring Diagram (ASCII Schematic)

```
                      ┌─────────────────────────────────┐
External 5V ─────────► VCC (servos)                    │
                      │                                 │
                      │  NUCLEO-F401RE                  │
             PA0 ────►  Sig ─ [Servo 1 Azimuth]        │
             PA1 ────►  Sig ─ [Servo 2 Elevation]      │
                      │                                 │
           PA10 ──────  ◄─ TX [NEO-6M GPS]             │
                      │                                 │
    PB8 (SCL) ────────  ─ SCL ─┬─ [DS3231]             │
    PB9 (SDA) ────────  ─ SDA ─┘   [SSD1306 OLED]      │
                      │                                 │
     PA4 ─── midpoint ─ [100k─V_panel] [20k─GND]       │
     PA5 ◄─ OUT ─ [ACS712] ─ VCC=5V                    │
                      │                                 │
    PC13 ◄── [Button] ─── GND                          │
                      └─────────────────────────────────┘
```
