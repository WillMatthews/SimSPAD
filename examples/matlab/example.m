%% Full Generic Input Example

irrad_tx = 10E-3; % MEAN Transmit Power
irrad_dc = 1.2E-3; % DC irradiance
samplerate = 1E9; % 1Gsps (upsampled in make_config)

%% Make arbitary optical input
tend = 5E-6;
time = 0:(1/samplerate):tend;
inputShape = 1 + sin(time * 2 * pi * 1E6); % Sine 1MHz
%inputShape = time; % Ramp


%% Create SiPM settings
config = make_config(irrad_tx, irrad_dc, samplerate);

%% Create Calibrated Optical Input
opticalInput = make_calibrated_input(inputShape, config); % in expected photons striking sipm per dt

upsampled_time = 0:config.dt:(config.dt * (numel(opticalInput)-1));

%% Send to server (parameters as a JSON header, waveform as an octet-stream body)
sipmOutput = simspad_server(config, opticalInput);

%% (Alternatively, run the standalone CLI on the same inputs:)
%   binary_pack("demo", config, opticalInput);
%   system("../../build/apps/simspad -p binary/demo.json -i binary/demo.npy -o binary/demo_out.npy");
%   sipmOutput = binary_unpack("demo_out")';

sipmOutput_shaped = pulse_shape(sipmOutput, config);

current = sum(sipmOutput(1000:end))/(config.dt * numel(sipmOutput(1000:end)));
fprintf("Simulated Current: %3.3f mA\n", current * 1E3);

pde = config.pde_est(config.vbias-config.vbr);

figure();
yyaxis left;
plot(upsampled_time, sipmOutput_shaped);
ylabel("SiPM Output");
hold on;
yyaxis right;
plot(upsampled_time, opticalInput*pde);
ylabel("Fixed PDE Expected Detections per dt");
xlabel("Time [s]");
set(gca, "FontSize", 12, "FontWeight", "Bold");
