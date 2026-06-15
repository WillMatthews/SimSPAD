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

## Included decks

All are Onsemi J-Series (MICROJ-SERIES/D rev.7), with inputs from the datasheet
in `docs/datasheets/microj-series-datasheet.pdf` (Tables 1–3, gain quoted at
OV = +2.5 V). Each deck writes both the fast (`v(f)`) and standard/anode (`v(a)`)
outputs, so one deck serves both terminals.

| Deck | Device | Cells | Recharge τ | Fast cap | Provenance |
|---|---|---|---|---|---|
| `jseries_fastout.cir` | MicroFJ-30020 (3 mm, 20 µm) | 14410 | — | 50 pF | Bench-calibrated; the validation anchor |
| `microfj_30035_fastout.cir` | MicroFJ-30035 (3 mm, 35 µm) | 5676 | 45 ns | 40 pF | **Datasheet** |
| `microfj_40035_fastout.cir` | MicroFJ-40035 (4 mm, 35 µm) | 9260 | 48 ns | 70 pF | **Datasheet** |
| `microfj_60035_fastout.cir` | MicroFJ-60035 (6 mm, 35 µm) | 22292 | 50 ns | 160 pF | **Datasheet** |
| `template_fastout.cir` | **fast-output template** | — | — | — | Fill in datasheet inputs; internals derived |
| `template_stdout.cir` | **standard-output template** | — | — | — | For devices with NO fast pin (e.g. Hamamatsu MPPCs) |

The datasheet's own consistency is a nice check on the model: with Cμ = G·e/OV =
185.9 fF, the array capacitance N·Cμ reproduces the datasheet anode capacitances
to 96–100 % (60035: 4143 vs 4140 pF).

### Fidelity: fast output vs standard output

The **standard (anode) output** is governed by the datasheet recharge τ and is
well-matched. The **fast output** is harder: the simple `N·Cf` fast-rail lumping
*over-broadens* it, and the error grows with array size — the decks give FWHM
~1.8 / 2.7 / 5.4 ns for the 30035 / 40035 / 60035 vs the datasheet's 1.5 / 1.7 /
3.0 ns. So for an **exact fast output**, don't rely on the lumped circuit:
**digitise the datasheet's measured pulse** (Figs. 5 and 6 give the fast and
standard shapes for all three) and feed that curve straight to
`pulse_to_kernel.py`. That is the whole advantage of the tabulated-kernel
approach — a measured curve always beats a model.

## Your own device: run SPICE → kernel (the general path)

For any device, the recommended route is the source-agnostic
`examples/python/pulse_to_kernel.py`, which turns *any* 1-PE fast-output trace
(a SPICE `wrdata` dump **or** a bench oscilloscope capture) into a kernel. The
SPICE half is just "run a deck that writes the fast node":

1. **Make the deck.** Copy `template_fastout.cir` and fill in its *datasheet
   inputs* block — gain, overvoltage, breakdown, cell count `NCELL`, recharge τ,
   total fast capacitance, and the load `RLFAST`. Everything internal (`Cd`,
   `Cq`, `Rq`, `Cf`) is **derived** from those via `.param` expressions, so you
   never hand-pick a component value. Point the `wrdata <file> v(f)` line at your
   own output path. Fire one cell with the `VFIRE` switch; the exact trigger
   time doesn't matter (the tool auto-detects the onset). Set the `tran` window
   to ≳ 8 × recharge τ so the AC-coupled pulse fully returns to zero.

2. **Run it** and feed the dump in:

   ```bash
   ngspice -b spice/mydevice.cir            # -> writes your wrdata file
   python examples/python/pulse_to_kernel.py \
       --pulse spice/mydevice_tran.txt --time-col 0 --signal-col 1 \
       --signal-units voltage --load 50 \
       --device mydevice.json --params-out mydevice_kernel.json
   simspad -p mydevice_kernel.json -i light.npy -o resp.npy --shape kernel
   ```

`mydevice.json` is your device's parameter file (the avalanche-model parameters
SimSPAD simulates — SPICE only supplies the *terminal shape*). See
`examples/python/README.md` for the full option list, the bench-capture path,
and the normalisation choices.

### Standard ("slow") output instead of the fast output

The standard/anode output is what most SiPMs are read on (the fast terminal is a
specialised extra pin). Every fast-output deck already writes the anode node
`v(a)` as its second `wrdata` column, so you build a **slow kernel** from the
*same* run — just point the tool at the anode column and load:

```bash
python examples/python/pulse_to_kernel.py \
    --pulse spice/mydevice_tran.txt --time-col 0 --signal-col 3 \
    --signal-units voltage --load 10 \
    --device mydevice.json --params-out mydevice_slow.json
```

(`wrdata v(f) v(a)` lays the columns out as `t v(f) t v(a)`, so the anode is
column 3; `--load 10` is `RLANODE`.) For a device with **no fast pin** at all,
start from `template_stdout.cir`, which models the standard output directly
(single `wrdata v(a)` column → `--signal-col 1`).

Unlike the AC-coupled fast output (bipolar, net charge ≈ 0), the standard output
is **DC-coupled** — unipolar, carrying the full avalanche charge, so its kernel's
net charge ≈ 1. `pulse_to_kernel.py` reports which regime it detected.

> Security note: an ngspice deck can run arbitrary shell commands (via
> `.control` `shell`/`system`), so only run decks you trust — and never wire
> "upload a deck" into the web server. The bench-capture path takes only numbers
> and has no such surface.
