#!/usr/bin/env python3
"""
Solar Tracker Backend - Real-time UART to WebSocket Bridge
Calls compiled SPA executable for validation
"""

import asyncio
import json
import serial
import subprocess
import os
from datetime import datetime
try:
    from websockets.server import serve
except ImportError:
    import websockets
    serve = websockets.serve
from pathlib import Path

# =================== SPA EXECUTABLE WRAPPER ===================
class SPACalculator:
    """Wrapper to call compiled SPA C program"""
    
    def __init__(self, exe_path=None):
        """Initialize with path to spa_calc executable"""
        if exe_path is None:
            if os.name == 'nt':  # Windows
                exe_path = "./spa_calc.exe"
            else:  # Linux/Mac
                exe_path = "./spa_calc"
        
        self.exe_path = exe_path
        
        if not os.path.exists(exe_path):
            raise FileNotFoundError(
                f"SPA executable not found: {exe_path}\n"
                f"Compile with: gcc -o {os.path.basename(exe_path)} spa.c main.c -lm"
            )
        
        print(f"✓ Using SPA executable: {exe_path}")
    
    def calculate(self, year, month, day, hour, minute, second, lat, lon):
        """
        Call SPA executable and parse output
        Args: year, month, day, hour, minute, second, lat, lon
        Returns: dict with azimuth, elevation, zenith
        """
        try:
            # Build command
            cmd = [
                self.exe_path,
                str(year),
                str(month),
                str(day),
                str(hour),
                str(minute),
                str(second),
                str(lat),
                str(lon)
            ]
            
            # Run subprocess
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=5
            )
            
            if result.returncode != 0:
                raise RuntimeError(f"SPA calculation failed: {result.stderr}")
            
            # Parse output
            output = {}
            for line in result.stdout.strip().split('\n'):
                if '=' in line:
                    key, value = line.split('=')
                    output[key.strip()] = float(value.strip())
            
            return {
                'azimuth': round(output.get('AZIMUTH', 0.0), 2),
                'elevation': round(output.get('ELEVATION', 0.0), 2),
                'zenith': round(output.get('ZENITH', 0.0), 2)
            }
            
        except subprocess.TimeoutExpired:
            raise RuntimeError("SPA calculation timeout")
        except Exception as e:
            raise RuntimeError(f"SPA calculation error: {e}")

# =================== TELEMETRY PARSER ===================
def parse_telemetry(line):
    """
    Parse firmware telemetry line:
    $TELEM,YYYY-MM-DD,HH:MM:SS,LAT(N/S),LON(E/W),AZ,EL,SERVO_AZ,SERVO_EL,V,A,MODE*
    """
    if not line.startswith('$TELEM,'):
        return None
    
    try:
        parts = line.strip().rstrip('*').split(',')[1:]  # Skip $TELEM
        
        # Parse date/time
        date_parts = parts[0].split('-')
        time_parts = parts[1].split(':')
        
        # Parse location
        lat_str = parts[2]
        lon_str = parts[3]
        lat = float(lat_str[:-1])
        lon = float(lon_str[:-1])
        ns = lat_str[-1]
        ew = lon_str[-1]
        
        return {
            'datetime': {
                'year': int(date_parts[0]),
                'month': int(date_parts[1]),
                'day': int(date_parts[2]),
                'hour': int(time_parts[0]),
                'minute': int(time_parts[1]),
                'second': int(time_parts[2])
            },
            'location': {
                'latitude': lat,
                'longitude': lon,
                'ns': ns,
                'ew': ew,
                'source': 'GPS' if 'GPS' in parts[-1] else 'FIXED'
            },
            'firmware': {
                'azimuth': float(parts[4]),
                'elevation': float(parts[5])
            },
            'servo': {
                'azimuth': float(parts[6]),
                'elevation': float(parts[7])
            },
            'power': {
                'voltage': float(parts[8]),
                'current': float(parts[9])
            },
            'mode': parts[10].strip()
        }
    except (IndexError, ValueError) as e:
        print(f"Parse error: {e}")
        return None

