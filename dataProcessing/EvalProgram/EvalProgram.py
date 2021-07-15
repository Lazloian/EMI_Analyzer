import analyzerFunctions as af
import nrfFunctions as nrf
import usbFunctions as uf
import pandas as pd

# variables to store program state
gotGain = False
gotSweep = False

# variable to store the number of saved sweeps from the device
numSaved = -1

# the currently loaded sweep
sweepFrame = pd.DataFrame(columns=['Frequency', 'Impedance', 'Phase'])
gainFrame = pd.DataFrame(columns=['Frequency', 'Gain Factor', 'System Phase'])

# let the user know that h can be used to get command
print('\nAD5933 Eval Program 2')
print('\nFor a list of commands enter "help"\n')

# main loop for getting user input
while(1):
    cmd = input('Input a Command: ')
    print('')

    if (cmd in ['quit','0']):
        quit()

    elif (cmd in ['port','1']):
        uf.set_com_port()

    elif (cmd in ['num', '2']):
        numSaved = nrf.get_num_saved()
        print(f'Number of Saved Sweeps: {numSaved}')

    elif (cmd in ['exec','3']):
        if (gotGain):
            sweepFrame = af.sweep(gain)
            gotSweep = True
        else:
            print('Perform Calibration First!')

    elif (cmd in ['cali','4']):
        gain = af.get_gain()
        gainFrame = af.create_gainFrame(gain)
        print('')
        print(gainFrame)
        gotGain = True
        gotSweep = False

    elif (cmd in ['plot','5']):
        af.plot(sweepFrame, gainFrame)

    elif (cmd in ['save','6']):
        af.save(sweepFrame, gainFrame)

    elif (cmd in ['del','7']):
        nrf.delete_sweeps()

    elif (cmd in ['help','8']):
        af.print_commands()

    else:
        print('Input a valid command!')
    print('')
