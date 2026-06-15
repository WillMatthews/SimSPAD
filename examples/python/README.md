# Python Examples

Helpers for SimSPAD's self-describing IO and for building output-shaping
kernels.

| File | What it is |
|---|---|
| `simspad.py` | Importable helpers: write/read device parameters (JSON) and waveforms (`.npy`), and a small web client. |
| `example.py` | End-to-end usage: configure a device, simulate, save the params + waveforms. |
| `pulse_to_kernel.py` | Turn a **measured or simulated single-PE fast-output pulse** into a `--shape kernel` kernel (see below). |

## Bring your own SiPM: the `kernel` shape mode

SimSPAD's core Monte-Carlo models the **avalanche statistics** of *your* device
from its parameters (`numMicrocell`, `vBr`, `tauRecovery`, `pdeMax`, `vChr`,
`cCell`). On top of that, `--shape` filters the output to mimic a real readout.
The `kernel` mode is the faithful, device-specific one: it convolves the
avalanche charge train with a **tabulated fast-output impulse response** — the
actual shape of your terminal — instead of a built-in approximation.

`pulse_to_kernel.py` builds that kernel from one single-photoelectron (1 p.e.)
fast-output pulse. The pulse can come from **a bench capture** or **a SPICE
run** — the tool only needs a `time, signal` table.

### What the kernel is

The kernel `g(t)` is the fast-output **current per unit avalanche charge**
(units 1/s):

```
g(t) = i_f(t) / Q_avalanche
```

`i_f` is the fast-output current (`= v / R_load` if you captured a voltage), and
`Q_avalanche` is the charge one fired microcell deposits. Convolving `g` with
SimSPAD's charge-per-step train then gives physical fast-output current. One
captured 1-PE pulse *is* one avalanche, so dividing by `Q_avalanche` is the
entire normalisation — the measured amplitude already carries the AC-coupling
fraction and the rail RC of the real terminal.

You supply `Q_avalanche` one of three ways (most → least self-consistent):

* `--device params.json` → `Q = (vBias − vBr) · cCell`. **Recommended:** this is
  the *same* charge SimSPAD assigns a fired cell, so the convolution units match
  the simulation exactly.
* `--gain G` → `Q = G · e` (datasheet gain × electron charge).
* `--charge C` → `Q = C` directly (Coulombs).

> Note: normalising by the device's own `(vBias−vBr)·cCell` rather than a
> circuit's avalanche charge matters when the two disagree (e.g. a SPICE deck
> whose junction capacitance differs from the model `cCell`). The `--device`
> path keeps the kernel consistent with the simulation it feeds.

### Example A — bench capture

Export an oscilloscope trace of a single dark-count fast-output pulse as CSV
(`time, voltage`, here in nanoseconds across a 50 Ω load):

```bash
python pulse_to_kernel.py \
    --pulse single_pe.csv --time-unit ns \
    --signal-units voltage --load 50 \
    --device j30020.json --params-out j30020_kernel.json
simspad -p j30020_kernel.json -i light.npy -o resp.npy --shape kernel
```

This writes `fast_kernel.npy` and a copy of `j30020.json` with `kernelFile` /
`kernelDt` added. `--dt` defaults to the device's `dt`; the onset is
auto-detected (override with `--t0`).

### Example B — SPICE deck

Run a fast-output equivalent circuit (see `spice/README.md` for the shipped
J-Series deck and how to adapt it to your device), then feed its `wrdata` dump
straight in — `pulse_to_kernel.py` reads whitespace tables and skips headers, so
no conversion step is needed:

```bash
ngspice -b spice/jseries_fastout.cir          # -> spice/fastout_tran.txt (t v(f) t v(a))
python pulse_to_kernel.py \
    --pulse spice/fastout_tran.txt --time-col 0 --signal-col 1 \
    --signal-units voltage --load 50 \
    --device j30020.json --params-out j30020_kernel.json
simspad -p j30020_kernel.json -i light.npy -o resp.npy --shape kernel
```

### Options

`--help` lists everything. The common ones:

| flag | meaning |
|---|---|
| `--pulse PATH` | trace file: CSV / whitespace / ngspice `wrdata` |
| `--time-col N`, `--signal-col N` | 0-based column indices (default 0, 1) |
| `--time-unit {s,ms,us,ns,ps}` | unit of the time column (default `s`) |
| `--signal-units {current,voltage}` | `voltage` needs `--load OHMS` |
| `--device / --gain / --charge` | the `Q_avalanche` normalisation (pick one) |
| `--dt SECONDS` | kernel sample step (default: device `dt`) |
| `--t0 SECONDS` | avalanche onset (default: auto-detect) |
| `--kernel-out`, `--params-out` | output paths |

### Verify the maths

```bash
python pulse_to_kernel.py --selftest
```

Round-trips an analytic bipolar pulse through the tool and checks the kernel
reproduces it (and integrates to ~0). This mirrors the C++ `make test` shaping
checks.

## The C++ `spice_kernel` tool

`src/spice_kernel.cpp` (`make spice_kernel`) is a one-shot C++ pipeline
specialised to the shipped J-Series deck: it runs ngspice, emits the kernel, and
*also* fits the cheaper two-pole `--shape fast` approximation as a cross-check.
`pulse_to_kernel.py` is the general, source-agnostic path — prefer it for an
arbitrary device or a bench measurement.
