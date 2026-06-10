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
pdeMax = 0.46
vChr = 2.04
cCell = 4.6E-14
tauFwhm = 1.5E-9
digitalThreshold = 0

j30020 = simspad.SiPM(dt, numMicrocell, vBias, vBr, tauRecovery,
                      pdeMax, vChr, cCell, tauFwhm, digitalThreshold)

samples = 10000
optical_input = [10 * (abs(x-samples/2)/samples/2) for x in range(samples)]
simulator_url = "http://localhost:33232/simspad"

for i in range(10):
    optical_input = [1000 for x in range(samples)]
    response = j30020.simulate_web(simulator_url, optical_input)
    N = 200

    response = np.convolve(response, np.ones(N)/N, mode='valid')

    time = np.arange(0, (len(response)) * j30020.dt, j30020.dt)

    plt.plot(time, response)


response = j30020.simulate_web(simulator_url, optical_input)

# To run the same simulation from the command line, write the device parameters
# (JSON) and the optical input (.npy), then:
#   simspad -p sipm.json -i sipm_in.npy -o sipm_out.npy
j30020.write_params("sipm.json")
simspad.SiPM.write_waveform("sipm_in.npy", optical_input)

# Waveforms (input or response) are plain 1-D float64 .npy arrays:
simspad.SiPM.write_waveform("sipm_out.npy", response)
bin_response = simspad.read_waveform("sipm_out.npy")
bin_sipm = simspad.read_params("sipm.json")

plt.show()
