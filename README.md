# 🌞 Location-Based Dual-Axis Solar Tracker

> **An open-source, GPS-driven smart solar tracker built on the STM32 NUCLEO-F401RE — no light sensors needed.**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform: STM32](https://img.shields.io/badge/Platform-STM32%20NUCLEO--F401RE-blue.svg)](https://www.st.com/en/evaluation-tools/nucleo-f401re.html)
[![Language: C](https://img.shields.io/badge/Language-C%20%2F%20Python-brightgreen.svg)]()
[![Status: Working Prototype](https://img.shields.io/badge/Status-Working%20Prototype-success.svg)]()

---

## 📖 Table of Contents

- [Introduction](#-introduction)
- [Project Overview](#-project-overview)
- [Folder Structure Guide](#-folder-structure-guide)
- [Recommended Reading Flow](#-recommended-reading-flow)
- [License](#-license)

---

## 🔍 Introduction

This project is a **dual-axis solar tracker** — a device that continuously rotates a solar panel to face the sun throughout the day, significantly improving energy yield compared to a fixed panel.

Built as a **Tech Fair 2026 project** by engineering students, it runs on an STM32 NUCLEO-F401RE board.

## 🚀 Project Overview

Unlike traditional solar trackers that use light sensors (LDRs) and get confused by clouds, **this tracker calculates where the sun _should_ be** based on its exact GPS location and time. It uses the National Renewable Energy Laboratory (NREL) Solar Position Algorithm (SPA) to mathematically track the sun, ensuring reliable operation in all weather conditions.

It features:
- **Astronomical Precision:** Math-driven tracking instead of light-chasing.
- **Hardware Integration:** Drives two servos (azimuth and elevation) while monitoring current/voltage via an ADC.
- **Night Return:** Automatically rests at the sunrise angle when the sun sets.
- **Live Telemetry:** Sends live system data to a beautiful Real-Time Python/HTML Dashboard over UART.

---

## 🗂️ Folder Structure Guide

To keep the repository clean and professional, the project is organized into dedicated directories for firmware, hardware, software, and detailed documentation. 

```
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



---



---

## 📜 License

This project is licensed under the **MIT License** — see [LICENSE](LICENSE) for full text.

The NREL Solar Position Algorithm (SPA) code in `dashboard/spa.c` and `dashboard/spa.h` is provided by the **National Renewable Energy Laboratory (NREL)** and is used in accordance with their terms.

---

*Built with ❤️ for EEE Tech Fair 2026*
