# Firmware — Build and Flash Guide

This folder contains all the STM32 firmware source code for the Location-Based Solar Tracker.

---

## Which Firmware Version?

| File | Version | Description |
|---|---|---|
| `src/main.c` | **v6.0 (use this)** | Final version with Check Mode, unified OLED display, structured telemetry, compile-time RTC init |
| `src/main_v56_calibration.c` | v5.6 | Earlier version with calibration mode and 10° step testing. Use this to verify servo mechanicals. |

> **Start with `src/main.c` (v6.0) for normal use.**  
> Use `src/main_v56_calibration.c` if you need the more detailed calibration sweep.

---

## Prerequisites

1. Install **[STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html)** (free, includes GCC compiler + debugger)
2. Install **STM32CubeF4 package** (STM32CubeIDE will prompt you on first use)
3. Connect the NUCLEO board to your PC with a Micro-USB cable (use the ST-Link USB port, not the morpho connector)

---

## Importing the Project

1. Open STM32CubeIDE
2. Go to `File → Import → General → Existing Projects into Workspace`
3. Browse to the `firmware/` folder
4. Select the project and click **Finish**

---

## Configuration (Before Building)

Open `firmware/src/main.c` and find the configuration section at the top of the file. Adjust these values for your location and use case:

```c
/* =================== DEMO MODE CONFIGURATION =================== */
#define DEMO_MODE 1              // Set to 0 for real tracking, 1 for demo/presentation

/* =================== LOCATION SOURCE (COMPILE-TIME) =================== */
// In DEMO_MODE=0, choose ONE:
#define USE_FIXED_LOCATION 1     // Use hardcoded lat/lon below
#define USE_GPS_LOCATION   0     // Use live GPS (requires NEO-6M module)

#define FIXED_LATITUDE  10.90    // Your latitude (positive = North)
#define FIXED_LONGITUDE 76.90    // Your longitude (positive = East)
#define FIXED_LAT_HEMI 'N'       // 'N' or 'S'
#define FIXED_LON_HEMI 'E'       // 'E' or 'W'

/* =================== NIGHT MODE CONFIGURATION =================== */
#define SUNRISE_HOUR 6           // Solar tracking starts at 6 AM
#define SUNSET_HOUR  18          // Solar tracking stops at 6 PM

/* =================== SMOOTH MOVEMENT =================== */
#define SMOOTH_STEP_SIZE  1.0f   // Degrees per step (smaller = smoother but slower)
#define SMOOTH_STEP_DELAY 40     // Milliseconds between steps

/* =================== DEFAULT TIMEZONE =================== */
#define DEFAULT_TIMEZONE 5.5f    // India = 5.5, UK = 0, US Eastern = -5
```

---

## Building the Firmware

1. Right-click the project in the Project Explorer
2. Click **Build Project**
3. Wait for `Build Finished` in the Console tab (should show 0 errors)

---

## Flashing to the Board

1. Connect the NUCLEO board via USB (ST-Link port)
2. Click the **Run** button (green play icon) or press `F11`
3. STM32CubeIDE will flash the firmware and start debugging
4. The OLED should show `Solar Tracker / v6.0 Ready` within a few seconds

**Alternative (command line):**
```bash
# Using OpenOCD (if installed separately)
openocd -f board/st_nucleo_f4.cfg -c "program Debug/firmware.elf verify reset exit"
```

---

## Serial Monitor / UART Debug

Connect a serial terminal to the NUCLEO's virtual COM port (appears as `COMx` on Windows, `/dev/ttyACM0` on Linux):

- **Baud rate:** 115200
- **Data bits:** 8
- **Parity:** None
- **Stop bits:** 1

The firmware prints detailed startup messages and `$TELEM` data every tracking cycle.

**Recommended terminals:**
- [PuTTY](https://www.putty.org/) (Windows)
- [Tera Term](https://ttssh2.osdn.jp/) (Windows)
- `screen /dev/ttyACM0 115200` (Linux/Mac)

---

## Required External Libraries

### SSD1306 OLED Driver (Included)

The `ssd1306.c` and `ssd1306.h` files provide:
- `SSD1306_Init(I2C_HandleTypeDef *i2c)` — Initialize the display
- `SSD1306_Clear()` — Clear the display buffer
- `SSD1306_DrawString(x, y, str)` — Draw text at a position
- `SSD1306_UpdateScreen()` — Push buffer to display

Place these in `firmware/src/` and add the path to your include directories in STM32CubeIDE.

---

## Common Compile Errors and Fixes

| Error | Fix |
|---|---|
| `ssd1306.h: No such file or directory` | Add `firmware/src` to include paths in project properties |
| `undefined reference to 'SSD1306_Init'` | Add `ssd1306.c` to the build (right-click → Add to Build) |
| `math.h function undefined` | Add `-lm` to linker flags (Project → Properties → C/C++ Build → Settings → Linker → Libraries) |

---

## Folder Structure

```
firmware/
├── src/
│   ├── main.c                    ← v6.0 final firmware
│   ├── main_v56_calibration.c    ← v5.6 calibration firmware
│   ├── ssd1306.c                 ← OLED driver implementation
│   └── ssd1306.h                 ← OLED driver header
└── config/
    └── STM32F401RETX_FLASH.ld    ← Linker script (flash execution)
```
