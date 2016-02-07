function [ deltaN ] = find_delay( mic, ref, reduceNoise )

    % remove spikes (robust way!)
    %med_len = 10;
    %ref = medfilt1(ref,med_len);
    %mic = medfilt1(mic,med_len);   
    
    
if exist('reduceNoise','var') && reduceNoise > 0
    mic =         reduce_noise(mic,ref);
   %ref =         reduce_noise(ref,ref);
end
    
    
    %player = audioplayer([mic, ref]/10,48000);
    %player.playblocking


    
    % remove DC
    ref = ref - mean(ref);
    mic = mic - mean(mic);
    
    % max normalize
    ref = ref / max(abs(ref));
    mic = mic / max(abs(mic));
    
    upS = 2;
    ref = upsample(ref,upS);
    mic = upsample(mic,upS);
    
    [coeff shiftedRef deltaN] = fitSignal_FFT(mic, ref);
    
    % numerically search for better solutiona
    if deltaN > 20 && deltaN < 20000
        y = ones(41,1)*10^10;
        for i=-20:20
            y(21+i) = sum((mic( (1:2^15) + round(deltaN)) - ref((1:2^15)  + round(deltaN) + i)).^2);
        end;
        [~,yi] = min(y);
        yi = yi - 21;

        if yi ~= 0
            deltaN = deltaN + yi;
            display('found a better solution');
            yi
        end
    end
    %for i=-1000:1000; y(1001+i) = sum((mic( (1:2^15+20000) + round(deltaN)) - ref((1:2^15+20000)  + round(deltaN) + i)).^2); end;  figure; plot(-1000:1000, y);
    
    % upsampled
    deltaN = deltaN / upS;
end

