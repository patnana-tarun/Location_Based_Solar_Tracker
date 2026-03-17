# Solar Tracker Real-Time Dashboard

This is the Python-based real-time telemetry dashboard for the smart solar tracker. It reads `$TELEM` packets from the STM32 via USB/UART and hosts a local WebSocket server to stream the live data to a web browser interface.

## Requirements
- Python 3.8+
- `pyserial` and `websockets` libraries

## Execution Instructions

1. **Install dependencies:**
   Open your terminal in this directory and run:
   ```bash
   pip install pyserial websockets
   ```

2. **Start the server:**
   Connect your STM32 or FTDI adapter to your PC. Identify the COM port (e.g., `COM8` on Windows, `/dev/ttyUSB0` on Linux/Mac).
   
   Run the server using your specific port:
   ```bash
   python server.py COM8
   ```

3. **Open the Dashboard UI:**
   While the server is running, exactly double-click `index.html` (or open it your favorite web browser). The page will connect to the local server automatically and start displaying solar position, servo angles, and power metrics.

## Important Note

The file `spa_calc.exe` (and its source files `spa.c`, `spa.h`, `main.c`) is used internally by `server.py` to continuously validate the microcontroller's math against the National Renewable Energy Laboratory (NREL) reference algorithm. Do not delete it.
