import pandas as pd
import time
import calcFunctions as cf
import nrfFunctions as nrf
import os

# saves all the sweeps on the device
def save_sweeps(gain):

    # create a directory to put all the saved data
    path = 'savedData'
    if not os.path.isdir(path):
        os.mkdir(path)

    numSaved = nrf.get_num_saved()

    if (numSaved < 1):
        print('No sweeps on flash to save. Aborting')
        return

    # get the name of the sweep to save
    print('Files will be saved in this format: (filename)_(sweep number). example: ZK18_1')
    name = input('Input a filename: ')

    # check if the name has already been used
    path = 'savedData\\' + name
    while (os.path.isdir(path)):
        name = input('Filename already present in saved data, please input a new one: ')
        path = 'savedData\\' + name

    os.mkdir(path)

    for i in range(numSaved, 0, -1):
        print(f'Saving Sweep #{i}')
        data = nrf.get_sweep(fromFlash=True)
        print('Data Transferred. Processing ...')
        data = cf.calc_impedance(data, gain)
        df = create_dataframe(data)
        print(df)
        print(f'Sweep #{i}')
        filename = name + '_' + str(i)
        output_csv(df, filename, path)
        time.sleep(0.25)

    return

# executes a sweep and immedietly sends the data over usb. Does not save to flash
def sweep_now():
    data = nrf.get_sweep(fromFlash=False)
    df = create_dataframe(data)
    print(df)

    return

# outputs the given dataframe to a csv file
def output_csv(df, name, path):
    df.to_csv(f'{path}\\{name}.csv')
    print(f'Saved To: .\\{path}\\{name}.csv')

    return

# creates a dataframe given an array of data
def create_dataframe(data):
    df = pd.DataFrame(data, columns=['Frequency', 'Impedance', 'Phase'])

    return df

# performs a gain calibration sweep and calculates the gain factors
def get_gain():
    calibration = int(input('Input the Calibration Resistance (Ohms) : '))
    
    data = nrf.get_sweep(fromFlash=False)
    gain = cf.calc_gain_factor(data, calibration)

    print('Gain Factors Calculated')

    return gain

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
