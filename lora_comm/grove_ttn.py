#switching scheme between Wi-Fi and LoRaWAN (TTN)
import subprocess
import re
import serial
import subprocess
import socket
import time

server_ip = "127.0.0.2" 
server_port = 14550
ser = serial.Serial('/dev/ttyTHS1')  # UART to LoRa module AT+ID AT+MODE=LWOTAA AT+KEY=APPKEY,"8985A37C522D1B02CDC883AB39FEA7B8" AT+DR=AS923 AT+DR=0

def write_lora(cmd):
    ser.write(str.encode(cmd))
    time.sleep(1) #delay to ensure command sent 

def setup(): #AT+ID
    write_lora("AT+MODE=LWOTAA") #set mode
    write_lora("AT+DR=AS923") #set region
    write_lora("AT+DR=5") #set DR, 0-7, DR0 = SF12 and so on, refer to datasheet (DR7 = FSK)
    write_lora("AT+ADR= OFF")
    write_lora("AT+CLASS=C") #set class C
    write_lora("AT+KEY=APPKEY,\"8985A37C522D1B02CDC883AB39FEA7B8\"") #set app key AT+KEY=APPKEY,"8985A37C522D1B02CDC883AB39FEA7B8"
    print("setup done")
    join_ttn()
          
def join_ttn():
    write_lora("AT+JOIN") #join network
    join_check = ser.readline() 
    print("Status:" + str(join_check))
    if (join_check) == "+JOIN: Join failed":
        join_ttn()
    elif(join_check == "+JOIN: Network joined" or join_check == "+JOIN: Joined already"):
        print("JOIN done")

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
                        print(signal_level)
                else:
                    print(signal_level)

def lora_transmit(ip_address, port):
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    try:
        udp_socket.bind((ip_address, port))
        #print(f"UDP server is listening on {ip_address}:{port}")

        while True:
            data, addr = udp_socket.recvfrom(1024)
            msg_type = data[7]
            comp_id = data[6]
            #print(f"Received data from {addr}: {data}")
            mav_msg = data.hex()  # Convert bytes to hexadecimal string
            #counter = 0
            #counter_string = str(counter)
            mav_msg = "D1" + mav_msg #add node ID
            #print({mav_msg})

            if msg_type == 0 and comp_id == 0x01: #filter by autopilot heartbeat
                transmit_ttn(mav_msg)  # Send msg via AT command
                break

    except KeyboardInterrupt:
        print("\nServer stopped by the user.")
    finally:
        udp_socket.close()

def transmit_ttn(mav_msg):
    try:
        print(f"Message: {mav_msg}")
        ser.write(str.encode(f"AT+MSGHEX=\"{mav_msg}\"\r\n"))  # Send message to TTN via AT command AT+MSGHEX="D1FF0209"
        if (ser.readline() == "Done"):
         print("sent heartbeat")
         time.sleep(20)
         
        #write_lora("AT+MSG") #flush uplink (need to do every cycle)
        #print("sent empty message")
        #time.sleep(20)
        #time.sleep(0.2) #any lower will have error 24
    except Exception as e:
        print(f"Error writing to serial: {e}")

if __name__ == "__main__":
    setup()
    try:
        while 1:
            lora_transmit(server_ip,server_port)

    except KeyboardInterrupt:
        print("\nStopped by the user.")