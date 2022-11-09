from random import random
import simspad
from matplotlib import pyplot as plt
import numpy as np


# Simulate a SiPM
dt = 1E-11


# https://www.onsemi.com/pdf/datasheet/microj-series-d.pdf
JnumMicrocell = 14410
JvBias = 27.5
JvBr = 24.5
JtauRecovery = 2.2*14E-9
JpdeMax1 = 0.46/0.37 * 0.37 # 405nm
JpdeMax2 = 0.46/0.37 * 0.1 # 650nm
JvChr = 2.04
JcCell = 4.6E-14
JtauFwhm = 1.5E-9
JdigitalThreshold = 0

J30020 = simspad.SiPM(dt, JnumMicrocell, JvBias, JvBr, JtauRecovery,
                      JpdeMax1, JpdeMax2, JvChr, JcCell, JtauFwhm, JdigitalThreshold)


# https://www.onsemi.com/pdf/datasheet/microrb-series-d.pdf
RBnumMicrocell = 1590
RBvBias = 27.5
RBvBr = 24.5
RBtauRecovery = 2.2*21E-9
RBpdeMax1 = 0.46/0.37 * 0.1  # 405nm, approximate
RBpdeMax2 = 0.46/0.37 * 0.27 # 650nm
RBvChr = 2.04
RBcCell = 4.6E-14 # unknown
RBtauFwhm = 2.0E-9
RBdigitalThreshold = 0

RB10020 = simspad.SiPM(dt, RBnumMicrocell, RBvBias, RBvBr, RBtauRecovery,
                      RBpdeMax1, RBpdeMax2, RBvChr, RBcCell, RBtauFwhm, RBdigitalThreshold)


samples = 10000

optical_input1 = [4 * (abs(x-samples/2)/(samples/2)) for x in range(samples)]
optical_input2 = [10 * (1-abs(x-samples/2)/(samples/2)) for x in range(samples)]

simulator_url = "http://localhost:33232/simspad"

responseJ  =  J30020.simulate_web(simulator_url, optical_input1, optical_input2)

area_ratio = 9
optical_input1 = [o/area_ratio for o in optical_input1]
optical_input2 = [o/area_ratio for o in optical_input2]

responseRB = RB10020.simulate_web(simulator_url, optical_input1, optical_input2)
N = 200

responseJ = np.convolve(responseJ, np.ones(N)/N, mode='valid')
responseRB = np.convolve(responseRB, np.ones(N)/N, mode='valid')

time = np.arange(0, (len(responseJ)) * J30020.dt, J30020.dt)

plt.plot(time, responseJ)
plt.plot(time, responseRB)
plt.show()
