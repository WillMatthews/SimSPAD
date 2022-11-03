function [] = binary_pack(filename, config, opticalInput)
%PACKAGE_TO_BINARY packages a SiPM configuration and input vector into a binary file
% takes as arguments a filename as a string, a configuration structure, and an input vector


%% Settings for Simulation
%config.samplerate;
%config.numsample;
%config.num_microcells_to_store;
%config.photonE;
%config.A_sipm;

x(1) = config.dt;
x(2) = config.num_microcell;
x(3) = config.vbias;
x(4) = config.vbr;
x(5) = config.recovery;
x(6) = config.pde_Max;
x(7) = config.pde_vchr;
x(8) = config.ccell;
x(9) = config.pulse_fwhm;
x(10) = config.digital_thresholds;

fname = strcat("./binary/", filename, ".bin");

fileID = fopen(fname,'w');
fwrite(fileID,x,'double');
fclose(fileID);

fileID = fopen(fname,'a');
fwrite(fileID,opticalInput,'double');
fclose(fileID);

end

