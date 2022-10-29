# SimSPAD

![logo](https://github.com/WillMatthews/SimSPAD/blob/documentation/doc/img/simspad_logo.svg)

A high performance avalanche multiplication based optical receiver simulator.
Version 0.1

## About

**SimSPAD** is an input output converter which takes an input of simulation parameters and an arbitrary length vector of expected number of photons striking the detector array per time step. 
From here, the output of a silicon photomultiplier is simulated.
Currently the software returns the total charge emitted per time step, which can then be pulse-shaped with a convolution further down the line.
Pulse-shaping is being integrated into the software so in the future this step may be omitted.

## Background on SiPMs, and why simulation is necessary

Silicon Photomultipliers (SiPMs) are solid-state single-photon-sensitive optical receivers.
SiPMs use a vast array of single photon avalanche diodes (SPADs) to enable single photon detection at an exceptionally high count rate.

For the purposes of my DPhil, I investigated using SiPMs with application of high-performance receivers for optical wireless communications.
SiPMs consequently have turned out to be extremely good detectors as they have a high bandwidth, high gain, and large area when contrasted with other silicon based photodetectors.

SiPMs unfortunately experience recovery-time based nonlinearity, which is due to the recharge period each microcell undergoes after detection oh a photon.
Interestingly, there are unobservable parameters inside the device as a result which dictate the shape of the output.
These unobservable parameters include both the array average photon detection efficiency, and the mean fired charge per microcell.
This simulation allows both simulation of SiPMs for arbitary optical input, and observation of those unobservable parameters.

Simulation also allows for more complicated experiments to be performed in a contained environment where transmitter effects, thermal noise and more can be eliminated.

The simulation was originally written in MATLAB to interface with existing bit error rate detection code, but has since been migrated to c++ to enable the simulator to work (much, oh so much) faster.

## Publications involving this simulation

This is an **experimental work in progress**, and more documentation will follow. This work has been/will be published in:

1. PRIME 2022 (Using Original MATLAB Simulation)
2. Photonics (Submitted, pending...) (Using Original MATLAB Simulation)
3. Upcoming Paper (Pending... Equaliser based)
5. Upcoming Paper (Pending... OFDM based)
4. My Thesis (Pending...)

## How to Reference:

If you have used this software for your work, please reference it as follows:
William Matthews (2022) SimSPAD (Version 0.1) [Source Code]. Private Distribution

Eventually, when this repo is made public, you may cite as:
William Matthews (2022) SimSPAD (Version 0.1) [Source Code]. https://github.com/WillMatthews/SimSPAD