# =================== WEBgccSOCKET SERVER ===================
class SolarTrackerServer:
    def __init__(self, uart_port='/dev/ttyUSB0', uart_baud=115200):
        self.uart_port = uart_port
        self.uart_baud = uart_baud
        self.clients = set()
        self.spa_calc = SPACalculator()
        self.serial_conn = None
        
    async def uart_reader(self):
        """Read UART data and broadcast to all clients"""
        try:
            self.serial_conn = serial.Serial(
                self.uart_port, 
                self.uart_baud, 
                timeout=1
            )
            print(f"✓ Connected to {self.uart_port}")
            
            while True:
                if self.serial_conn.in_waiting:
                    line = self.serial_conn.readline().decode('utf-8', errors='ignore')
                    
                    if line.startswith('$TELEM,'):
                        data = parse_telemetry(line)
                        if data:
                            # Calculate SPA reference
                            dt = data['datetime']
                            loc = data['location']
                            
                            lat = loc['latitude'] if loc['ns'] == 'N' else -loc['latitude']
                            lon = loc['longitude'] if loc['ew'] == 'E' else -loc['longitude']
                            
                            try:
                                spa_result = self.spa_calc.calculate(
                                    dt['year'], dt['month'], dt['day'],
                                    dt['hour'], dt['minute'], dt['second'],
                                    lat, lon
                                )
                                
                                # Calculate deltas
                                delta_az = round(data['firmware']['azimuth'] - spa_result['azimuth'], 2)
                                delta_el = round(data['firmware']['elevation'] - spa_result['elevation'], 2)
                                
                                # Determine accuracy level
                                max_error = max(abs(delta_az), abs(delta_el))
                                if max_error < 0.5:
                                    accuracy = 'excellent'
                                elif max_error < 1.0:
                                    accuracy = 'good'
                                else:
                                    accuracy = 'poor'
                                
                                data['spa'] = spa_result
                                data['delta'] = {
                                    'azimuth': delta_az,
                                    'elevation': delta_el,
                                    'accuracy': accuracy
                                }
                                
                            except Exception as e:
                                print(f"SPA calculation failed: {e}")
                                data['spa'] = None
                                data['delta'] = None
                            
                            # Broadcast to all connected clients
                            await self.broadcast(data)
                
                await asyncio.sleep(0.01)  # Small delay to prevent CPU spinning
                
        except serial.SerialException as e:
            print(f"✗ UART error: {e}")
            await asyncio.sleep(5)
    
    async def broadcast(self, data):
        """Send data to all connected WebSocket clients"""
        if self.clients:
            message = json.dumps(data)
            await asyncio.gather(
                *[client.send(message) for client in self.clients],
                return_exceptions=True
            )
    
    async def handler(self, websocket):
        """Handle new WebSocket connection"""
        self.clients.add(websocket)
        print(f"✓ Client connected ({len(self.clients)} total)")
        
        try:
            await websocket.wait_closed()
        finally:
            self.clients.remove(websocket)
            print(f"✗ Client disconnected ({len(self.clients)} remaining)")
    
    async def run(self):
        """Start server"""
        print("═══════════════════════════════════════════")
        print("   Solar Tracker WebSocket Server v1.0")
        print("   Using Compiled NREL SPA Executable")
        print("═══════════════════════════════════════════\n")
        
        async with serve(self.handler, "localhost", 8765):
            print("✓ WebSocket server running on ws://localhost:8765")
            await self.uart_reader()

# =================== MAIN ===================
if __name__ == "__main__":
    import sys
    
    uart_port = sys.argv[1] if len(sys.argv) > 1 else 'COM8'
    server = SolarTrackerServer(uart_port=uart_port)
    
    try:
        asyncio.run(server.run())
    except KeyboardInterrupt:
        print("\n\n✓ Server shutdown")