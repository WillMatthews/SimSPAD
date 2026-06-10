"""Python helpers for SimSPAD's self-describing IO.

Local files: device parameters as a flat JSON object, the optical-input and
response waveforms as 1-D little-endian float64 NumPy ``.npy`` arrays. Web:
parameters as a JSON ``X-SiPM-Params`` request header, the waveform as a raw
``application/octet-stream`` body (little-endian float64); the response is the
same, charge-per-step, one value per time step.
"""
import json

import numpy as np

# Parameter order matches SiPM::dump_configuration() / the .json schema.
PARAM_KEYS = [
    "dt", "numMicrocell", "vBias", "vBr", "tauRecovery",
    "pdeMax", "vChr", "cCell", "tauFwhm", "digitalThreshold",
]


class SiPM:
    def __init__(self, *args):
        # SiPM(seq_of_10) or SiPM(dt, numMicrocell, vBias, ...) (10 positional).
        vals = list(args[0]) if len(args) == 1 else list(args)
        if len(vals) != len(PARAM_KEYS):
            raise ValueError(f"expected {len(PARAM_KEYS)} parameters, got {len(vals)}")
        for key, val in zip(PARAM_KEYS, vals):
            setattr(self, key, val)

    # -- parameters (JSON) --------------------------------------------------
    def params_dict(self):
        d = {k: getattr(self, k) for k in PARAM_KEYS}
        d["numMicrocell"] = int(d["numMicrocell"])
        return d

    def write_params(self, filename):
        """Write the device parameters to a flat JSON file."""
        with open(filename, "w") as f:
            json.dump(self.params_dict(), f, indent=2)

    @classmethod
    def from_params(cls, filename):
        """Construct a SiPM from a JSON parameter file."""
        with open(filename) as f:
            d = json.load(f)
        return cls([d[k] for k in PARAM_KEYS])

    # -- waveforms (.npy) ---------------------------------------------------
    @staticmethod
    def write_waveform(filename, optical_input):
        """Write an optical-input waveform as a 1-D little-endian float64 .npy."""
        np.save(filename, np.ascontiguousarray(optical_input, dtype="<f8"))

    # -- web client ---------------------------------------------------------
    def simulate_web(self, url, optical_input):
        """POST a waveform to a SimSPAD server; return the response as an ndarray.

        Parameters travel in the ``X-SiPM-Params`` JSON header, the waveform as
        a raw little-endian float64 octet-stream body. The response body is the
        charge-per-step waveform (same length as the input)."""
        import requests

        body = np.ascontiguousarray(optical_input, dtype="<f8").tobytes()
        headers = {
            "X-SiPM-Params": json.dumps(self.params_dict()),
            "Content-Type": "application/octet-stream",
        }
        response = requests.post(url, data=body, headers=headers)
        response.raise_for_status()
        return np.frombuffer(response.content, dtype="<f8")


def read_waveform(filename):
    """Read a 1-D float64 .npy waveform written by SimSPAD."""
    return np.load(filename)


def read_params(filename):
    """Read a JSON parameter file, returning a configured SiPM."""
    return SiPM.from_params(filename)
