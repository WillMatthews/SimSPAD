#!/usr/bin/env python3
"""Turn a measured (or simulated) single-photon fast-output pulse into a
SimSPAD ``--shape kernel`` kernel.

SimSPAD's ``kernel`` shape mode convolves the avalanche charge train with a
*tabulated* fast-output impulse response, so the simulated output carries the
real shape of *your* device's terminal instead of a two-pole approximation.
This tool builds that kernel from a single-photoelectron (1 p.e.) pulse, taken
from any source:

  * a **bench capture** -- an oscilloscope trace of one dark-count / single-PE
    fast-output pulse, exported as CSV;
  * a **SPICE run** -- ``ngspice -b your_device.cir`` writing a ``wrdata`` table
    of the fast-output node (see ``spice/README.md``);
  * any 2-column ``time, signal`` table.

What the kernel is
------------------
The kernel ``g(t)`` is the fast-output **current per unit avalanche charge**
(units 1/s)::

    g(t) = i_f(t) / Q_avalanche

where ``i_f`` is the fast-output current (``= v / R_load`` if you captured a
voltage across a load) and ``Q_avalanche`` is the charge one fired microcell
deposits. Convolving ``g`` with SimSPAD's avalanche charge-per-step train
(Coulombs/step) then yields physical fast-output current (A). One captured 1-PE
pulse *is* one avalanche, so dividing by ``Q_avalanche`` is the whole
normalisation -- the measured amplitude already carries the AC-coupling fraction
and the rail RC of the real terminal.

``Q_avalanche`` can be given three ways (most to least self-consistent):

  * ``--device params.json`` -> ``Q = (vBias - vBr) * cCell`` (the *same* charge
    SimSPAD assigns a fired cell, so the convolution units match exactly);
  * ``--gain G``             -> ``Q = G * e`` (datasheet gain x electron charge);
  * ``--charge C``           -> ``Q = C`` (Coulombs, if you know it directly).

Output
------
Writes ``fast_kernel.npy`` (1-D float64, the kernel sampled at the simulation
``dt``) and, when ``--device`` is given, an augmented parameter file with
``kernelFile``/``kernelDt`` added, ready to run::

    python pulse_to_kernel.py --pulse single_pe.csv --signal-units voltage \
        --load 50 --device j30020.json --params-out j30020_kernel.json
    simspad -p j30020_kernel.json -i light.npy -o resp.npy --shape kernel

Run ``python pulse_to_kernel.py --selftest`` to verify the maths against an
analytic bipolar pulse, or ``--help`` for the full option list.
"""
import argparse
import sys

import numpy as np

try:
    from simspad import read_params  # augment an existing device parameter file
except ImportError:  # allow running from outside examples/python/
    read_params = None

ELECTRON_CHARGE = 1.602176634e-19  # C

# Time-unit suffixes accepted for --time-unit and recognised in column headers.
TIME_UNITS = {"s": 1.0, "ms": 1e-3, "us": 1e-6, "ns": 1e-9, "ps": 1e-12}


def load_trace(path, time_col=0, signal_col=1):
    """Read two numeric columns from a CSV / whitespace / ngspice-wrdata table.

    Lines that are blank or whose first column is non-numeric (headers, SPICE
    comment lines beginning ``*`` or ``#``) are skipped, so the same reader
    handles a scope CSV with a header row and a raw ngspice ``wrdata`` dump.
    Comma-separated files are detected automatically; everything else is split
    on whitespace.
    """
    times, signals = [], []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line[0] in "#*;":
                continue
            fields = line.split(",") if "," in line else line.split()
            if max(time_col, signal_col) >= len(fields):
                continue
            try:
                t = float(fields[time_col])
                s = float(fields[signal_col])
            except ValueError:
                continue  # header / non-numeric row
            times.append(t)
            signals.append(s)
    if len(times) < 4:
        raise ValueError(
            f"{path}: found only {len(times)} numeric rows in columns "
            f"({time_col}, {signal_col}) -- check --time-col/--signal-col"
        )
    return np.asarray(times, dtype=float), np.asarray(signals, dtype=float)


