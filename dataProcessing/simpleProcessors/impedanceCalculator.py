import pandas as pd
import csv
import calcFunctions as cf
import os

# functions gets all the files in a path with a given extension
def get_files(path, extension):
    file_list = []

    # loop through all files in the path
    for r, dirs, files in os.walk(path):
        for f in files:
            if (f.endswith(extension)):
                file_list.append(os.path.join(r, f))

    return file_list

# deletes all files in a directory
def delete_files(path):
    # loop through all files in the path
    for r, dirs, files in os.walk(path):
        for f in files:
            os.remove(os.path.join(r, f))
    return

# the folder paths for input/output
inPath = 'Input'
outPath = 'Output'
gainPath = 'Gain'

# check if the folders exist at the paths. If not, create them and end the program
if not ((os.path.isdir(inPath)) and (os.path.isdir(outPath)) and (os.path.isdir(inPath))):
    print('Input directories not found. Creating them ...')
    os.mkdir(inPath)
    os.mkdir(outPath)
    os.mkdir(gainPath)
    print('Directories created. Copy sweep used for gain factor calculation into the Gain folder and the sweeps to process into the Input folder. Run the program again to get the impedance data in the Output folder.')
    quit()

# check if there is a sweep in the Gain folder to calculate gain factors
gainFiles = get_files(gainPath, '.csv')

if (len(gainFiles) == 0):
    print('ERROR: Gain sweep not found in Gain folder')
    quit()
if (len(gainFiles) != 1):
    print('ERROR: Multiple sweeps found in Gain folder')
    quit()

# read the csv file and convert to tuples
gain = pd.read_csv(gainFiles[0])
gain = list(gain.itertuples(index=False, name=None))

# calculate the gain factors (for now it is assumed that the calibration resistance is 1k Ohm)
gain = cf.calc_gain_factor(gain, 1000)

# check if there is at least one sweep in input folder
inFiles = get_files(inPath, '.csv')

if (len(inFiles) < 1):
    print('ERROR: No sweeps found in Input file')
    quit()

# clear the output folder
delete_files(outPath)

# Calcualte impedance for each sweep and save to Output folder
for sweep in inFiles:
    data = list(pd.read_csv(sweep).itertuples(index=False,name=None))
    data = cf.calc_impedance(data, gain)
    data = pd.DataFrame(data, columns=['Frequency', 'Impedance', 'Phase'])
    data.to_csv(outPath + '\\' + os.path.basename(sweep), index=False)

print('DONE')
