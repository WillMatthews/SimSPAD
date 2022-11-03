function [config, opticalOutput] = binary_unpack(filename)
%PACKAGE_TO_BINARY retrieves a SiPM configuration and vector from a binary file
% takes as arguments a filename as a string

fname = strcat("./binary/", filename, ".bin");
fileID = fopen(fname,'r');
x = fread(fileID, 'double');
fclose(fileID);

%% Settings for Simulation
%config.samplerate;
%config.numsample;
%config.num_microcells_to_store;
%config.photonE;
%config.A_sipm;

config.dt = x(1);
config.num_microcell = x(2);
config.vbias = x(3);
config.vbr = x(4);
config.recovery = x(5);
config.pde_Max = x(6);
config.pde_vchr = x(7);
config.ccell = x(8);
config.pulse_fwhm = x(9);
config.digital_thresholds = x(10);

opticalOutput = x(11:end);

end