def detect_onset(t, i_f, frac=0.01):
    """Estimate the avalanche onset time: the foot of the rising edge.

    Finds the positive peak and walks back to the last sample below
    ``frac * peak``. Override with ``--t0`` if your trace has a pre-trigger
    offset or a non-standard baseline.
    """
    peak_idx = int(np.argmax(i_f))
    peak = i_f[peak_idx]
    if peak <= 0:
        return t[0]
    thresh = frac * peak
    onset_idx = 0
    for k in range(peak_idx, -1, -1):
        if i_f[k] < thresh:
            onset_idx = k
            break
    return t[onset_idx]


def pulse_to_kernel(t, signal, q_avalanche, dt, t0=None, load=None, onset_frac=0.01):
    """Build a SimSPAD kernel from a single-PE fast-output pulse.

    Parameters
    ----------
    t, signal : 1-D arrays
        Time [s] and the captured fast-output signal (current, or voltage if
        ``load`` is given).
    q_avalanche : float
        Charge one fired microcell deposits [C]; the normalisation.
    dt : float
        Simulation time step [s] to sample the kernel at (== ``kernelDt``).
    t0 : float or None
        Avalanche onset time [s]; auto-detected when None.
    load : float or None
        Load resistance [ohm]; if given, ``signal`` is a voltage and the
        current is ``signal / load``.
    onset_frac : float
        Fraction-of-peak threshold for onset auto-detection.

    Returns
    -------
    kernel : 1-D float64 array
        ``g(t) = i_f(t) / q_avalanche`` resampled onto the ``dt`` grid from the
        onset (units 1/s).
    info : dict
        Diagnostics: ``t0``, ``n_taps``, ``net_charge`` (per unit avalanche
        charge; ~0 for an AC-coupled terminal), ``fwhm`` of the positive lobe.
    """
    t = np.asarray(t, dtype=float)
    i_f = np.asarray(signal, dtype=float)
    if load is not None:
        if load <= 0:
            raise ValueError("--load must be > 0 ohm")
        i_f = i_f / load
    order = np.argsort(t)  # tolerate unsorted / non-monotone exports
    t, i_f = t[order], i_f[order]

    if t0 is None:
        t0 = detect_onset(t, i_f, onset_frac)
    if t0 >= t[-1]:
        raise ValueError(f"onset t0={t0:g}s is at/after the trace end {t[-1]:g}s")
    if q_avalanche <= 0:
        raise ValueError("avalanche charge must be > 0 C")
    if dt <= 0:
        raise ValueError("dt must be > 0 s")

    # Resample i_f onto a uniform grid at the simulation dt, measured from the
    # onset; samples past the trace end hold the last value (the tail).
    span = t[-1] - t0
    n = int(np.floor(span / dt)) + 1
    grid = np.arange(n) * dt
    i_grid = np.interp(grid + t0, t, i_f, left=0.0, right=i_f[-1])
    kernel = i_grid / q_avalanche

    net = float(np.sum(kernel) * dt)  # net charge per unit avalanche charge
    info = {
        "t0": float(t0),
        "n_taps": int(n),
        "net_charge": net,
        "fwhm": _positive_fwhm(grid, kernel),
        "peak": float(np.max(kernel)),
    }
    return kernel.astype("<f8"), info


def _positive_fwhm(t, y):
    """FWHM [s] of the positive lobe, by half-maximum crossings."""
    pk = float(np.max(y))
    if pk <= 0:
        return 0.0
    above = np.where(y >= pk / 2)[0]
    if above.size < 2:
        return 0.0
    return float(t[above[-1]] - t[above[0]])


