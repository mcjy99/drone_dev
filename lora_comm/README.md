# LoRa Communication System for UAV/UGV

## Setup:
1. Install and setup mavlink-router on companion computer (eg: Raspberry Pi etc) 
2. Download **grove_switch.py**

## Deployment
1. Run mavlink-router with the command (substitute second endpoint with QGC IP)
   ```
   mavlink-routerd /dev/ttyACM0 -e 127.0.0.2:14550 -e 192.168.43.194:14550 -r
   ```
2. Run **grove_switch.py**
   ```
   python3 grove_switch.py
   ```

## Note
1. Ensure IP addresses correspond
2. Ensure UART/GPIO pins correspond (program is for Grove E5 module)
