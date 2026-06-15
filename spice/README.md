# J-Series fast-output equivalent circuit

`jseries_fastout.cir` is an [ngspice](http://ngspice.sourceforge.net/) deck for
the Onsemi **J-Series** SiPM fast-output terminal (values for **MicroFJ-30020**,
overvoltage 2.5 V). It exists to give the output-shaping modes a physical
provenance: rather than hand-picking a pulse shape, you derive it from a circuit
whose component values trace to the datasheet. The `spice_kernel` tool turns
this deck into a tabulated impulse response the simulator convolves directly
(`--shape kernel`), and also fits the cheaper two-pole `--shape fast`
approximation as a cross-check.

## What it models

One fired microcell plus the lumped passive array, per datasheet Fig. 9
(MICROJ-SERIES/D rev. 5):

* an APD junction capacitance `Cd` (+ quench parasitic `Cq`) that self-terminates
  by exactly the overvoltage `OV`, so the avalanche charge is `(Cd+Cq)·OV = G·e`;
* a quench resistor `Rq` that recharges the cell;
* a fast-output coupling capacitor `Cf` from the internal node to a common
  **fast rail** — this is the AC-coupled terminal, hence the bipolar (zero net
  charge) pulse;
* `RLFAST` (50 Ω) and `RLANODE` (10 Ω) output loads.

The avalanche is triggered by a switch at t = 5 ns; the deck runs a 120 ns
transient and writes node voltages `v(f)` (fast) and `v(a)` (anode) to
`spice/fastout_tran.txt` (currents are `v/RL`).

## Two ways to use this pulse

**1. `--shape kernel` — the actual curve (recommended, faithful).**
SimSPAD convolves the avalanche charge train with the *tabulated* fast-output
impulse response taken straight from this deck:

```
g(t) = i_f(t) / Q_av,   Q_av = (Cd+Cq)·OV     (units 1/s)
```

i.e. the fast-terminal current per unit avalanche charge, resampled onto the
simulation `dt` and stored in `fast_kernel.npy`. No functional form is assumed,
so whatever shape your circuit (or a measured pulse) produces is what the
simulator uses. This is the right path for *any* SiPM: change the deck, rerun,
get that device's real fast output.

**2. `--shape fast` — a two-pole reduction (cheaper, approximate).**
The bipolar shaper added in #28 implements, per unit avalanche charge,

```
h(t) = A·( e^{-t/tauLoad}/tauLoad − e^{-t/tauRecovery}/tauRecovery ),
A = tauRecovery/(tauRecovery − tauLoad),   ∫ h dt = 0
```

with the two time constants mapped onto the circuit as:

* `tauLoad  = RLFAST · N · Cf`  — the fast-rail RC (≈ 2.5 ns for these values);
* `tauRecovery` — the cell's *loaded* recovery constant (array loading slows the
  bare `Rq·(Cd+Cf)`; the fit gives ≈ 24.5 ns here).

So `--shape fast` is a two-pole reduction of `--shape kernel`. It's a recursive
one-pole pair (O(1)/sample, streaming), whereas `kernel` is an FIR convolution
with the full pulse (O(taps)/sample, buffers the trace) — pick `fast` when you
need speed and the two-pole shape is good enough, `kernel` when you want the
real curve.

## Deriving the kernel (`spice_kernel`)

`src/spice_kernel.cpp` is the spice → kernel pipeline. It runs this deck,
extracts the single-photon fast pulse, normalises it by the avalanche charge,
resamples it onto the simulation `dt`, and writes `fast_kernel.npy` plus a
ready-to-run `params.json` that points at it (`kernelFile`/`kernelDt`). As a
cross-check it also fits the two-pole `tauLoad`/`tauRecovery` and reports their
RMS error against the SPICE pulse (and writes `tauLoad`, so `--shape fast` works
off the same file). Build and run from the repository root (needs `ngspice` on
PATH):

```
make spice_kernel
./build/apps/spice_kernel
# -> sipm_fast.json + fast_kernel.npy
simspad -p sipm_fast.json -i light.npy -o resp.npy --shape kernel   # the actual curve
simspad -p sipm_fast.json -i light.npy -o resp.npy --shape fast     # two-pole fit
```

`tauLoad` is read from the deck's `.param` values (`R_Lfast·N·C_f`), so it can
never drift from the circuit. Sample run:

```
tauLoad     = R_Lfast*N*C_f =   2.500 ns
tauRecovery = loaded fit    =  24.539 ns
peak-normalised shape RMS error (kernel vs spice, <40 ns): 0.088   [PASS]
kernel: 11500 taps @ dt = 10.0 ps, net charge/avalanche = +6.400e-04 (want ~0)
```

The kernel's net charge is near zero (the fast terminal is AC-coupled); the
small residual is the recovery tail still decaying when the 120 ns transient
ends (≈4.7·tauRecovery), and shrinks if you lengthen the `tran` window.

## Running the deck directly

```
ngspice -b spice/jseries_fastout.cir      # writes spice/fastout_tran.txt
```

Tested with ngspice-45. The transient dump is a build artifact (gitignored).
Single-photon checks against the datasheet (run from the deck): FWHM ≈ 1.9 ns
into 50 Ω, positive-lobe charge ≈ `Cf·OV`, near-zero net charge (AC-coupled),
anode charge ≈ `G·e`.
