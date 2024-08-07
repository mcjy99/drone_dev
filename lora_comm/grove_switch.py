#switching scheme between Wi-Fi and LoRa P2P
import subprocess
import re
import serial
import subprocess
import socket
import time

server_ip = "127.0.0.2" #depends on local port mavlink-router directs to
server_port = 14550
ser = serial.Serial('/dev/ttyTHS1')  # UART to LoRa module 

def setup():
    ser.write(str.encode(f"AT+MODE=TEST"))
    time.sleep(0.6) #delay to ensure command sent
    ser.write(str.encode(f"AT+TEST=RFCFG,923,SF12,125,8,8,14,ON,OFF,OFF")) #set parameters (FREQ,SF,BW,TXPREAMBLE,RXPREAMBLE,POWER,CRC,INVERTEDIQ,NET[PUBLIC LORAWAN])

def scan_transmit(): #iwconfig wlan0 not valid for jetpack 6
    process = subprocess.Popen('iw dev wlan0 link', shell=True, stdout=subprocess.PIPE, universal_newlines=True)
    output, error = process.communicate()
    for line in output.split('\n'):
        if 'Not connected.' in line:
            print("not connected")
            lora_transmit(server_ip,server_port) #if not connected
        elif 'signal' in line:
            match = re.search(r'signal:\s*(-?\d+)\s*dBm', line)
            if match:
                signal_level = match.group(1)
                signal_level = int(signal_level)
                #print(signal_level)                                                                    
                if signal_level < -65:
                    print("signal low")
                    lora_transmit(server_ip,server_port)
                    if signal_level > -55: #hysteresis
                        #print("mavlink-router") #switch back
                        print(signal_level)
                else:
                    #print("mavlink-router") #don't switch to lora
                    print(signal_level)

def lora_transmit(ip_address, port):
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    try:
        udp_socket.bind((ip_address, port))
        print(f"UDP server is listening on {ip_address}:{port}")

        while True:
            data, addr = udp_socket.recvfrom(1024)
            msg_type = data[7]
            comp_id = data[6]
            #print(f"Received data from {addr}: {data}")
            mav_msg = data.hex()  # Convert bytes to hexadecimal string
            mav_msg = "D1" + mav_msg #add node ID
            #print(mav_msg)

            if msg_type == 0 and comp_id == 0x01: #filter by autopilot heartbeat
                write_lora(mav_msg)  # Send msg via AT command
                #print(mav_msg)
                break

    except KeyboardInterrupt:
        print("\nServer stopped by the user.")
    finally:
        udp_socket.close()

def write_lora(mav_msg):
    try:
        print(f"Writing: {mav_msg}")
        ser.write(str.encode(f"AT+TEST=TXLRPKT,{mav_msg}\r\n"))  # Send MAV message via AT command
        time.sleep(0.2) #any lower will have error 24
    except Exception as e:
        print(f"Error writing to serial: {e}")
        
if __name__ == "__main__":
    setup()
    while 1:
        scan_transmit()
