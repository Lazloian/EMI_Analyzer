import analyzerFunctions as af
import pandas as pd
# panda is a data analysis library

# create the default sweep dictionary to store the sweep parameters
sweep = {
        'start': 1000,
        'delta': 100,
        'steps': 490,
        'cycles': 15,
        'cyclesMultiplier': 1,
        'range': 1,
        'clockSource': 'internal',
        'clockFrequency': 16776000,
        'gain': 1
        }

# create DataFrame to store the data (blank at start)
df = pd.DataFrame()

# variables to store program state
gotGain = False
gotData = False

# the number of sweeps saved on the device, if negative then it hasnt been check yet
num_saved = 0

# let the user know that h can be used to get command
print('\nThis is the default sweep:')
af.print_sweep(sweep)
print('\nFor a list of commands enter "h"\n')

# main loop for getting user input
while(1):
    cmd = input('Input a Command: ')
    print('')

    if (cmd == 'q'):
        quit()

    elif (cmd == 'c'):
        num_saved = af.get_num_saved()
        if (num_saved >= 0):
            print(f'Sweeps on flash: {num_saved}')

    elif (cmd == 'o'):
        if (gotGain):
            af.save_sweeps(gain)

    elif (cmd == 'a'):
        num_ave = af.set_ave()

    elif (cmd == 'g'):
        gain = af.get_gain()
        gotGain = True

    elif (cmd == 'x'):
        af.execute_sweep()

    elif (cmd == 'NOT_IN_USE'):
        if gotGain:
            df = af.sweep_ave(gain, num_ave)
            gotData = True
            print(df)
            print('Sweep Complete')
        else:
            print('Calculate Gain Factor First!')
    elif (cmd == 'NOT_IN_USE_EITHER'):
        if gotData:
            af.output_csv(df)
        else:
            print('Execute a Sweep First!')

    elif (cmd == 'h'):
        af.print_commands()
    elif (cmd == 'p'):
        af.set_com_port()
    elif (cmd == 'e'):
        sweep = af.edit_sweep(sweep)
        af.print_sweep(sweep)
    elif (cmd == 's'):
        af.send_sweep(sweep)
    else:
        print('Input a valid command!')
    print('')
