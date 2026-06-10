function opticalOutput = binary_unpack(filename)
%BINARY_UNPACK reads a response waveform written by the simspad CLI.
% Reads ./binary/<filename>.npy (1-D float64 charge-per-step) into a vector.
% Device parameters are no longer embedded in the waveform file -- they live in
% the separate .json parameter file the simulation was run with.

waveFile = char(strcat("./binary/", filename, ".npy"));
opticalOutput = read_npy(waveFile);

end
