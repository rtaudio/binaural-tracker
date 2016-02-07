function [ mic_denoised,H ] = reduce_noise( mic, ref )
 %   alpha = 1.5;
 %   epsilon = .05;
 %   smoothBW = 1/3;
 
 
 
    % works good:
    alpha = 1.0;
    epsilon = 0.005;
    smoothBW = 1/16;
    
    %    alpha = 2.0;
    %epsilon = 0.005;
    %smoothBW = 1/32;
    
    % these settings where good, but not in non-noisy reflection case
    %alpha = 2;
    %epsilon = .7;
    %smoothBW = 1/8;
    
    
    %alpha = 3;
    %epsilon = .1;
    %smoothBW = 1/16;
    
    
    %alpha = 1;
    %epsilon = .7;
    %smoothBW = 1/16;
    
       beta = 1;
    
    n = length(ref); 
    hw = hamming(n/16);
    hh = length(hw)/2;
    
    ref_win = ref;
    ref_win(1:hh) = ref_win(1:hh) .* hw(1:hh);
    ref_win(end-hh:end) = ref_win(end-hh:end) .* hw(hh:end);
    
    mic_win = ref;
    mic_win(1:hh) = mic_win(1:hh) .* hw(1:hh);
    mic_win(end-hh:end) = mic_win(end-hh:end) .* hw(hh:end);
    
    psd_ref = pwelch(ref);
    psd_mic = pwelch(mic);
    


    
    %H = (1 - ( (psd_mic-psd_ref) ./ (psd_mic + epsilon)).^beta).^(alpha/2);
    
    H = ((psd_ref ) ./ (psd_mic + epsilon)).^(alpha/2);

    h = ifftshift(ifft(H));
    h = padarray(h(1:end-1), ceil((n-length(h))/2));
    
    
iaH = itaAudio;
iaH.freqData = H;
iaH = ita_normalize_dat(iaH);
iaH = ita_zerophase(iaH);
iaH.timeData = [iaH.timeData(end/2:end); iaH.timeData(1:end/2-1)];

iaH = ita_smooth_frequency(iaH, 'bandwidth', smoothBW);

%iaH.plot_all

iaMic = itaAudio;
iaMic.timeData = mic;

mic_denoised = ita_normalize_dat(ita_convolve(iaMic, iaH))*10;
mic_denoised = mic_denoised.timeData;


mic_denoised = mic_denoised((1:n) + (length(mic_denoised)-n)/2);
    %ifft(fft(mic) * H);
    
    
    %n = length(psd_ref);    
    %   psd_noise = psd_noise * alpha;
    % filter = zeros(n,1);
    %for k = 1:n
    %        filter(k) = (1 - ( (psd_mic(k)-psd_ref(k)) / psd_mic(k) psd_ref(k) / psd_mic(k);
    %    end

    
  %mic_denoised = conv(psd_mic,real(h),'same');

end

