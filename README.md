# SimSPAD

![logo](https://github.com/WillMatthews/SimSPAD/blob/master/doc/img/simspad_logo.svg)

A high performance avalanche multiplication based optical receiver simulator.
Version 0.1

[![Build](https://github.com/WillMatthews/SimSPAD/actions/workflows/makefile.yml/badge.svg)](https://github.com/WillMatthews/SimSPAD/actions/workflows/makefile.yml)

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


## Publications involving this simulation:

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


## Usage

### Standalone

![logo](https://github.com/WillMatthews/SimSPAD/blob/master/doc/img/example.gif)

SimSPAD can be run as a standalone program from the command line, taking an input binary file, and producing an output binary file.
This binary file is currently produced by MATLAB, but Python and C++ generators will be created soon.

### Web Application

Once SimSPAD server is running, send a POST request to `http://localhost/simspad`.
The reply from the server will be the result from the simulation.
The data to and from the server is packaged as characters - see below in the Binary Format section for more details.

## Install

### Standalone
Make the executable with `make`. The executable will produced as `./build/apps/simspad`.

### Web Application
Make the executable with `make server`. The executable will produced as `./build/apps/server`.

Create a new user `useradd simspad`
<details>
<summary>Create a systemd file to run the executable </summary>

```
[Unit]
Description=SimSPAD Avalanche Photodetector Simulator
Requires=network-online.target
Wants=network-online.target
After=network.target syslog.target network-online.target

[Service]
User=simspad
ExecStart=/path/to/server
RestartSec=5
Restart=always

[Install]
WantedBy=multi-user.target
```
</details>

<details>
<summary>Edit Nginx sites-available to proxy pass to the simspad server </summary>

Add the following location:

```
    location /sim/ {
        # proxy_buffering off;
        proxy_pass http://127.0.0.1:33232/;
    }
```
</details>


<details>
<summary>Edit nginx.conf to increase maximum upload size </summary>

Add the following to the end of `http{}`:

```
    client_max_body_size 200M;
```
</details>

## Binary Format

The binary files used by SimSPAD are vectors of double precision floating point numbers.
<details>
<summary>The first ten double floats are:</summary>

    (in order)
        dt                - Simulation time step size
        numMicrocell      - Number of Detectors
        vBias             - Bias Voltage
        vBr               - Breakdown Voltage
        tauRecovery       - Recharge time constant
        pdeMax            - Max PDE for PDE-Vover equation
        vChr              - Characteristic Voltage for PDE-Vover equation
        cCell             - Capacitance per detector
        tauFwhm           - Output pulse full width half max time
        digitalThreshhold - Detection Threshhold (as a fraction of overvoltage from bias)

</details>

And the remainder of doubles in the file are the optical input in expected number of photons per time step dt striking the photodetector.

The output file is identical, with the simulation parameters taking the first ten positions of the binary file, and the remainder of binary file are the detector's response (in terms of electrical charge output) for each time step.

This is *identical* to the input for the web application, except that the input and output are character encoded.
A char is a 1 byte input, and eight bytes are needed for each double.
This is packaged and unpackaged at either end of the process to reproduce the simulation inputs and outputs.
