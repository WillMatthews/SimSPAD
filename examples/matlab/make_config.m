function [config] = make_config(irrad_tx, irrad_dc, samplerate)
%MAKE_CONFIG creates a configuration structure for the SiPM given
%transmit irradiance, dc irradiance, and a sample rate

config.irrad_tx = irrad_tx;
config.irrad_dc = irrad_dc;

if (config.irrad_tx + config.irrad_dc) > 1.1
    error("Irrad too high!");
end

%% Settings for Simulation
config.sa_per_bit = 21;
%samplerate = config.samplerate;
% Simulation time step (keep dt << recovery and dt << sample rate)
config.dt = (1/samplerate)/config.sa_per_bit; 
% Number of Microcells to store the overvoltage for each time step
config.num_microcells_to_store = 10;
% Detection threshholds. Keep at zero for analogue SiPM
config.digital_threshholds = [0];
% photon energy (currently set at 405nm)
config.photonE = 4.90E-19; % 405nm light energy per photon in Joules


%% Settings for Experimental Setup
config.vbias = 27.5; % SiPM Bias Voltage (May be changed)
config.vbr   = 24.5; % SiPM Breakdown Voltage


%% Settings for SiPM (Set as J30020)
config.num_microcell = 14410; % number of microcells to simulate
config.recovery      = 2.2*14e-9; % config the recovery time (10-90 time in seconds)

% SiPM PDE with respect to overvoltage. Empirical model.
config.pde_Max  = 0.46;
config.pde_vchr = 2.04;

config.pde_est  = @(overvoltage) ...
    config.pde_Max.*(1-exp(-(overvoltage./config.pde_vchr)));

config.A_sipm     = (3.07E-3)^2; % SiPM Area (in m^2)
config.ccell      = 4.6e-14; % cell capacitance in farads
config.pulse_fwhm = 6E-9; % Full width half max pulse output size in seconds


end

