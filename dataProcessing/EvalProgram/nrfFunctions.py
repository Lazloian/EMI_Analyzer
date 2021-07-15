# nrfFunctions.py
#
# This file contains functions to interact with the nrf52
#
# Author: Henry Silva

import usbFunctions as uf
import time

# sends the command to delete all sweeps on the device
def delete_sweeps():
    # open usb connection
    ser = uf.open_usb()
    if not ser:
        return -1

    # send the get num saved command
    buff = [ord('4')]
    buff = bytes(buff)
    ser.write(buff)

    print('Deleting Sweeps on Device ...')

    # wait for sweeps to be delete
    buff = ser.read(1)
    ret = int.from_bytes(buff, "little")

    # if device sends 1 then success, if 2 then fail
    if (ret == 1):
        print('Device Cleared')
    else:
        print('Sweep Deletion Failed')
    
    ser.close()

    return

# gets the number of saved sweeps from the device
def get_num_saved():
    # open usb connection
    ser = uf.open_usb()
    if not ser:
        return -1

    # send the get num saved command
    buff = [ord('1')]
    buff = bytes(buff)
    ser.write(buff)
    
    # should get back the number of sweeps (4 byte int)
    buff = ser.read(4)

    # convert and print the number of saved sweeps
    num_saved = int.from_bytes(buff, "little")

    ser.close()

    return num_saved

# gets the next saved sweep from the device's flash memory
def get_saved_sweep():
    # open usb connection and check if success
    ser = uf.open_usb()
    if not (ser):
        return

    # send the get saved sweep command
    buff = [ord('3')]

    buff = bytes(buff)
    ser.write(buff)

    # should read 1 back
    buff = ser.read(1)
    if (int.from_bytes(buff, "little") != 1):
        print("Sweep Execute Failed")
        ser.close()
        return
    else:
        print("Sweep Executing")

    return get_sweep(ser)


# sends the execute sweep command and returns the sweep data
def execute_sweep():
    # open usb connection and check if success
    ser = uf.open_usb()
    if not (ser):
        return

    # send the execute sweep command
    buff = [ord('2')]

    buff = bytes(buff)
    ser.write(buff)

    # should read 1 back
    buff = ser.read(1)
    if (int.from_bytes(buff, "little") != 1):
        print("Sweep Execute Failed")
        ser.close()
        return
    else:
        print("Sweep Executing")

    return get_sweep(ser)

# gets a sweep that is being sent over usb
# ser is the open usb port
def get_sweep(ser):

    freq = [0]   # stores the frequency data
    imp = [0, 0] # stores the impedance data 0:real 1:imaginary
    data = []    # list to store the data

    # get data from the sweep, if the current frequency is 0, then the sweep is done
    while (True):
        # read a data point from the sensor
        buff = ser.read(8)
        
        # if the frequency of the data point is 0 (which is not possible on the AD5933) then the sweep is done
        freq = int.from_bytes(buff[0:4], "little", signed=False)
        if (freq == 0):
            break

        # get real and imaginary data and convert it
        imp[0] = int.from_bytes(buff[4:6], "little", signed=True)
        imp[1] = int.from_bytes(buff[6:8], "little", signed=True)

        print(f'Real: {imp[0]} Imag: {imp[1]}')

        data.append((freq, imp[0], imp[1]))

    return data
