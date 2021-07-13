import pandas as pd
import time
import calcFunctions as cf
import nrfFunctions as nrf
import os
import matplotlib.pyplot as plt

def plot(sweepFrame, gainFrame):
    toPlot = input('Plot (s)weep, (g)ain, or (b)oth: ')

    if (toPlot == 's'):
        plot_sweep(sweepFrame)
    elif (toPlot == 'g'):
        plot_gain(gainFrame)
    elif (toPlot == 'b'):
        plot_sweep(sweepFrame)
        plot_gain(gainFrame)
    else:
        print('Invalid Option, Input s, g, or b')
    return

def plot_sweep(sweepFrame):
    # create figure with subplots
    fig, axis = plt.subplots(2,2)

    axis[0, 0].plot(sweepFrame['Frequency'], sweepFrame['Impedance'], color='b')
    axis[0, 0].set_xlabel('Frequency (Hz)')
    axis[0, 0].set_ylabel('Impedance')
    axis[0, 0].set_title('Sweep Impedance')

    axis[1, 0].plot(sweepFrame['Frequency'], sweepFrame['Phase'], color='r')
    axis[1, 0].set_xlabel('Frequency (Hz)')
    axis[1, 0].set_ylabel('Phase (degrees)')
    axis[1, 0].set_title('Sweep Phase')

    axis[0, 1].plot(sweepFrame['Frequency'], sweepFrame['Conductance'], color='c')
    axis[0, 1].set_xlabel('Frequency (Hz)')
    axis[0, 1].set_ylabel('Conductance')
    axis[0, 1].set_title('Sweep Conductance')

    axis[1, 1].plot(sweepFrame['Frequency'], sweepFrame['Susceptance'], color='y')
    axis[1, 1].set_xlabel('Frequency (Hz)')
    axis[1, 1].set_ylabel('Susceptance')
    axis[1, 1].set_title('Sweep Susceptance')

    fig.tight_layout()
    plt.show()

def plot_gain(gainFrame):
    # create figure with subplots
    fig, axis = plt.subplots(2,1)

    axis[0].plot(gainFrame['Frequency'], gainFrame['Gain Factor'], color='g')
    axis[0].set_xlabel('Frequency (Hz)')
    axis[0].set_ylabel('Gain Factor')
    axis[0].set_title('Gain Factors')

    axis[1].plot(gainFrame['Frequency'], gainFrame['System Phase'], color='c')
    axis[1].set_xlabel('Frequency (Hz)')
    axis[1].set_ylabel('System Phase (degrees)')
    axis[1].set_title('System Phase')

    fig.tight_layout()
    plt.show()

def save(sweepFrame, gainFrame):
    toSave = input('Save (s)weep, (g)ain, or (b)oth: ')

    if (toSave == 's'):
        save_sweep(sweepFrame)
    elif (toSave == 'g'):
        save_gain(gainFrame)
    elif (toSave == 'b'):
        save_sweep(sweepFrame)
        save_gain(gainFrame)
    else:
        print('Invalid Option, Input s, g, or b')
    return


# save the currently loaded sweep
def save_sweep(sweep):
    print('')
    print('Saving Sweep')

    # create a directory to put all the saved data
    path = 'savedSweeps'
    if not os.path.isdir(path):
        os.mkdir(path)

    # get the name of the sweep to save
    name = input('Input a filename: ')

    # check if the name has already been used
    csv = 'savedSweeps\\' + name + '.csv'
    while (os.path.isfile(csv)):
        name = input('Filename already present in saved data, please input a new one: ')
        csv = 'savedSweeps\\' + name + '.csv'
    
    # output sweep in csv format
    filename = name
    output_csv(sweep, filename, path)
    return

# save the currently loaded gain factors
def save_gain(gain):
    print('')
    print('Saving Gain Factors')
    # create a directory to put all the saved data
    path = 'savedGainFactors'
    if not os.path.isdir(path):
        os.mkdir(path)

    # get the name of the gain to save
    name = input('Input a filename: ')

    # check if the name has already been used
    csv = 'savedGainFactors\\' + name + '.csv'
    while (os.path.isfile(csv)):
        name = input('Filename already present in saved data, please input a new one: ')
        csv = 'savedGainFactors\\' + name + '.csv'
    
    # output gain in csv format
    filename = name
    output_csv(gain, filename, path)
    return

# executes a sweep and calculates impedance
def sweep(gain):
    data = nrf.execute_sweep()
    data = cf.calc_impedance(data, gain)
    df = create_dataframe(data)
    print('')
    print(df)

    return df

# outputs the given dataframe to a csv file
def output_csv(df, name, path):
    df.to_csv(f'{path}\\{name}.csv', index=False)
    print(f'Saved To: .\\{path}\\{name}.csv')

    return

# creates a dataframe given an array of data
def create_dataframe(data):
    df = pd.DataFrame(data, columns=['Frequency', 'Impedance', 'Phase', 'Conductance', 'Susceptance'])

    return df

def create_gainFrame(gain):
    gf = pd.DataFrame(gain, columns=['Frequency', 'Gain Factor', 'System Phase'])

    return gf

# performs a gain calibration sweep and calculates the gain factors
def get_gain():
    calibration = int(input('Input the Calibration Resistance (Ohms) : '))
    
    data = nrf.execute_sweep()
    gain = cf.calc_gain_factor(data, calibration)

    print('Gain Factors Calculated')

    return gain

# prints the valid commands
def print_commands():
    print('''Valid Commands (Both number and word work):
             0. quit - quit the program
             1. port - select COM port
             2. num  - get the nuber of saved sweeps from the device
             3. exec - execute and calculate impedance
             4. cali - perform gain calibration sweep
             5. plot - plot the current sweep
             6. save - save the current sweep
             7. del  - delete all sweeps on the device
             8. help - display this message again''')