def resolve_avalanche_charge(args, device_params):
    """Pick Q_avalanche [C] from --charge / --gain / --device (in that order)."""
    if args.charge is not None:
        return args.charge, "explicit --charge"
    if args.gain is not None:
        return args.gain * ELECTRON_CHARGE, f"--gain {args.gain:g} x e"
    if device_params is not None:
        v_over = device_params["vBias"] - device_params["vBr"]
        q = v_over * device_params["cCell"]
        return q, "(vBias - vBr) * cCell from --device"
    raise SystemExit(
        "error: need an avalanche charge -- pass one of --device, --gain or --charge"
    )


def _read_device(path):
    """Load a device parameter JSON as a plain dict (no SimSPAD import needed)."""
    import json

    with open(path) as f:
        return json.load(f)


def main(argv=None):
    p = argparse.ArgumentParser(
        description="Build a SimSPAD --shape kernel from a single-PE fast-output pulse.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        epilog="See examples/python/README.md and spice/README.md for worked examples.",
    )
    p.add_argument("--pulse", help="trace file: CSV / whitespace / ngspice wrdata")
    p.add_argument("--time-col", type=int, default=0, help="0-based time column")
    p.add_argument("--signal-col", type=int, default=1, help="0-based signal column")
    p.add_argument("--time-unit", choices=sorted(TIME_UNITS), default="s",
                   help="unit of the time column")
    p.add_argument("--signal-units", choices=["current", "voltage"], default="current",
                   help="captured signal: current [A], or voltage [V] (needs --load)")
    p.add_argument("--load", type=float, default=None,
                   help="load resistance [ohm], required for --signal-units voltage")

    norm = p.add_argument_group("normalisation (pick one)")
    norm.add_argument("--device", help="device params JSON; derives Q = (vBias-vBr)*cCell and dt")
    norm.add_argument("--gain", type=float, default=None, help="device gain G (Q = G*e)")
    norm.add_argument("--charge", type=float, default=None, help="avalanche charge [C] directly")

    p.add_argument("--dt", type=float, default=None,
                   help="simulation dt [s] to sample the kernel at (default: device dt)")
    p.add_argument("--t0", type=float, default=None,
                   help="avalanche onset time [s] (default: auto-detect)")
    p.add_argument("--onset-frac", type=float, default=0.01,
                   help="fraction-of-peak threshold for onset auto-detection")
    p.add_argument("--kernel-out", default="fast_kernel.npy", help="kernel .npy output path")
    p.add_argument("--params-out", default=None,
                   help="write an augmented device params JSON here (requires --device)")
    p.add_argument("--selftest", action="store_true",
                   help="verify against an analytic bipolar pulse and exit")
    args = p.parse_args(argv)

    if args.selftest:
        return _selftest()

    if not args.pulse:
        p.error("--pulse is required (or use --selftest)")
    if args.signal_units == "voltage" and args.load is None:
        p.error("--signal-units voltage requires --load OHMS")

    device_params = _read_device(args.device) if args.device else None
    q_av, q_src = resolve_avalanche_charge(args, device_params)

    dt = args.dt
    if dt is None:
        if device_params is None:
            p.error("--dt is required when --device is not given")
        dt = device_params["dt"]

    t_raw, sig = load_trace(args.pulse, args.time_col, args.signal_col)
    t = t_raw * TIME_UNITS[args.time_unit]
    load = args.load if args.signal_units == "voltage" else None

    kernel, info = pulse_to_kernel(
        t, sig, q_av, dt, t0=args.t0, load=load, onset_frac=args.onset_frac
    )
    np.save(args.kernel_out, kernel)
    if not args.kernel_out.endswith(".npy"):
        # np.save appends .npy; keep our reported name honest.
        args.kernel_out += ".npy"

    print("=== pulse -> --shape kernel ===")
    print(f"  source pulse : {args.pulse}  ({len(t)} samples)")
    print(f"  Q_avalanche  : {q_av:.4e} C   [{q_src}]")
    print(f"  onset t0     : {info['t0'] * 1e9:.3f} ns"
          + ("  (auto)" if args.t0 is None else "  (given)"))
    print(f"  kernel       : {info['n_taps']} taps @ dt = {dt * 1e12:.1f} ps")
    print(f"  positive FWHM: {info['fwhm'] * 1e9:.3f} ns")
    print(f"  net charge/avalanche = {info['net_charge']:+.3e}  (want ~0: AC-coupled)")
    print(f"  wrote {args.kernel_out}")

    if args.params_out:
        if args.device is None:
            p.error("--params-out requires --device (the params to augment)")
        _write_augmented_params(args.device, args.params_out, args.kernel_out, dt)
        print(f"  wrote {args.params_out}")
        print(f"  run: simspad -p {args.params_out} -i light.npy -o resp.npy --shape kernel")
    elif args.device:
        print(f'  add to {args.device}:  "kernelFile": "{args.kernel_out}", "kernelDt": {dt:g}')
    return 0


