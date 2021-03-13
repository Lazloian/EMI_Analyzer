import serial
import math
import numpy as np
import pandas as pd
import struct
import time

comPort = 'COM5'

def save_sweeps(gain):
    numSaved = get_num_saved()

    if (numSaved < 1):
        print('No sweeps on flash to save. Aborting')
        return

    print('Files will be saved in this format: (filename)_(sweep number). example: ZK18_1')
    name = input('Input a filename: ')

    for i in range(numSaved, 0, -1):
        print(f'Saving Sweep #{i}')
        data = get_sweep(fromFlash=True)
        data = calc_impedance(data, gain)
        df = create_dataframe(data)
        print(df)
        print(f'Sweep #{i}')
        filename = name + '_' + str(i)
        output_csv(df, filename)
        time.sleep(1)

    return

def sweep_now():
    data = get_sweep(fromFlash=False)
    df = create_dataframe(data)
    print(df)

    return

def output_csv(df, name):
    df.to_csv(f'savedData\{name}.csv')
    print(f'Saved To: ./savedData/{name}.csv')

    return

def create_dataframe(data):
    df = pd.DataFrame(data, columns=['Frequency', 'Impedance', 'Phase'])

    return df

def get_gain():
    calibration = int(input('Input the Calibration Resistance (Ohms) : '))
    
    data = get_sweep(fromFlash=False)
    gain = calc_gain_factor(data, calibration)

    print('Gain Factors Calculated')

    return gain

# calculates the phase in degrees given real and imaginary impedance
def calc_phase(real, imag):
    if (real == 0):
        real = 1

    p = math.degrees(math.atan(imag / real))
    if (real < 0):
        p = p + 180
    else:
        if (imag < 0):
            p = p + 360
    return p

# calculates the adjusted impedance and phase values using the gain factor
def calc_impedance(data, gain):
    imp = []
    
    for ind, d in enumerate(data):
        mag = math.sqrt((d[1] ** 2) + (d[2] ** 2))
        i = 1 / (gain[ind][1] * mag)
        p = calc_phase(d[1], d[2])
        p = p - gain[ind][2]
        imp.append((d[0], i, p))

    return imp

# returns a list of tuples with gain factor and system phase for each frequency given a calibration resistance
def calc_gain_factor(data, calibration):
    gain = []
    for d in data:
        mag = math.sqrt((d[1] ** 2) + (d[2] ** 2))
        g = (1 / calibration) / mag
        ps = calc_phase(d[1], d[2])
        gain.append((d[0], g, ps))
    return gain

def get_num_saved():
    # open usb connection
    ser = open_usb()
    if not ser:
        return -1

    # send the get num saved command
    buff = bytes([1])
    ser.write(buff)
    
    # should get back the number of sweeps (4 byte int)
    buff = ser.read(4)

    # convert and print the number of saved sweeps
    num_saved = int.from_bytes(buff, "little")

    ser.close()

    return num_saved

# Executes a sweep that is then saved to flash on the nrf
def execute_sweep():
    # get the number of sweeps currently saved on flash
    numSaved = get_num_saved()
    print(f'Executing Sweep #{numSaved + 1}')

    # open usb connection and check if success
    ser = open_usb()
    if not (ser):
        return

    # send the execute sweep command
    buff = [2]
    buff = bytes(buff)
    ser.write(buff)

    # should read 2 back
    buff = ser.read(1)
    if (int.from_bytes(buff, "little") != 2):
        print("Sweep Execute Failed")
        return
    else:
        print(f'Sweep #{numSaved + 1} Executed and Saved To Flash')
        return

