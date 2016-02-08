function [ deltaN ] = find_delay( mic, ref, reduceNoise, mask )

    % remove spikes (robust way!)
    %med_len = 10;
    %ref = medfilt1(ref,med_len);
    %mic = medfilt1(mic,med_len);   
    
    %mic = mic((end/4):(end/2));
    %ref = ref((end/4):(end/2));
    
    mic_or = mic;
    ref_or = ref;
    
    mask = exist('mask','var') && mask > 0;
    reduceNoise = exist('reduceNoise','var') && reduceNoise > 0;

    if reduceNoise
        mic =         reduce_noise(mic,ref);
       %ref =         reduce_noise(ref,ref);
    end
    
    
    %player = audioplayer([mic, ref]/10,48000);
    %player.playblocking


    
    % remove DC
    %ref = ref - mean(ref);
    %mic = mic - mean(mic);
    
    % max normalize
    ref = ref / max(abs(ref));
    mic = mic / max(abs(mic));
    
    upS = 2;
    ref = upsample(ref,upS);
    mic = upsample(mic,upS);
    
    [coeff, shiftedRef, deltaN] = fitSignal_FFT(mic, ref);
    
     deltaN = deltaN / upS;
     
     if ~mask
         return
     end
     
     rec_os = ref_or(1:(end-round(deltaN)));
     mic_os = mic_or((1+round(deltaN)):end);
     
     mask_ref = medfilt1(abs(rec_os),480);     
     mask_mic = medfilt1(abs(mic_os),480);
     mask = mask_ref .* mask_mic;
     mask = mask / max(mask);
     
     rec_masked = rec_os .* mask;
     mic_masked = mic_os .* mask;
     
     [coeff, ~, deltaN2] = fitSignal_FFT(mic_masked, rec_masked);
     
     if deltaN2 > length(rec_masked)/2
         deltaN2 = deltaN2-length(rec_masked);
     end
     
     deltaN = deltaN + deltaN2;
     
     return;
     
    % numerically search for better solution (with original signals)
    span=50;
    if deltaN > span && deltaN < length(mic)/2
        y = ones(span+1,1)*10^10;
        ss = (span+length(mic_or)-2^15):(length(mic_or)-span-round(deltaN));
        for i=-span:span
            y(span+1+i) = sum((mic_or(ss + round(deltaN) + i) - ref_or(ss)).^2);
        end;
        [~,yi] = min(y);
        yi = yi - span - 1;

        if yi ~= 0 %&& (y(yi+span+1) / y(span+1)) < 0.99 
            deltaN = deltaN + yi;
           % display(['found a better solution @', num2str(yi)]);
            %  figure;plot(-span:span, y);
        end
    end
    %for i=-1000:1000; y(1001+i) = sum((mic( (1:2^15+20000) + round(deltaN)) - ref((1:2^15+20000)  + round(deltaN) + i)).^2); end;  figure; plot(-1000:1000, y);
    
    % upsampled
   
end