def _write_augmented_params(device_in, params_out, kernel_file, kernel_dt):
    """Copy a device params JSON, adding kernelFile/kernelDt."""
    if read_params is not None:
        sipm = read_params(device_in)
        sipm.kernelFile = kernel_file
        sipm.kernelDt = kernel_dt
        sipm.write_params(params_out)
    else:  # standalone fallback: edit the JSON dict directly
        import json

        with open(device_in) as f:
            d = json.load(f)
        d["kernelFile"] = kernel_file
        d["kernelDt"] = kernel_dt
        with open(params_out, "w") as f:
            json.dump(d, f, indent=2)


def _selftest():
    """Round-trip an analytic bipolar pulse through pulse_to_kernel().

    Samples h(t) = A*(e^{-t/tauL}/tauL - e^{-t/tauR}/tauR) at fine resolution,
    treats it as a measured current trace, and checks the produced kernel
    reproduces h(t) (after the dt resampling) and integrates to ~0.
    """
    dt = 1e-11
    tauL, tauR = 2.0e-9, 24.5e-9
    A = tauR / (tauR - tauL)
    q_av = 1.0  # normalise to 1 so the kernel equals h directly

    # Fine "measured" trace with a 5 ns pre-trigger baseline.
    t = np.arange(0, 200e-9, dt / 3)
    t0 = 5e-9
    u = t - t0
    h = np.where(u >= 0, A * (np.exp(-u / tauL) / tauL - np.exp(-u / tauR) / tauR), 0.0)

    kernel, info = pulse_to_kernel(t, h, q_av, dt)  # onset auto-detected

    # Expected kernel: h at the SAME absolute sample times the tool used
    # (grid measured from the *detected* onset), so the comparison is not thrown
    # off by the sub-sample onset-detection offset at the sharp peak.
    grid = np.arange(info["n_taps"]) * dt
    want_u = (grid + info["t0"]) - t0  # time relative to the true onset
    want = np.where(
        want_u >= 0, A * (np.exp(-want_u / tauL) / tauL - np.exp(-want_u / tauR) / tauR), 0.0
    )
    rel = np.max(np.abs(kernel - want)) / np.max(np.abs(want))

    onset_err = abs(info["t0"] - t0)
    ok_shape = rel < 1e-2
    ok_onset = onset_err < 0.2e-9
    ok_net = abs(info["net_charge"]) < 5e-3
    print("=== pulse_to_kernel self-test ===")
    print(f"  onset detected : {info['t0'] * 1e9:.3f} ns (want 5.000)   "
          f"{'PASS' if ok_onset else 'FAIL'}")
    print(f"  shape rel err  : {rel:.2e} (want < 1e-2)               "
          f"{'PASS' if ok_shape else 'FAIL'}")
    print(f"  net charge     : {info['net_charge']:+.2e} (want ~0)         "
          f"{'PASS' if ok_net else 'FAIL'}")
    ok = ok_shape and ok_onset and ok_net
    print("  RESULT:", "PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