# returns a list of tuples with each element a data point in the sweep
# By default, this gets the next sweep from flash
# If False is passed, it will execute and get data from a sweep immedietly
def get_sweep(fromFlash=True):
    # open usb connection and check if success
    ser = open_usb()
    if not (ser):
        return

    # send the get sweep command
    if (fromFlash):
        buff = [4]
    else:
        buff = [3]

    buff = bytes(buff)
    ser.write(buff)

    # should read 3 back
    buff = ser.read(1)
    if (int.from_bytes(buff, "little") not in [3, 4]):
        print("Sweep Start Failed")
        ser.close()
        return

    freq = [0] # stores the frequency data
    imp = [0, 0] # stores the impedance data 0:real 1:imaginary
    data = [] # list to store the data

    # get data from the sweep, if the current frequency is 3, then the sweep is done
    while (True):
        # read a data point from the sensor
        buff = ser.read(8)
        
        # if the frequency of the data point is 0 (which is not possible on the AD5933) then the sweep is done
        freq = int.from_bytes(buff[0:4], "little", signed=False)
        if (freq == 0):
            break

        # get real and imaginary data and convert it
        imp[0] = int.from_bytes(buff[4:6], "big", signed=True)
        imp[1] = int.from_bytes(buff[6:8], "big", signed=True)

        #print(f'{freq} {imp}')

        data.append((freq, imp[0], imp[1]))

    return data

# sends the current sweep over usb
def send_sweep(sweep):
    # open a usb connection
    ser = open_usb()

    # check if connection success
    if not (ser):
        return

    # convert cyclesMultiplier, range, clockSource, and gain to values understood by AD5933
    converted = [0,0,0,0] # cyclesMultiplier, range, clockSource, gain
    
    # convert cyclesMultiplier
    if (sweep.get('cyclesMultiplier') == 1):
        converted[0] = 0
    elif (sweep.get('cyclesMultiplier') == 2):
        converted[0] = 1
    elif (sweep.get('cyclesMultiplier') == 4):
        converted[0] = 3

    # convert range
    if (sweep.get('range') == 1):
        converted[1] = 0
    elif (sweep.get('range') == 2):
        converted[1] = 3
    elif (sweep.get('range') == 3):
        converted[1] = 2
    elif (sweep.get('range') == 4):
        converted[1] = 1

    # convert clockSource
    if (sweep.get('clockSource') == 'internal'):
        converted[2] = 0
    elif (sweep.get('clockSource') == 'external'):
        converted[2] = 1

    # convert gain
    if (sweep.get('gain') == 1):
        converted[3] = 1
    elif (sweep.get('gain') == 5):
        converted[3] = 0
    
    # write a 2 for sweep transfer
    buff = [2]
    buff = bytes(buff)
    ser.write(buff)

    # expect a 2 in return
    buff = ser.read(1)
    if (int.from_bytes(buff, "little") == 2):
        print('Sending Sweep')
        buff = struct.pack('I I H H B B B B',
               sweep.get('start'),
               sweep.get('delta'),
               sweep.get('steps'),
               sweep.get('cycles'),
               converted[0],
               converted[1],
               converted[2],
               converted[3])

        print(buff)

        # write the data
        num = ser.write(buff)
        print(num)
    else:
        print('Sweep send failed')

# opens a usb connection on COM5
def open_usb():
    # create a serial connection
    ser = serial.Serial()

    # try to connect to port COM5
    try:
        ser.port = comPort
        ser.timout = 10
        ser.write_timeout = 10
        ser.open()
    except:
        # for some reason I couldn't put the code below here so I just
        print('')

    if not (ser.is_open):
        print('USB connection failed')
        return False
    else:
        return ser

# checks if the prototype is connected over usb
def check_usb():
    # create a serial connection
    ser = serial.Serial()

    # try to connect to port comPort
    try:
        ser.port = comPort
        ser.timout = 10
        ser.write_timout = 10
        ser.open()
    except:
        # for some reason I couldn't put the code below here so I just
        ser.close()

    if not (ser.is_open):
        print('COM port not open. Make sure the device is plugged in')
    else:
        # send a 1 to test connection
        send = [1]
        send = bytes(send)
        ser.write(send)

        # device should return 1
        recieve = ser.read(1)

        if (int.from_bytes(recieve, "little") == 1):
            print('Device Connected')
        else:
            print('Device Not Connected')

        ser.close()

    return

