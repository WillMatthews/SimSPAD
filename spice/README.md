# J-Series fast-output equivalent circuit

`jseries_fastout.cir` is an [ngspice](http://ngspice.sourceforge.net/) deck for
the Onsemi **J-Series** SiPM fast-output terminal (values for **MicroFJ-30020**,
overvoltage 2.5 V). It exists to give the `--shape fast` / `--shape bench` pulse
kernels a physical provenance: rather than hand-picking the fast-rail time
constant, you can derive it from a circuit whose component values trace to the
datasheet.

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

## Relation to `--shape fast`

The bipolar shaper added in #28 implements, per unit avalanche charge,

```
h(t) = A·( e^{-t/tauLoad}/tauLoad − e^{-t/tauRecovery}/tauRecovery ),
A = tauRecovery/(tauRecovery − tauLoad),   ∫ h dt = 0
```

The two time constants map onto the circuit as:

* `tauLoad  = RLFAST · N · Cf`  — the fast-rail RC (≈ 2.5 ns for these values);
* `tauRecovery` — the cell's *loaded* recovery constant (array loading slows the
  bare `Rq·(Cd+Cf)`; the fit gives ≈ 24.5 ns here).

So `--shape fast` is a two-pole reduction of this circuit.

## Deriving the kernel (`spice_kernel`)

`src/spice_kernel.cpp` is the spice → kernel pipeline. It runs this deck, fits
the two time constants, checks the analytic kernel against the SPICE pulse, and
writes a ready-to-run `params.json` whose `tauLoad` is calibrated to the
circuit. Build and run from the repository root (needs `ngspice` on PATH):

```
make spice_kernel
./build/apps/spice_kernel
# -> sipm_fast.json
simspad -p sipm_fast.json -i light.npy -o resp.npy --shape fast
```

`tauLoad` is read from the deck's `.param` values (`R_Lfast·N·C_f`), so it can
never drift from the circuit. Sample run:

```
tauLoad     = R_Lfast*N*C_f =   2.500 ns
tauRecovery = loaded fit    =  24.539 ns
peak-normalised shape RMS error (kernel vs spice, <40 ns): 0.088   [PASS]
```

## Running the deck directly

```
ngspice -b spice/jseries_fastout.cir      # writes spice/fastout_tran.txt
```

Tested with ngspice-45. The transient dump is a build artifact (gitignored).
Single-photon checks against the datasheet (run from the deck): FWHM ≈ 1.9 ns
into 50 Ω, positive-lobe charge ≈ `Cf·OV`, near-zero net charge (AC-coupled),
anode charge ≈ `G·e`.
