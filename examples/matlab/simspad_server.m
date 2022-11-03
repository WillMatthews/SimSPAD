function [opticalOutput, config] = simspad_server(filename)
%SIMSPAD_SERVER performs the post request to retrieve data
%from the SimSPAD web server

%% Web Settings
httpUrl  = 'http://localhost:33232/simspad';
opt = weboptions;
opt.Username = "";
opt.Password = "";
opt.ArrayFormat = 'json';
opt.ContentType = 'text';
opt.MediaType = 'application/octet-stream';
opt.CharacterEncoding = 'ISO-8859-1';
opt.RequestMethod = 'post';
opt.Timeout = 600;

%% type coerce binary double array -> char*
dataFile = strcat("./binary/", filename, ".bin");
fid = fopen(dataFile, 'r');
data = char(fread(fid)');
fclose(fid);

fprintf("Sending simulation request...");
responseData = webwrite(httpUrl, data, opt);
fprintf(" Simulation result received.\n");

if ((sum(not(data(1:80) == responseData(1:80)))))
    error("Config Mismatch: Input and Output SiPM Settings do not match! This may be due to a bad file transfer or error with the simulator.");
end

%% type coerce output (char* -> double)
dataFile = strcat("./binary/", filename, "_out", ".bin");
fid = fopen(dataFile, "w");
fwrite(fid, uint8(responseData), "uint8");
fclose(fid);

[config, opticalOutput] = binary_unpack(strcat(filename, "_out"));
opticalOutput = opticalOutput';

%% (DEBUG) plot input and output side by side
% fid = fopen(dataFile, 'r');
% input = fread(fid, "double");
% fclose(fid);

% figure();
% yyaxis left;
% plot(input(10:end));
% hold on;
% yyaxis right;
% plot(output(10:end));

end