# edits the current sweep
def edit_sweep(sweep):
    # each parameter only has a certain range so we have to do this the long way
    print('Hit ENTER to keep current value')

    start = input('Start Frequency (Hz): ')
    if (start != ''):
        start = int(start)
        while ((start < 1000) or (start > 100000)):
            print('Start Frequency must be within 1kHz to 100kHz')
            start = int(input('Start Frequency (Hz): '))
        sweep['start'] = start

    delta = input('Delta Frequency (Hz): ')
    if (delta != ''):
        delta = int(delta)
        while ((delta < 0) or (delta > 100000)):
            print('Delta Frequency must be within 0Hz to 100kHz')
            delta = int(input('Delta Frequency (Hz): '))
        sweep['delta'] = delta

    steps = input('Number of Increments: ')
    if (steps != ''):
        steps = int(steps)
        while ((steps < 0) or (steps > 511)):
            print('Increments must be within 0 to 511')
            steps = int(input('Number of Increments: '))
        sweep['steps'] = steps

    cycles = input('Number of Settling Time Cycles: ')
    if (cycles != ''):
        cycles = int(cycles)
        while ((cycles < 0) or (cycles > 511)):
            print('Time Cycles must be within 0 to 511')
            cycles = int(input('Number of Settling Time Cycles: '))
        sweep['cycles'] = cycles

    mult = input('Time Cycles Multiplier (1, 2, 4): ')
    if (mult != ''):
        mult = int(mult)
        while (not (mult in [1, 2, 4])):
            print('Multiplier must be 1, 2, or 4')
            mult = int(input('Time Cycles Multiplier: '))
        sweep['cyclesMultiplier'] = mult
    
    rang = input('Output Excitation Range (1, 2, 3, 4): ')
    if (rang != ''):
        rang = int(rang)
        while (not (rang in [1, 2, 3, 4])):
            print('Range must be 1, 2, 3, or 4')
            rang = int(input('Output Excitation Range: '))
        sweep['range'] = rang

    source = input('Clock Source ("internal" or "external"): ')
    if (source != ''):
        while not ((source == 'internal') or (source == 'external')):
            print('Clock Source must be "internal" or "external"')
            source = input('Clock Source: ')
        sweep['clockSource'] = source

    if (source == 'internal'):
        print('Setting Clock Frequency to AD5933 internal clock 16776000 Hz')
        sweep['clockFrequency'] = 16776000
    elif (source == 'external'):
        sweep['clockFrequency'] = int(input('Input the external clock frequency: '))

    gain = input('PGA Gain (1 or 5): ')
    if (gain != ''):
        gain = int(gain)
        while (not (gain in [1, 5])):
            print('PGA Gain must be 1 or 5')
            gain = int(input('PGA Gain: '))
        sweep['gain'] = gain
    
    print('Sweep Edit Complete')
    return sweep

# prints the current sweep to the console
def print_sweep(sweep):
    print(f'''Sweep Parameters
            Start Frequency: {sweep.get('start')} Hz
            Delta Frequency: {sweep.get('delta')} Hz
            Number of Increments: {sweep.get('steps')}
            Number of Settling Time Cycles: {sweep.get('cycles')}
            Time Cycles Multiplier: X{sweep.get('cyclesMultiplier')}
            Output Excitation Range: Range {sweep.get('range')}
            Clock Source: {sweep.get('clockSource')}
            Clock Frequency: {sweep.get('clockFrequency')} Hz
            PGA Gain: X{sweep.get('gain')}''')

# prints the valid commands
def print_commands():
    print('''Valid Commands:
             q - quit the program
             p - print the current sweep
             e - edit the current sweep
             c - check if device is connected
             s - send the sweep to the sensor
             a - set the number of sweeps to average
             g - calculate multi-point gain factor
             x - execute the sweep on the sensor (must send the sweep with "s" first)
             o - output the impedance data to csv''')
