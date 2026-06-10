function [] = write_npy(filename, vec)
%WRITE_NPY writes a vector as a 1-D little-endian float64 NumPy .npy file.
% This is the optical-input / response waveform format read and written by the
% simspad CLI (simspad -p params.json -i light.npy -o out.npy).

vec = double(vec(:)); % force a column of doubles
n = numel(vec);

% Header dictionary (note: '' is a literal single quote in MATLAB strings).
hdr = sprintf('{''descr'': ''<f8'', ''fortran_order'': False, ''shape'': (%d,), }', n);

% Pad with spaces so the whole preamble (6 magic + 2 version + 2 len + header,
% including the trailing newline) is a multiple of 64 bytes.
preamble = 10 + numel(hdr) + 1; % +1 for the trailing newline
pad = mod(64 - mod(preamble, 64), 64);
hdr = [hdr, repmat(' ', 1, pad), char(10)]; % char(10) == '\n'
hlen = numel(hdr);

fid = fopen(filename, 'w');
if fid < 0
    error('write_npy:open', 'cannot open %s for writing', filename);
end
fwrite(fid, 147, 'uint8');                 % \x93
fwrite(fid, uint8('NUMPY'), 'uint8');
fwrite(fid, [1 0], 'uint8');               % format version 1.0
fwrite(fid, mod(hlen, 256), 'uint8');      % header length, little-endian uint16
fwrite(fid, floor(hlen / 256), 'uint8');
fwrite(fid, uint8(hdr), 'uint8');
fwrite(fid, vec, 'float64', 0, 'ieee-le'); % the array body
fclose(fid);

end
