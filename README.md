# SimSPAD

![logo](https://github.com/WillMatthews/SimSPAD/blob/master/doc/img/simspad_logo.svg)

A high performance avalanche multiplication based optical receiver simulator.
Version 0.2.0

[![Build](https://github.com/WillMatthews/SimSPAD/actions/workflows/makefile.yml/badge.svg)](https://github.com/WillMatthews/SimSPAD/actions/workflows/makefile.yml)
[![Simulator Tests](https://github.com/WillMatthews/SimSPAD/actions/workflows/sim-accuracy.yml/badge.svg)](https://github.com/WillMatthews/SimSPAD/actions/workflows/sim-accuracy.yml)
[![Spell Check](https://github.com/WillMatthews/SimSPAD/actions/workflows/spelling.yml/badge.svg)](https://github.com/WillMatthews/SimSPAD/actions/workflows/spelling.yml)

## About

**SimSPAD** is an input output converter which takes an input of simulation parameters and an arbitrary length vector of expected number of photons striking the detector array per time step.
From here, the output of a silicon photomultiplier is simulated, and packaged into a binary file.
Currently the software returns the total charge emitted per time step, which can then be pulse-shaped with a convolution further down the line.
Pulse-shaping is being integrated into the software so in the future this step may be omitted.

### SimSPAD does not:
- Do fancy random walks to determine if microcells will fire when an electron-hole pair is generated - this is far too slow and can be encapsulated into the PDE approximation.
- Calculate electrical fields present in the device.
- Have an after-pulsing probability method (although this is easy to implement - I don't have enough time and will welcome pull requests).
- Assume digital driving circuitry (although I will accept pull requests which add this functionality).

## Background on SiPMs, and why simulation is necessary

Silicon Photomultipliers (SiPMs) are solid-state single-photon-sensitive optical receivers.
SiPMs use a vast array of single photon avalanche diodes (SPADs) to enable single photon detection at an exceptionally high count rate.

For the purposes of my DPhil, I investigated using SiPMs with application of high-performance receivers for optical wireless communications.
SiPMs consequently have turned out to be extremely good detectors as they have a high bandwidth, high gain, and large area when contrasted with other silicon based photo-detectors.

SiPMs unfortunately experience recovery-time based nonlinearity, which is due to the recharge period each microcell undergoes after detection oh a photon.
Interestingly, there are unobservable parameters inside the device as a result which dictate the shape of the output.
These unobservable parameters include both the array average photon detection efficiency, and the mean fired charge per microcell.
This simulation allows both simulation of SiPMs for arbitrary optical input, and observation of those unobservable parameters.

Simulation also allows for more complicated experiments to be performed in a contained environment where transmitter effects, thermal noise and more can be eliminated.

The simulation was originally written in MATLAB to interface with existing bit error rate detection code, but has since been migrated to c++ to enable the simulator to work (much, oh so much) faster.


## Publications involving this simulation:

This is an **experimental work in progress**, and more documentation will follow. This work has been/will be published in:

1. [PRIME 2022: The negative impact of anode resistance on SiPMs as VLC receivers](https://doi.org/10.1109/PRIME55000.2022.9816749)
2. [MDPI Photonics: An experimental and numerical study of the impact of ambient light of SiPMs in VLC receivers](https://doi.org/10.3390/photonics9120888)
3. Upcoming Paper (Written... Parameter variation based)
4. Upcoming Paper (Pending... Equaliser based)
5. Upcoming Paper (Pending... OFDM based)
6. My Thesis (Pending...)

## How to Reference:

If you have used this software for your work, please reference it as follows:
William Matthews (2022) SimSPAD (Version 0.1) [Source Code]. Private Distribution

Eventually, when this repo is made public, you may cite as:
William Matthews (2022) SimSPAD (Version 0.1) [Source Code]. https://github.com/WillMatthews/SimSPAD


## Usage

### Standalone

![logo](https://github.com/WillMatthews/SimSPAD/blob/master/doc/img/example.gif)

SimSPAD can be run as a standalone program from the command line, taking an input binary file, and producing an output binary file.
This binary file can be created using examples in the examples directory. Currently MATLAB and Python methods exist, and a C++ method will be added in the future.

### Web Application

Once SimSPAD server is running, you are able to send a POST request to `http://localhost:33232/simspad`.
To stop the server, access `http://localhost:33232/stop`.
To see if the server is running, access `http://localhost:33232/` where you should see a greeting message in plain text.
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
Description=SimSPAD Avalanche Photo-detector Simulator
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
        digitalThreshold - Detection Threshold (as a fraction of overvoltage from bias)

</details>

And the remainder of doubles in the file are the optical input in expected number of photons per time step dt striking the photo-detector.

The output file is identical, with the simulation parameters taking the first ten positions of the binary file, and the remainder of binary file are the detector's response (in terms of electrical charge output) for each time step.

This is *identical* to the input for the web application, except that the input and output are character encoded.
A char is a 1 byte input, and eight bytes are needed for each double.
This is packaged and unpackaged at either end of the process to reproduce the simulation inputs and outputs.

## Contributing:
Pull requests are extremely welcome, as long as you obey the give key points in the design philosophy:

- Keep SimSPAD fast.
- Keep SimSPAD simple, and all in C++.
- Ensure the output compares well to experimental data.
- Do not use magic numbers - ground everything in device parameters and physical constants.
- Keep SimSPAD fast (again).
