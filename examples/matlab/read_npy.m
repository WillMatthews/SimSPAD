function vec = read_npy(filename)
%READ_NPY reads a 1-D little-endian float64 NumPy .npy file into a column vector.
% Matches the waveform format written by the simspad CLI.

fid = fopen(filename, 'r');
if fid < 0
    error('read_npy:open', 'cannot open %s', filename);
end

magic = fread(fid, 6, 'uint8=>uint8');
if numel(magic) ~= 6 || ~isequal(magic(2:6)', uint8('NUMPY'))
    fclose(fid);
    error('read_npy:format', '%s is not a .npy file', filename);
end

ver = fread(fid, 2, 'uint8=>uint8');
if ver(1) == 1
    hlen = fread(fid, 1, 'uint16', 0, 'ieee-le');
else
    hlen = fread(fid, 1, 'uint32', 0, 'ieee-le');
end

% Skip the header dictionary (we assume a 1-D '<f8' array).
fread(fid, hlen, 'uint8=>uint8');

vec = fread(fid, Inf, 'float64=>double', 0, 'ieee-le');
fclose(fid);

end
