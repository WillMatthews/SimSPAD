function [shaped_output] = pulse_shape(sipmOutput, config)
%PULSE_SHAPE gaussian shapes output pulses

% generate output pulse shape (approximately a Gaussian)
pulseshape = gausswin(round(( 1.75 * (2/1.5)*config.pulse_fwhm)/config.dt));
if numel(pulseshape) < 2
    pulseshape = 1;
end
% get integral of output pulse shape
pulseintegral = sum(pulseshape);

shaped_output = conv(sipmOutput, pulseshape/pulseintegral);

halfpulsewidth = floor(numel(pulseshape)/2);
halfpulsewidth2 = halfpulsewidth;
if ~(halfpulsewidth * 2 == numel(pulseshape))
    halfpulsewidth = halfpulsewidth + 1;
end

shaped_output = shaped_output(halfpulsewidth : (end-halfpulsewidth2));
%plot((1:x)*config.dt, pulseshape)

end

