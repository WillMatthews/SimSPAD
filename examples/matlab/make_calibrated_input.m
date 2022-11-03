function [light] = make_calibrated_input(optical_input, config)
%MAKE_OPTICAL_INPUT calibrates the optical input given the configuration
%of the SiPM to create a vector with the expected number of photons per time step



% Set any optical input less than zero to be zero (can't have negative
% quantities of light)
optical_input(optical_input<0) = 0;

% Prepend and append portions of optical input vector to allow simulation
% to stabilise, and have some remaining input for filtering without
% truncation
try
    pre = optical_input((end-2500):end);
catch
    pre = optical_input;
end
try
    post = optical_input(1:1000);
catch
    post = optical_input;
end
optical_input = [pre, optical_input, post];

% Determine photon rates (photons per second)
config.photon_rates_tx = config.A_sipm/config.photonE * config.irrad_tx;
config.photon_rates_dc = config.A_sipm/config.photonE * config.irrad_dc;

% Convert optical_input to simulation input
% Uses config.sa_per_bit samples per element in optical_input
optical_input = repelem(optical_input,config.sa_per_bit);

%% Loop through different light levels
% create incident light (expected num of photons per time step)
light = config.photon_rates_tx*config.dt.*optical_input./ ...
    mean(optical_input)+config.photon_rates_dc*config.dt;



end

