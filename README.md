# 🌞 Location-Based Dual-Axis Solar Tracker

> **A professional, industrial-grade telemetry and control platform for solar tracking. This project combines high-performance STM32 firmware with astronomical algorithms to provide real-time GPS-driven positioning, active monitoring, and precision servo regulation.**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform: STM32](https://img.shields.io/badge/Platform-STM32%20NUCLEO--F401RE-blue.svg)](https://www.st.com/en/evaluation-tools/nucleo-f401re.html)
[![Language: C](https://img.shields.io/badge/Language-C%20%2F%20Python-brightgreen.svg)]()
[![Status: Working Prototype](https://img.shields.io/badge/Status-Working%20Prototype-success.svg)]()

---

## 📖 Table of Contents

- [Project Overview](#-project-overview)
- [Hardware Specifications](#%EF%B8%8F-hardware-specifications)
- [Control & Safety Features](#-control--safety-features)
- [Dashboard Tech Stack](#-dashboard-tech-stack)
- [Installation & Setup](#-installation--setup)
- [Usage & Telemetry](#-usage--telemetry)
- [Folder Structure Guide](#%EF%B8%8F-folder-structure-guide)
- [Recommended Reading Flow](#%EF%B8%8F-recommended-reading-flow)

---

## 📝 Project Overview

Unlike traditional solar trackers that use light sensors (LDRs) and get confused by clouds, **this system calculates where the sun _should_ be** based on its exact GPS location and time. It uses the National Renewable Energy Laboratory (NREL) Solar Position Algorithm (SPA) to mathematically track the sun, ensuring reliable operation in all weather conditions.

Why this project?
- **Astronomical Precision:** Math-driven tracking instead of light-chasing.
- **Hardware Integration:** Drives two servos (azimuth and elevation) while monitoring current/voltage via an ADC.
- **Night Return:** Automatically rests at the sunrise angle when the sun sets.
- **Live Telemetry:** Sends live system data to a Real-Time Python/HTML Dashboard over UART.

---

## ⚙️ Hardware Specifications

The system is built on a modular architecture consisting of the following core components:

**Control & Processing**
- **Microcontroller:** STM32F401RE (ARM Cortex-M4 @ 84 MHz).
- **Timekeeping:** DS3231 High-Precision Real-Time Clock (RTC) via I2C.
- **Location:** NEO-6M GPS Module for real-time spatial coordinates via UART.
- **Telemetry:** High-speed UART broadcasting for real-time data ingestion.

**Actuation & Positioning**
- **Servos:** 2x MG995 or SG90 for Dual-Axis (Azimuth and Elevation) control.
- **Algorithm Output:** PWM generation mapped to 0–180° rotation limits.

**Sensing & Feedback**
- **Current Sensing:** ACS712 Hall-Effect sensor for RMS current measurement on the PV output.
- **Voltage Sensing:** Precision Resistor Voltage Divider to safely monitor PV voltage.
- **Display:** SSD1306 OLED (128x64) for localized status and parameter readouts.

---

## ✨ Control & Safety Features

- **Astronomical SPA Integration:** Implements Julian Day rendering and atmospheric refraction calculation to output exact solar positioning regardless of environmental light conditions.
- **Adaptive Smooth Actuation:** Step-looped rotational tracking with programmable delay insertion to prevent mechanical shock and high inrush currents on servo movement.
- **Intelligent Night Mode:** Automatically disables active tracking and regressions the array to sunrise coordinates when solar elevation drops below 0°.
- **System Calibration Mode:** Hardcoded step-through testing sequence at 10° intervals for precise initial calibration and mechanical limit verification.
- **Integrated Power Monitoring:** Real-time ADC polling to monitor generated power and voltage levels continuously.

---

## 💻 Dashboard Tech Stack

The Solar Tracker frontend is a lightweight telemetry dashboard designed for real-time data visualization via local WebSocket tunneling.

- **Backend Bridge:** Python 3 `websockets` and `pyserial` server to parse continuous `$TELEM` packets.
- **Frontend Markup:** Responsive HTML/JS UI.
- **Visualization:** Live numerical and state-based readouts (Azimuth, Elevation, Voltage, Current, Algorithm Accuracy Delta).
- **Validation Engine:** Built-in compiled NREL SPA logic to validate the microcontroller's math identically in real-time.

---

## 🚀 Installation & Setup

### Prerequisites
- [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html)
- Python 3.8+ (for dashboard)
- A connected STM32 NUCLEO-F401RE board

### Step 1: Clone the Repository
```bash
git clone https://github.com/patnana-tarun/Location_Based_Solar_Tracker.git
cd Location_Based_Solar_Tracker
```

### Step 2: Flash the Firmware
1. Open **STM32CubeIDE**.
2. Import the project from the `firmware/` directory.
3. Open `firmware/src/main_v6.c` and adjust `FIXED_LATITUDE` and `FIXED_LONGITUDE` if desired.
4. Build and flash the code to the STM32.

### Step 3: Launch the Dashboard
```bash
cd dashboard
pip install pyserial websockets
python server.py COM8  # Use your specific COM port (e.g., /dev/ttyUSB0 on Linux)
```
Then, exactly double-click `index.html` (or open it your favorite web browser) to view the live dashboard.

---

## 🛠️ Usage & Telemetry

**Data Output Format (STM32 to Dashboard)**
The dashboard expects a comma-separated `$TELEM` string from the STM32 via UART:
`$TELEM,YYYY-MM-DD,HH:MM:SS,LAT(N/S),LON(E/W),AZ,EL,SERVO_AZ,SERVO_EL,V,A,MODE*`

- **AZ/EL:** Calculated Astronomical Azimuth and Elevation
- **SERVO_AZ/EL:** Actual driven PWM servo angles
- **V/A:** Solar Panel Voltage and Current
- **MODE:** Current operational state (TRACKING, NIGHT, CALIBRATING)

---

## 🗂️ Folder Structure Guide

To keep the repository clean and professional, the project is organized into dedicated directories for firmware, hardware, software, and detailed documentation. 

```text
solar-tracker/
│
├── docs/                        ← Detailed project documentation
│   ├── project-overview.md      ← Deep-dive on why and how the tracker works
│   ├── hardware-architecture.md ← Explanation of electronics and BOM
│   ├── software-architecture.md ← Explanation of the firmware algorithms
│   ├── wiring-diagram.md        ← STM32 pin connections guide
│   ├── calibration.md           ← How to tune the servos and sensors
│   ├── troubleshooting.md       ← Common issues and solutions
│   └── reference_materials/     ← Original project reports & reviews (.docx/.pdf)
│
├── hardware/                    ← Physical build resources
│   ├── components-list.md       ← Comprehensive bill of materials (BOM)
│   ├── mechanical/              ← 3D models and chassis designs
│   ├── pcb/                     ← Custom board layouts
│   └── wiring/                  ← Circuit diagrams
│
├── firmware/                    ← STM32 Embedded C Code
│   ├── src/                     ← Main codebase (main_v6.c, driver files)
│   ├── config/                  ← IDE configurations
│   ├── stm32_project/           ← Complete STM32CubeIDE project workspace
│   └── README.md                ← Instructions on flashing the microcontroller
│
├── dashboard/                   ← PC Telemetry Software
│   ├── server.py                ← Python WebSocket & UART bridge
│   ├── index.html               ← Real-time browser UI
│   ├── spa.c / spa.h            ← Reference calculation files
│   └── README.md                ← Instructions for starting the dashboard
│
├── images/                      ← Visual resources for the docs
├── simulations/                 ← Math models or testing scripts
├── examples/                    ← Usage examples and logs
└── data/                        ← Stored telemetry data
```

---

## 🗺️ Recommended Reading Flow

If you are seeing this repository for the first time, we recommend following this path to understand everything quickly:

1. **[docs/project-overview.md](docs/project-overview.md):** Read this first. It explains the core concepts and the fundamental difference between GPS-based and Sensor-based tracking.
2. **[docs/hardware-architecture.md](docs/hardware-architecture.md):** Understand what components make up the physical device.
3. **[docs/software-architecture.md](docs/software-architecture.md):** Learn how the math in the firmware (the Solar Position Algorithm) operates.
4. **[docs/wiring-diagram.md](docs/wiring-diagram.md):** See how the STM32, servos, GPS, and RTC connect together.
5. **[firmware/README.md](firmware/README.md):** Follow these steps to compile and flash the code to your NUCLEO board.
6. **[dashboard/README.md](dashboard/README.md):** Learn how to launch the live telemetry visualization on your PC.

---

## 📜 License

This project is licensed under the **MIT License** — see [LICENSE](LICENSE) for full text.

The NREL Solar Position Algorithm (SPA) code in `dashboard/spa.c` and `dashboard/spa.h` is provided by the **National Renewable Energy Laboratory (NREL)** and is used in accordance with their terms.

---

*Built with ❤️ for EEE Tech Fair 2026*
