function opticalOutput = simspad_server(config, opticalInput)
%SIMSPAD_SERVER posts a waveform to the SimSPAD web server and returns the response.
% Device parameters travel as a JSON object in the X-SiPM-Params request header;
% the request body is the raw optical-input waveform (little-endian float64). The
% response body is the little-endian float64 charge-per-step waveform, the same
% length as the input.

httpUrl = 'http://localhost:33232/simspad';

% --- device parameters as a JSON header ---
paramJson = jsonencode(params_struct(config));

% --- optical input as a raw little-endian float64 body ---
% NOTE: typecast uses the machine byte order; this assumes a little-endian host
% (all current MATLAB platforms), matching the server's '<f8' expectation.
body = typecast(double(opticalInput(:)).', 'uint8');

opt = weboptions( ...
    'RequestMethod', 'post', ...
    'MediaType', 'application/octet-stream', ...
    'ContentType', 'binary', ...
    'HeaderFields', {'X-SiPM-Params', paramJson}, ...
    'Timeout', 600);

fprintf("Sending simulation request...");
responseData = webwrite(httpUrl, body, opt);
fprintf(" Simulation result received.\n");

% --- reinterpret the octet-stream response as little-endian float64 ---
opticalOutput = typecast(uint8(responseData(:)).', 'double');
opticalOutput = opticalOutput(:)';

end
