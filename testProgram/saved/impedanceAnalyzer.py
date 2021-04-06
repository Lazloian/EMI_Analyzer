import analyzerFunctions as af
import pandas as pd
import time

# create DataFrame to store the data (blank at start)
df = pd.DataFrame()

# variables to store program state
gotGain = False

# the number of sweeps saved on the device, if negative then it hasnt been check yet
num_saved = 0

# let the user know that h can be used to get command
print('\nImpedance Analyzer Controller')
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
        else:
            print("Calculate Gain Factors First!")

    elif (cmd == 'g'):
        gain = af.get_gain()
        gotGain = True

    elif (cmd == 'x'):
        for i in range(50):
            af.execute_sweep()
            time.sleep(0.25)

    elif (cmd == 'h'):
        af.print_commands()

    elif (cmd == 'p'):
        af.set_com_port()

    elif (cmd == 'd'):
        af.delete_sweeps()

    else:
        print('Input a valid command!')
    print('')
