function [fires, avail_spads, meanpdes, meancellcharge, spadstore] = simspad(light,set)
numlevels = numel(set.digital_threshholds);
fires = zeros(numlevels,numel(light));
avail_spads = zeros(1,numel(light));

spadstore=zeros(set.spads_to_store, numel(light));

numspad = set.numspad;
spads = 0*rand(1,numspad); % initialise at random points
spadtime = rand(1,numspad)*10E-8;
recovery = set.recovery;
dt = set.dt;
digital = set.digital;
paralyze = set.paralyzable;
vary_pde = set.vary_pde;
%digital_thresh = set.digital_threshhold;
meanpdes = zeros(1,numel(light));
meancellcharge = zeros(1,numel(light));

vbias = set.vbias;
vbr = set.vbr;
vover = set.vbias-set.vbr;

slen = 0;

pde_est = set.pde_est;
%pde_est = @(overvoltage)0.46.*(1-exp(-(overvoltage./set.vbr)./0.083));

nl = numel(light);

a=0;
b=0;
c=0;
d=0;
e=0;

for i = 1:nl
    l = light(i);
    
    [spads, spadtime] = incrementspads(spadtime); % recovery
    
    if digital && vary_pde
        [fires(:,i), avail_spads(i), spads, spadtime, meanfiredcharge] = illuminatespads_paralyze_PDE_digital(spads, l, spadtime);
    elseif ~digital && vary_pde
        [fires(:,i), avail_spads(i), spads, spadtime, meanfiredcharge] = illuminatespads_paralyze_PDE(spads, l, spadtime);
    else
        error("Not Configured");
    end
    
    meanpdes(i) = get_mean_pdes(spads); % meanfiredpde; 
    meancellcharge(i) = meanfiredcharge; %mean(spads * set.ccell);
    spadstore(:,i) =  spads(1:set.spads_to_store)';
    
    if mod(i,100) == 0
        str_out = sprintf("%3.2f%%%%\\n", (i/nl) * 100 );
        fprintf(strcat(repmat('\b', 1, slen),str_out));
        slen = length(char(str_out))-2;
    end
    
    
end

    function [meanpde] = get_mean_pdes(spads)
        meanpde = mean(pde_est(spads));
    end


    function [spads,spadtime] = incrementspads(spadtime)
        %spads = spads + dt/recovery;
        spadtime = spadtime + dt;
        spads = vover * (1-exp(-spadtime/recovery));
        spads(spads>vover) = vover;
        %[max(spads), max(spadtime)*1E9]
    end


    function [fires, avail_spads, spads, spadtime, meanfiredcharge] = illuminatespads_paralyze_PDE_digital(spads, illum, spadtime)
        toFire = illum;
        candidates = 0 + (rand(numspad, 1) < toFire/numspad.*(pde_est(spads)'))';
        %candidates = rand(1,numspad).*candidates;
        
        avail_spads = sum(spads>=vover);
        
        %spads_to_fire = (candidates < spads) & (candidates > 0);
        
        spads_to_fire = (candidates>0);
        spads_to_fire_OUT = (spads >= (digital_thresh*vover)) & spads_to_fire;
        
        fires = set.ccell * sum(spads(spads_to_fire_OUT)); %sum(spads_to_fire(spads >= (digital_thresh*vover)));

        meanfiredcharge = set.ccell * fires/sum(spads_to_fire_OUT);
        
        spads(spads_to_fire) = 0;
        spadtime(spads_to_fire) = 0;
        
        if numel(fires) == 0
            fires=0;
            meanfiredcharge=0;
            
        end
    end


    function [fires, avail_spads, spads, spadtime, meanfiredcharge] = illuminatespads_paralyze_PDE(spads, illum, spadtime)
        toFire = illum;
        candidates = 0 + (rand(numspad, 1) < toFire/numspad.*(pde_est(spads)'))';
        %candidates = rand(1,numspad).*candidates;
        avail_spads = sum(spads>=vover);
        spads_to_fire = (candidates>0);
        
        k = 1;
        fires = zeros(numlevels,1);
        for digital_thresh = set.digital_threshholds
            spads_to_fire_OUT = (spads >= (digital_thresh*vover)) & spads_to_fire;
            fires(k,:) = set.ccell * sum(spads(spads_to_fire_OUT)); % charge from voltage Q=CV
            k = k+1;
        end
        
        meanfiredcharge = set.ccell * sum(spads(spads_to_fire))/sum(spads_to_fire);
        %meanfiredpde = sum(pde_est(spads(spads_to_fire)))/sum(spads_to_fire);
        
        spads(spads_to_fire) = 0;
        spadtime(spads_to_fire) = 0;
    end
end

