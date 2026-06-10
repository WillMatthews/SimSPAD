# MATLAB Examples

MATLAB requires a few more functions than the Python example, however a full
working example is presented in `example.m`.

Helpers:
- `make_config.m` / `make_calibrated_input.m` — build the device config and the
  calibrated optical-input waveform.
- `params_struct.m` — map the config to the SimSPAD JSON parameter schema.
- `write_npy.m` / `read_npy.m` — write/read 1-D little-endian float64 `.npy`
  waveforms (the simspad CLI's input/output format).
- `binary_pack.m` — write `<name>.json` + `<name>.npy` for the standalone CLI;
  `binary_unpack.m` — read a `<name>.npy` response.
- `simspad_server.m` — run a simulation over HTTP (JSON params in the
  `X-SiPM-Params` header, waveform as the octet-stream body).

> Note: the `.npy` writer assumes a little-endian host (all current MATLAB
> platforms). These scripts were updated alongside the new file format but have
> not been re-run in MATLAB; please report any issues.