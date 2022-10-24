# SimSPAD
Silicon Photomultiplier Simulator (Core Detection Code)

**SimSPAD** is a simple input output converter which takes an expected number of photons per time step, and various simulation parameters, and simulates the output of a silicon photomultiplier.

Silicon Photomultipliers (SiPMs) are solid-state single-photon-sensitive optical receivers.
SiPMs use a vast array of single photon avalanche diodes (SPADs) to enable single photon detection at an exceptionally high count rate.

For the purposes of my DPhil, I investigated using SiPMs with application of optical wireless receivers.
SiPMs consequently turn out to be extremely good detectors as they have a high bandwidth, high gain, and large area when contrasted with other silicon based photodetectors.

SiPMs unfortunately experience recovery-time based nonlinearity, which is due to typically unobservable parameters inside the device. This simulation allows both simulation of SiPMs for arbitary optical input, and observation of those unobservable parameters.

Simulation also allows for more complicated experiments to be performed in a contained environment where transmitter effects, thermal noise and more can be eliminated.

The simulation was originally written in MATLAB to interface with existing bit error rate detection code, but has since been migrated to c++ to enable the simulator to work (much, oh so much) faster.

This is an **experimental work in progress**, and more documentation will follow. This work has been/will be published in:

1. PRIME 2022 (Using Original MATLAB Simulation)
2. Photonics (Submitted, pending...) (Using Original MATLAB Simulation)
3. Upcoming Paper (Pending... Equaliser based)
5. Upcoming Paper (Pending... OFDM based)
4. My Thesis (Pending...)
