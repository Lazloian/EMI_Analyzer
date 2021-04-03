import serial
import pandas as pd
import time
import usbFunctions as uf
import calcFunctions as cf

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
        print('Data Transferred. Processing ...')
        data = cf.calc_impedance(data, gain)
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
    gain = cf.calc_gain_factor(data, calibration)

    print('Gain Factors Calculated')

    return gain

def get_num_saved():
    # open usb connection
    ser = uf.open_usb()
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

def delete_sweeps():
    # open usb connection
    ser = uf.open_usb()
    if not ser:
        return -1

    # send the get num saved command
    buff = bytes([5])
    ser.write(buff)

    # should read 5 back
    buff = ser.read(1)
    if (int.from_bytes(buff, "little") != 5):
        print("Sweep Deletes Failed")
        return
    else:
        print(f'All sweeps deleted')
        return

# Executes a sweep that is then saved to flash on the nrf
def execute_sweep():
    # get the number of sweeps currently saved on flash
    numSaved = get_num_saved()
    print(f'Executing Sweep #{numSaved + 1}')

    # open usb connection and check if success
    ser = uf.open_usb()
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
    ser = uf.open_usb()
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

# prints the valid commands
def print_commands():
    print('''Valid Commands:
             q - quit the program
             p - select the COM port of the device
             c - check if device is connected
             g - calculate gain factors
             x - execute and save a sweep from the device
             o - output the impedance data to csv
             d - delete all sweeps on device''')
