# usbFunctions.py
# This file contains functions for interracting over usb
# 
# Author: Henry Silva

import serial

# COM port of usb device
comPort = 'COM5'

# opens a usb connection. Returns the connection if success, NULL if failed
def open_usb():
    # create a serial connection
    ser = serial.Serial()

    # try to connect to a usb device on comPort
    try:
        ser.port = comPort
        ser.timout = 10
        ser.write_timeout = 10
        ser.open()
    except:
        ser.close()

    if not (ser.is_open):
        print('USB connection failed')
        return None
    else:
        return ser

def send_byte(buff):
    ser = open_usb()
    if not ser:
        return

    ser.write(buff)

    ser.close()
    return

# sets the COM port that the device is connected to
def set_com_port():
    global comPort
    port = input('COM Port: ')
    comPort = 'COM' + port
    return
