#simple P2P over LoRa with Grove E5 LoRa module
import serial
import time

ser = serial.Serial('/dev/ttyTHS1') #uart to grove
ser.write(str.encode("AT+MODE=TEST")) #AT+TEST=? AT+MODE=? AT+TEST=TXCLORA
time.sleep(0.6)
ser.write(str.encode("AT+TEST=RFCFG,923,SF7,125,8,8,14,ON,OFF,OFF")) #set parameters 
time.sleep(0.6)
#print(ser.readline())

def readInput(): 
    ser.write(str.encode("AT+TEST=RXLRPKT")) 
    print(ser.readline())

def writeOut():
    try:
        mav_msg = "D1FD0901020304" #test message
        print(f"Writing: {mav_msg}")
        ser.write(str.encode(f"AT+TEST=TXLRPKT,{mav_msg}\r\n"))  # Send MAVlink via AT command
        time.sleep(0.2) #any lower will have error 24
    except Exception as e:
        print(f"Error writing to serial: {e}")

while 1:
    #readInput() 
    writeOut()
    
