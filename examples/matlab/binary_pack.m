function [] = binary_pack(filename, config, opticalInput)
%BINARY_PACK writes the inputs for the standalone simspad CLI.
% Produces ./binary/<filename>.json (device parameters) and
% ./binary/<filename>.npy (optical input waveform), so the simulation can be
% run with:
%   simspad -p binary/<filename>.json -i binary/<filename>.npy -o binary/<filename>_out.npy

if ~exist('./binary', 'dir')
    mkdir('./binary');
end

% --- device parameters as JSON ---
paramFile = char(strcat("./binary/", filename, ".json"));
txt = jsonencode(params_struct(config));
fid = fopen(paramFile, 'w');
if fid < 0
    error('binary_pack:open', 'cannot open %s for writing', paramFile);
end
fwrite(fid, txt, 'char');
fclose(fid);

% --- optical input as a 1-D float64 .npy ---
waveFile = char(strcat("./binary/", filename, ".npy"));
write_npy(waveFile, opticalInput);

end
