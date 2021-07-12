import pandas as pd
import csv
import calcFunctions as cf
import os
import sys

# Get the file paths from stdin
# The first argument is gain, second is input, third is output
gainFile = sys.argv[1];
inFile   = sys.argv[2];
outFile  = sys.argv[3];

# read the gain file and convert to tuples
gain = pd.read_csv(gainFile)
gain = list(gain.itertuples(index=False, name=None))

# calculate the gain factors (for now it is assumed that the calibration resistance is 1k Ohm)
gain = cf.calc_gain_factor(gain, 1000)

# Calcualte impedance for each sweep and save to Output folder
data = list(pd.read_csv(inFile).itertuples(index=False,name=None))
data = cf.calc_impedance(data, gain)
data = pd.DataFrame(data, columns=['Frequency', 'Impedance', 'Phase'])
data.to_csv(outFile, index=False)
