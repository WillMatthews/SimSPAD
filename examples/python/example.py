from random import random
import simspad
from matplotlib import pyplot as plt
import numpy as np


# Simulate a SiPM
dt = 1E-11
numMicrocell = 14410
vBias = 27.5
vBr = 24.5
tauRecovery = 2.2*14E-9
pdeMax1 = 0.46
pdeMax2 = 0.05
vChr = 2.04
cCell = 4.6E-14
tauFwhm = 1.5E-9
digitalThreshold = 0

j30020 = simspad.SiPM(dt, numMicrocell, vBias, vBr, tauRecovery,
                      pdeMax1, pdeMax2, vChr, cCell, tauFwhm, digitalThreshold)

samples = 100
optical_input1 = [10 * (abs(x-samples/2)/samples/2) for x in range(samples)]
optical_input2 = [90 * (abs(x-samples/2)/samples/2) for x in range(samples)]
simulator_url = "http://localhost:33232/simspad"

response = j30020.simulate_web(simulator_url, optical_input1, optical_input2)
N = 200

#response = np.convolve(response, np.ones(N)/N, mode='valid')
print(response)
print(sum(response))

time = np.arange(0, (len(response)) * j30020.dt, j30020.dt)

#plt.plot(time, response)
#plt.show()
