# Troubleshooting Guide

This guide covers the most common problems you might encounter and how to fix them.

---

## Hardware Issues

### OLED not displaying anything

| Possible Cause | Fix |
|---|---|
| Wrong I2C address | Check if your OLED uses `0x3C` or `0x3D`. Update `ssd1306.h` |
| Missing pull-up resistors | Add 4.7kΩ from SDA and SCL to 3.3V |
| Loose I2C wires | Reseat PB8 (SCL) and PB9 (SDA) connections |
| OLED powered from wrong pin | Use 3.3V pin, not 5V — SSD1306 is a 3.3V device |

---

### Servos not moving / jittering

| Possible Cause | Fix |
|---|---|
| Powered from STM32 5V pin | Use external 5V ≥2A supply |
| Incorrect PWM frequency | Verify TIM2 prescaler produces 50 Hz (1 MHz / 20000 = 50 Hz) |
| Wrong signal pin | Use PA0 for Servo 1, PA1 for Servo 2 |
| Ground not shared | Connect GND of external supply to NUCLEO GND |

---

### GPS not getting a fix indoors

This is normal. GPS requires line-of-sight to satellites.

- Test near a window or outside
- The firmware uses the hardcoded `FIXED_LATITUDE/LONGITUDE` automatically when no fix is available
- After power-on, allow 30–90 seconds outdoors for the first fix
- Check UART debug output — it will print GPS status messages

---

### UART debug output showing garbage / wrong characters

| Possible Cause | Fix |
|---|---|
| Wrong baud rate in terminal | Set serial monitor to **115200 baud, 8N1** |
| Wrong COM port | Check Device Manager for the ST-Link virtual COM port |
| STM32CubeIDE monitor open | Close CubeIDE's serial view before using another terminal |

---

### Current sensor always reads high or negative

| Possible Cause | Fix |
|---|---|
| Load connected during calibration | Power on with nothing connected to the current sense path |
| ACS712 supply not 5.0V | Verify VCC is exactly 5V — use a multimeter |
| Wrong sensitivity constant | 5A module uses 185mV/A, 20A uses 100mV/A, 30A uses 66mV/A |

---

## Firmware / Software Issues

### STM32CubeIDE build error: "ssd1306.h not found"

The SSD1306 OLED driver header is not automatically included. Add the path:
1. Right-click the project → Properties → C/C++ Build → Settings → Include paths
2. Add `../Core/Inc` (or wherever you placed ssd1306.h)

---

### Firmware flashes successfully but nothing happens

1. Check the OLED — if blank, the code might be stuck in `Error_Handler()`
2. Open serial terminal at **115200 baud** — debug messages appear there
3. Common culprit: I2C bus hanging because a device is not responding. Disconnect OLED or RTC and try again.

---

### Solar position looks wrong / panel points away from sun

1. Verify `FIXED_LATITUDE` and `FIXED_LONGITUDE` match your actual location
2. Confirm `DEFAULT_TIMEZONE` is correct (India = `5.5`, UK = `0`, US Eastern = `-5`)
3. Make sure the DS3231 has the correct time — check UART output for the RTC init line
4. Verify the servo mechanical orientation matches the software's assumption:
   - Servo 1 (Azimuth) 0° = East, 90° = South, 180° = West (northern hemisphere)
   - Servo 2 (Elevation) 0° = Horizon, 90° = Zenith

---

### Python server.py error: "SPA executable not found"

The NREL SPA needs to be compiled first:

```bash
cd dashboard
gcc -o spa_calc spa.c main.c -lm
```

On Windows, use MinGW:
```bash
gcc -o spa_calc.exe spa.c main.c -lm
```

---

### Python server.py error: "No module named serial"

Install the required packages:
```bash
pip install pyserial websockets
```

---

### WebSocket dashboard not updating

1. Confirm `server.py` is running and shows `WebSocket server running on ws://localhost:8765`
2. Check the browser console (F12) for WebSocket connection errors
3. Ensure the STM32 is sending `$TELEM` lines — check in a serial terminal first
4. Try refreshing the browser page

---

## Getting Help

If your issue is not listed here:
1. Check the UART debug output — the firmware prints detailed status at startup
2. Enable `CALIBRATION_MODE 1` to isolate servo issues
3. Open an issue on the GitHub repository with:
   - Your wiring setup (or a photo)
   - The UART debug output from serial monitor
   - Which firmware version you are using (`v5.6` or `v6.0`)
