deltas = zeros(2^20,1);

Fs = 48000;
C = 343;

hist_len = 10;

clock = 0;

[speaker_offsets, drift2] = calibrate(2);

%figure;
%profile on


i = 0;
timeDataPrev = [];
for ii = 1:10000
    timeData = wav_read_safe('R:\sig_mic.WAV', timeDataPrev); 
    if isempty(timeData)
        pause(1/1000);
        continue;
    end
    timeDataPrev = timeData;   
    nSamples = size(timeData,1);
    clock = clock + nSamples;
    i = i + 1;
    
    ref = timeData(:,1:2);
    mic = timeData(:,3); % only mono mic yet
          
    % energy-normalization
    %ref = ref / sqrt(mean(ref .^ 2));
    %mic = mic / sqrt(mean(mic .^ 2));

    deltaN = find_delay(mic, ref(:,2)) - clock*drift/13;    
    deltas(i) = deltaN;
    
    
    
    ref_matched = ref(1:(end-round(deltaN)),2);
    mic_matched = mic((1+round(deltaN)):end);
    
    
    subplot(2,1,1);
        plot([ref_matched mic_matched]);
        ylim([-1 1]);
        grid on;
        

     recentDeltas = deltas(max(1,i-hist_len):i);
     
     md = median(recentDeltas);
     
     subplot(2,1,2);
     
        h =   histogram((deltas(max(1,i-hist_len*8):i)), 200); 
        h.FaceColor = [0.5 0.5 0.5];
         h.EdgeColor = [1 1 1];
        hold on;        
        histogram((recentDeltas), 200); 
        xlim( (floor(md/200+.5)*200 + [-100 100]));
        ylim([0 hist_len]);
        plot([md md], [0 hist_len], 'LineWidth',3);
        grid on;
        hold off;

        
    
    fprintf('Delta: %5.1f cm\n', deltaN/Fs*C*100);
    
    
    
    drawnow;
    continue;
    
    deltaN = find_delay(mic, ref);
    
    
    
    deltaN = find_delay(mic, ref);
    
    d = 0;
    
    if deltaN > 0
        d = deltaN;
        if isempty(deltas)
               md = d;
               offset = -md;
        else
            md = (median(deltas));
        end 
        e = abs((d - md) / abs(md)); 
        
        if reject_outliers == 1 && e > 0.02
            display('rejected outlier!');
            e
            md - d 
        else
            deltas(delta_i) = d;            
            delta_i = delta_i + 1;        
            if delta_i > delta_imax
                delta_i = 1;
                if reject_outliers == 0
                    reject_outliers = 1;
                    offset = -md;
                    fprintf('Offset: %10.0f\n', offset);
                end
            end
        end 
    end

    if deltaN < 0
        deltaN = 0;
    end
    
    % plot matched
    ref_matched = ref(1:(end-round(deltaN)));
    mic_matched = mic((1+round(deltaN)):end);
    
    %mic_matched = mic(1:end-round(deltaN)+1);
    %ref_matched = ref(round(deltaN):end);
    
    % plot ref % mic
    subplot(2,2,3);
        plot([downsample(ref,10) downsample(mic,10)]);
        ylim([-1 1]);
        
        
    subplot(2,2,4);
        plot([downsample(ref_matched,10) downsample(mic_matched,10) ]);
        %title(['deltaN = ' num2str(deltaN)]);
        ylim([-1 1]);
        
    %subplot(2,3,6);
    %    plot([mic_matched-ref_matched]);
    %    title(['res']);
    %    ylim([-1 1]);

     subplot(2,1,1);
        histogram((deltas + offset)/ 250, 250);        
        %title(['d = ' num2str(d)]);
        %xlim([delta_min-1 delta_max+1]);
        i = round((md + offset)/100-1)*100;
        xlim([i i+200] / 250);
        hold on;
        plot(([md md] + offset) / 250, [0 10], 'LineWidth',3);
        hold off;



    % ref auto correl
    %mu_aa_all = xcorr(ref,ref,'coeff');
    %mu_aa = abs(mu_aa_all(length(ref):end)); 

    % ref/mic correl
    %mu_ab_all = xcorr(ref,mic,'coeff');    
    %mu_ab = abs(mu_ab_all(length(ref):end)); 
    

    %pause(500/1000);
    drawnow
end
profile viewer

%player = audioplayer(a(round(deltaN):end)+b(1:end-round(deltaN)+1),48000);
%player.playblocking

ref_matched_d = downsample(ref_matched,3);
mic_matched_d = downsample(mic_matched,3);

e_ref = mean(ref_matched_d .^ 2);
e_mic = mean(mic_matched_d .^ 2);

ref_matched_d = ref_matched_d / sqrt(e_ref);
mic_matched_d = mic_matched_d / sqrt(e_mic);

gm = max(max(ref_matched_d), max(mic_matched_d)) / 1;
ref_matched_d = ref_matched_d / gm;
mic_matched_d = mic_matched_d / gm ;
    
figure; plot([mic_matched_d, ref_matched_d]);
player = audioplayer([ref_matched_d, mic_matched_d],48000);
player.play;

fprintf('Offset: %10.0f\n', offset);
