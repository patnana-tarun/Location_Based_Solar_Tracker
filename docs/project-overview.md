# Project Overview — Location-Based Dual-Axis Solar Tracker

## What Problem Does This Solve?

Solar panels generate the most electricity when they directly face the sun. A fixed panel loses 30–50% of its potential energy because the sun moves across the sky throughout the day.

A **solar tracker** mechanically follows the sun, keeping the panel pointed directly at it. This project implements a dual-axis tracker (two degrees of freedom: left/right and up/down) controlled by a microcontroller that knows exactly where the sun is using **GPS and astronomical math**.

---

## Background and Motivation

This project was built for **EEE Tech Fair 2026** to demonstrate:
1. A practical application of embedded systems in renewable energy
2. An alternative to the common LDR-based approach
3. Integration of GPS, RTC, OLED, servo motors, ADC, and a PC dashboard into one system

---

## How Traditional Trackers Work (and Why We Didn't Do That)

Traditional solar trackers use **Light Dependent Resistors (LDRs)**:
- Four LDR sensors are placed at the corners of the panel
- The microcontroller compares readings and moves the panel toward the brighter side
- This is a **closed-loop feedback system**

**Problem with LDRs:**
- On cloudy or partly cloudy days, the sensors are fooled by diffuse light
- The system can oscillate ("hunt") when light is roughly equal on both sides
- Shadows from nearby objects confuse the sensors
- No memory — if the panel is moved to the wrong position it can't self-correct without light

**Our Approach:**
- Calculate the sun's exact azimuth and elevation using GPS coordinates + current time
- Drive the motors to the calculated position directly (open-loop with astronomical precision)
- Works in overcast conditions — the panel moves to where the sun IS, not where it's brightest
- Night mode and automatic dawn recovery built in

---

## Academic Context

This type of tracker is called a **predictive** or **model-based** solar tracker. It is more commonly used in commercial and utility-scale solar installations. The algorithm we use is based on the **Solar Position Algorithm (SPA)** developed by Ibrahim Reda and Afshin Andreas at the **National Renewable Energy Laboratory (NREL)** in 2003, which is the industry standard for solar position calculations.

References:
- Reda, I., Andreas, A. (2003). *Solar Position Algorithm for Solar Radiation Applications*. NREL/TP-560-34302.
- Mousazadeh, H. et al. (2009). *A review of principle and sun-tracking methods for maximizing solar systems output*. Renewable and Sustainable Energy Reviews.

---

## Project Timeline

| Milestone | Description |
|---|---|
| v1–v4 | Early prototypes, sensor testing, basic servo control |
| v5.6 | Full calibration mode, OLED working, GPS integration |
| v6.0 | Final version — RTC init from build time, Check Mode, unified OLED display, structured UART telemetry |
| Dashboard | Python WebSocket server + HTML UI with SPA cross-validation |

---

## What This Project Demonstrates

- Reading and parsing GPS NMEA sentences in bare-metal C
- I2C communication with DS3231 RTC and SSD1306 OLED
- PWM servo control with smooth stepping via TIM2
- 12-bit ADC for voltage and current measurement with averaging
- Astronomical computation (Julian Day, Hour Angle, Declination) on an embedded processor
- Structured UART telemetry protocol (`$TELEM` format)
- Python asyncio WebSocket server
- Real-time browser dashboard
