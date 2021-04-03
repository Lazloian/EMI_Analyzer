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

# checks if the prototype is connected over usb
def check_usb():
    # create a serial connection
    ser = open_usb()

    # check if the connection is made
    if ser is None:
        print('COM port not open. Make sure to set the COM port with "p"')

    # make sure that the correct device is connected
    else:
        # send a 1 to test connection
        send = [1]
        send = bytes(send)
        ser.write(send)

        # device should return 1
        recieve = ser.read(1)

        # check if the returned value is correct
        if (int.from_bytes(recieve, "little") == 1):
            print('Device Connected')
        else:
            print('Device Not Connected')

        ser.close()

    return

# sets the COM port that the device is connected to
def set_com_port():
    global comPort
    port = input('COM Port: ')
    comPort = 'COM' + port
    return
