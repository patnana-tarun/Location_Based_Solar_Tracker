# Component List — Bill of Materials (BOM)

This is the complete list of components needed to build the solar tracker from scratch.

---

## Main Components

| # | Component | Specification | Qty | Purpose | Approx. Cost (INR) |
|---|---|---|---|---|---|
| 1 | **STM32 NUCLEO-F401RE** | ARM Cortex-M4, 84 MHz, 512KB Flash | 1 | Main microcontroller | ₹700–800 |
| 2 | **NEO-6M GPS Module** | 9600 baud UART, NMEA 0183 | 1 | Location and time source | ₹200–300 |
| 3 | **DS3231 RTC Module** | I2C, ±2 ppm accuracy, CR2032 battery | 1 | Real-time clock | ₹100–150 |
| 4 | **SSD1306 OLED Display** | 128×64 pixels, I2C, 0.96 inch | 1 | Status display | ₹150–200 |
| 5 | **Servo Motor #1 (Azimuth)** | MG995 (180°) or SG90 (lightweight) | 1 | Left/right panel rotation | ₹150–300 |
| 6 | **Servo Motor #2 (Elevation)** | MG995 (180°) or SG90 | 1 | Up/down panel tilt | ₹150–300 |
| 7 | **ACS712 Current Sensor** | 5A range, ±2.5V output | 1 | Panel current monitoring | ₹100–150 |
| 8 | **Resistor — 100kΩ** | ¼W, 1% tolerance | 1 | Voltage divider (top) | ₹5 |
| 9 | **Resistor — 20kΩ** | ¼W, 1% tolerance | 1 | Voltage divider (bottom) | ₹5 |
| 10 | **Small Solar Panel** | 5V–6V, 50–500mA | 1 | Demonstration load | ₹150–300 |
| 11 | **5V Power Supply** | ≥2A output, regulated | 1 | Powers servos + board | ₹150–200 |
| 12 | **Push Button (Momentary)** | Through-hole or breadboard | 1 | Check Mode trigger | ₹10–20 |
| 13 | **Breadboard** | 830-point or larger | 1 | Prototyping | ₹80–120 |
| 14 | **Jumper Wires** | Male-to-male and male-to-female | 1 set | Wiring | ₹50–80 |
| 15 | **CR2032 Battery** | 3V coin cell | 1 | RTC backup | ₹20–30 |

**Estimated Total: ₹2,000–3,000 (complete kit)**

> 💡 If you already have the NUCLEO board, everything else costs around ₹1,200–1,800.

---

## Component Descriptions and Specs

### STM32 NUCLEO-F401RE
- **Microcontroller:** STM32F401RE (ARM Cortex-M4)
- **Clock:** Up to 84 MHz
- **Flash:** 512 KB, SRAM: 96 KB
- **Peripherals used:** TIM2 (PWM), ADC1, I2C1, USART1, USART2, GPIO
- **Built-in:** ST-Link debugger/programmer, USB virtual COM port
- **Voltage:** 3.3V I/O logic

### NEO-6M GPS Module
- **Protocol:** UART, 9600 baud
- **Output:** NMEA 0183 sentences ($GPRMC, $GPGGA)
- **Channels:** 50 (can track 50 satellites)
- **Position accuracy:** ±2.5m CEP
- **Time accuracy:** 1µs (synchronized to GPS time)
- **Cold start time:** 27s typical, up to 90s
- **Operating voltage:** 3.3V (some modules have on-board regulators for 5V input)

### DS3231 RTC Module
- **Interface:** I2C (address 0x68)
- **Accuracy:** ±2 ppm (~1 minute/year)
- **Temperature-compensated oscillator:** Yes
- **Backup power:** CR2032 coin cell (holds time for years)
- **Operating voltage:** 3.3V–5.5V (3.3V recommended)

### SSD1306 OLED Display
- **Resolution:** 128 × 64 pixels (monochrome)
- **Interface:** I2C (address 0x3C or 0x3D)
- **Size:** 0.96 inch diagonal
- **Operating voltage:** 3.3V
- **Current draw:** ~10mA typical

### MG995 Servo Motor
- **Torque:** 9.4 kg·cm (4.8V), 11 kg·cm (6V)
- **Operating speed:** 0.17 s/60° (4.8V)
- **Angle range:** 0–180°
- **PWM frequency:** 50 Hz
- **Pulse range:** 500µs – 2500µs
- **Operating voltage:** 4.8V – 6V
- **Use for:** heavier panel mounts, outdoor builds

### SG90 Servo Motor (Lightweight alternative)
- **Torque:** 1.8 kg·cm (4.8V)
- **Angle range:** 0–180°
- **Use for:** lightweight panels, indoor demos, prototypes

### ACS712 Current Sensor (5A)
- **Output:** Analog voltage (2.5V = 0A, sensitivity 185mV/A)
- **Range:** ±5A
- **Bandwidth:** 80 kHz
- **Operating voltage:** 5V

### Voltage Divider Network
- **R1:** 100kΩ (from panel+ to ADC pin)
- **R2:** 20kΩ (from ADC pin to GND)
- **Scale factor:** ×5 → max measurable voltage = 16.5V

---

## Optional / Upgrade Components

| Component | Purpose |
|---|---|
| ESP8266 Wi-Fi module | Wireless UART bridge for dashboard |
| SD card module (SPI) | Log telemetry data locally |
| 18650 Li-ion cell + TP4056 charger | Power the tracker from the solar panel |
| 3D-printed mount | Professional enclosure for servos and panel |
| Custom PCB | Replace breadboard wiring with soldered board |

---

## Where to Buy (India)

- **Robu.in** — good range of servos, sensors, displays
- **Evelta Electronics** — STM32 boards and modules
- **Amazon.in** — GPS, RTC, OLED modules with next-day delivery
- **AliExpress** — cheapest prices if lead time allows
