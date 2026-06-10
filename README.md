# SimSPAD

![logo](https://github.com/WillMatthews/SimSPAD/blob/master/doc/img/simspad_logo.svg)

A high performance avalanche multiplication based optical receiver simulator.

[![Build](https://github.com/WillMatthews/SimSPAD/actions/workflows/makefile.yml/badge.svg)](https://github.com/WillMatthews/SimSPAD/actions/workflows/makefile.yml)
[![Simulator Tests](https://github.com/WillMatthews/SimSPAD/actions/workflows/sim-accuracy.yml/badge.svg)](https://github.com/WillMatthews/SimSPAD/actions/workflows/sim-accuracy.yml)
![version](https://img.shields.io/github/v/tag/WillMatthews/SimSPAD?label=version)

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

SiPMs unfortunately experience recovery-time based nonlinearity, which is due to the recharge period each microcell undergoes after detection of a photon.
Interestingly, there are unobservable parameters inside the device as a result which dictate the shape of the output.
These unobservable parameters include both the array average photon detection efficiency, and the mean fired charge per microcell.
This simulation allows both simulation of SiPMs for arbitrary optical input, and observation of those unobservable parameters.

Simulation also allows for more complicated experiments to be performed in a contained environment where transmitter effects, thermal noise and more can be eliminated.

The simulation was originally written in MATLAB to interface with existing bit error rate detection code, but has since been migrated to c++ to enable the simulator to work (much, oh so much) faster.

## Publications involving this simulation:

This is an **experimental work in progress**, and more documentation will follow. This work has been/will be published in:

1. [PRIME 2022: The negative impact of anode resistance on SiPMs as VLC receivers](https://doi.org/10.1109/PRIME55000.2022.9816749)
2. [MDPI Photonics: An experimental and numerical study of the impact of ambient light of SiPMs in VLC receivers](https://doi.org/10.3390/photonics9120888)
3. [MDPI Sensors: A Roadmap for Gigabit to Terabit Optical Wireless Communications Receivers](https://doi.org/10.3390/s23031101)
4. Upcoming Paper (Pending..? Equaliser based)
5. Upcoming Paper (Pending.?? OFDM based)
6. My Thesis: [Silicon photomultipliers as optical wireless receivers in ambient light](https://ora.ox.ac.uk/objects/uuid:1ca78608-cac4-41a0-8d1e-57fb31603103)

## How to Reference:

If you use this software in your work, please cite it as:
William Matthews (2022) SimSPAD (Which version here) [Source Code]. https://github.com/WillMatthews/SimSPAD

If you use the simulation in a publication, please cite the following papers at your discretion as well:
[MDPI Photonics: An experimental and numerical study of the impact of ambient light of SiPMs in VLC receivers](https://doi.org/10.3390/photonics9120888) and [MDPI Sensors: A Roadmap for Gigabit to Terabit Optical Wireless Communications Receivers](https://doi.org/10.3390/s23031101).

## Usage

### Standalone

![logo](https://github.com/WillMatthews/SimSPAD/blob/master/doc/img/example.gif)

SimSPAD reads the device parameters from a JSON file and the optical input from
a NumPy `.npy` file, then streams the simulation and writes the response to a
`.npy` file:

```
simspad -p params.json -i light.npy -o response.npy
```

The input files can be created using examples in the examples directory (see the
Python helpers in `examples/python/simspad.py`). The simulation streams in
bounded memory, so arbitrarily long traces can be run.

### Web Application

Once SimSPAD server is running, you are able to send a POST request to `http://localhost:33232/simspad`.
The device parameters travel as a JSON object in the `X-SiPM-Params` request
header; the request body is the raw optical-input waveform (little-endian
float64). The reply is streamed back as `application/octet-stream`: the
little-endian float64 charge-per-step response, the same length as the input.
See the File Format section below for details.

To stop the server, access `http://localhost:33232/stop`.
To see if the server is running, access `http://localhost:33232/` where you should see a greeting message in plain text.
Logs (the last 512KB of output to stdout) can be seen at `http://localhost:33232/logs`.

## Install

### Tests
Run `make build` to create the directories for the executables.
Then run `make test` to create and run the tests.

### Standalone
Run `make build` to create the directories for the executables.
Make the executable with `make`. The executable will be produced as `./build/apps/simspad`.

### Web Application
Run `make build` to create the directories for the executables.
Then run `make configure` to download the libraries for the web server.
Finally, make the executable with `make server`. The executable will be produced as `./build/apps/server`.

Create a new user `useradd simspad`
<details>
<summary>Create a systemd file to run the executable </summary>

```systemd
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

The following steps may be omitted if you are not planning on sharing the server over a network with multiple users.

<details>
<summary>Edit Nginx sites-available to proxy pass to the simspad server </summary>

This process means that users are not required to memorise the port number for the server, all you need to do is point your software at the link you define here.
This action also means that you are able to add SSL encryption quite easily with a service like 'lets encrypt'.

Add the following location:

```nginx
    location /whatever-you-want/ {
        # proxy_buffering off;
        proxy_pass http://127.0.0.1:33232/;
    }
```
</details>


<details>
<summary>Edit nginx.conf to increase maximum upload size </summary>

Add the following to the end of `http{}`:

```nginx
    client_max_body_size 200M;
```
</details>

## File Format

SimSPAD separates the (tiny) device parameters from the (bulk) waveform.

The **device parameters** are a flat JSON object:
<details>
<summary>The ten parameters are:</summary>

    dt               - Simulation time step size
    numMicrocell     - Number of Detectors
    vBias            - Bias Voltage
    vBr              - Breakdown Voltage
    tauRecovery      - Recharge time constant
    pdeMax           - Max PDE for PDE-Vover equation
    vChr             - Characteristic Voltage for PDE-Vover equation
    cCell            - Capacitance per detector
    tauFwhm          - Output pulse full width half max time
    digitalThreshold - Detection Threshold (as a fraction of overvoltage from bias)

```json
{
  "dt": 5e-11, "numMicrocell": 5676, "vBias": 3.0, "vBr": 0.0,
  "tauRecovery": 3.08e-08, "pdeMax": 0.46, "vChr": 2.04, "cCell": 1.4e-14,
  "tauFwhm": 0.0, "digitalThreshold": 0.0
}
```
</details>

The **optical input** and the **response** are each a 1-D, little-endian,
float64 NumPy `.npy` array (self-describing: dtype, shape and byte order live in
the file header, so truncation and dtype mismatches are detected rather than
silently misread). The optical input is the expected number of photons per time
step `dt` striking the photo-detector; the response is the electrical charge
output for each time step. Because the transform is length-preserving, both ends
stream in fixed-size chunks in bounded memory.

For the **web application** the parameters travel as that same JSON object in the
`X-SiPM-Params` header, and the waveform is the raw `.npy` *body* without the
header framing: a contiguous little-endian float64 octet-stream, eight bytes per
sample. The response is the same, streamed back chunk-by-chunk.

## Contributing:
Pull requests are extremely welcome, as long as you obey the give key points in the design philosophy:

- Keep SimSPAD fast.
- Keep SimSPAD simple, and all in C++.
- Ensure the output compares well to experimental data.
- Do not use magic numbers - ground everything in device parameters and physical constants.
- Keep SimSPAD fast (again).

