import pandas as pd
import csv
import calcFunctions as cf
import os
import sys
import matplotlib.pyplot as plt

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

# Get the file paths from stdin
# The first argument is gain, second is calibration resistance, 
# third is input, fourth is output
gainFile = sys.argv[1]
calib    = int(sys.argv[2])
inFile   = sys.argv[3]
outFile  = sys.argv[4]

# read the gain file and convert to tuples
gain = pd.read_csv(gainFile)
gain = list(gain.itertuples(index=False, name=None))

# calculate the gain factors (for now it is assumed that the calibration resistance is 1k Ohm)
gain = cf.calc_gain_factor(gain, calib)

# Calcualte impedance for each sweep and save to Output folder
data = list(pd.read_csv(inFile).itertuples(index=False,name=None))
data = cf.calc_impedance(data, gain)
data = pd.DataFrame(data, columns=['Frequency', 'Impedance', 'Phase', 'Conductance', 'Susceptance'])
print(data)
data.to_csv(outFile, index=False)
plot_sweep(data)
