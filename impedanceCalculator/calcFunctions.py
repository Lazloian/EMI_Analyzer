# calcFunctions.py
#
# This file contains the functions that perform the impedance and phase calculations
#
# Author: Henry Silva

import math

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

