# Usage Example — Sample Telemetry Output and Dashboard

This file shows what the system looks like in operation, including sample UART output and a description of the dashboard.

---

## Sample UART Output at Startup

Connect a serial terminal at **115200 baud** and press reset on the NUCLEO:

```
╔═══════════════════════════════════════════════╗
║  SOLAR TRACKER v6.0 - ENHANCED VERSION       ║
║  RTC Init + Location Control + Check Mode    ║
║  NUCLEO-F401RE @ 16 MHz HSI                  ║
╚═══════════════════════════════════════════════╝

Setting RTC from build time...
   RTC: 07/01/2026 10:00:00

Initializing Servos...
   Servos ready

Initializing ADC...
   ADC ready

Calibrating current sensor (disconnect load)...
   Zero offset: 0.0023A

Initializing OLED...
   OLED ready

Starting GPS receiver...
   GPS active

═══════════════════════════════════════════════
     DEMO MODE: Fixed Location                 
═══════════════════════════════════════════════
```

---

## Sample $TELEM Packets

After startup, the firmware sends one telemetry line per tracking cycle:

```
$TELEM,2026-01-07,06:00:00,10.900000N,76.900000E,109.45,2.15,19.5,2.2,5.10,0.120,DEMO*
$TELEM,2026-01-07,07:00:00,10.900000N,76.900000E,101.23,15.42,11.2,15.4,5.18,0.210,DEMO*
$TELEM,2026-01-07,08:00:00,10.900000N,76.900000E,93.44,28.78,3.4,28.8,5.21,0.280,DEMO*
$TELEM,2026-01-07,09:00:00,10.900000N,76.900000E,85.11,42.03,0.0,42.0,5.25,0.310,DEMO*
$TELEM,2026-01-07,12:00:00,10.900000N,76.900000E,173.88,67.12,83.9,67.1,5.50,0.455,DEMO*
$TELEM,2026-01-07,15:00:00,10.900000N,76.900000E,243.21,48.34,153.2,48.3,5.30,0.390,DEMO*
$TELEM,2026-01-07,17:00:00,10.900000N,76.900000E,263.54,18.22,173.5,18.2,5.11,0.185,DEMO*
$TELEM,2026-01-07,18:00:00,10.900000N,76.900000E,271.02,4.88,0.0,0.0,4.80,0.090,NIGHT*
```

### Understanding the Fields

```
$TELEM, 2026-01-07, 12:00:00, 10.90N, 76.90E, 173.88, 67.12, 83.9, 67.1, 5.50, 0.455, DEMO*
         └ date     └ time    └lat    └lon    └sun_az └sun_el └sv_az └sv_el └V   └A    └mode
```

| Field | Meaning |
|---|---|
| `2026-01-07` | Date |
| `12:00:00` | Time |
| `10.90N` | Latitude 10.90° North |
| `76.90E` | Longitude 76.90° East |
| `173.88` | Sun's calculated azimuth (°) |
| `67.12` | Sun's calculated elevation (°) |
| `83.9` | Servo 1 (Azimuth) actual angle (°) |
| `67.1` | Servo 2 (Elevation) actual angle (°) |
| `5.50` | Solar panel voltage (V) |
| `0.455` | Solar panel current (A) |
| `DEMO` | Operating mode |

---

## OLED Display Layout

At solar noon, the OLED shows:

```
┌──────────────────┐
│07/01/2026 12:00  │   ← Date and time from RTC
│10.90N  76.90E    │   ← GPS latitude and longitude
│Az:84  El:67      │   ← Current servo angles
│DEMO TRACK        │   ← Mode string
└──────────────────┘
```

---

## Starting the Dashboard

```bash
# Install dependencies (first time only)
pip install pyserial websockets

# Run the server (replace COM8 with your serial port)
cd dashboard
python server.py COM8

# Output when running:
# ═══════════════════════════════════════════
#    Solar Tracker WebSocket Server v1.0
#    Using Compiled NREL SPA Executable
# ═══════════════════════════════════════════
# ✓ Using SPA executable: ./spa_calc.exe
# ✓ WebSocket server running on ws://localhost:8765
# ✓ Connected to COM8
# ✓ Client connected (1 total)
```

Then open `dashboard/index.html` in your browser.

---

## Sample JSON Payload (WebSocket)

Each packet received by the browser looks like this:

```json
{
  "datetime": { "year": 2026, "month": 1, "day": 7, "hour": 12, "minute": 0, "second": 0 },
  "location":  { "latitude": 10.9, "longitude": 76.9, "ns": "N", "ew": "E", "source": "FIXED" },
  "firmware":  { "azimuth": 173.88, "elevation": 67.12 },
  "servo":     { "azimuth": 83.9,   "elevation": 67.1  },
  "power":     { "voltage": 5.50,   "current": 0.455   },
  "mode":      "DEMO",
  "spa":       { "azimuth": 174.01, "elevation": 67.08, "zenith": 22.92 },
  "delta":     { "azimuth": -0.13,  "elevation": 0.04,  "accuracy": "excellent" }
}
```

The `delta` field shows how close the firmware's calculation is to the NREL SPA reference. An `accuracy` of "excellent" means the error is less than 0.5°.

---

## Demo Mode Tips (Tech Fair / Presentation)

- Set `DEMO_MODE 1` in `main.c` — no GPS or RTC needed
- The demo simulates a 24-hour cycle at your hardcoded location
- Each "hour" advances after `DEMO_DELAY_MS` milliseconds (default 3000ms = 3 seconds per simulated hour)
- The panel will visibly sweep from east to west during the demo
- OLED and dashboard update throughout
- At "sunset" (hour 18), the panel returns to the sunrise position automatically
