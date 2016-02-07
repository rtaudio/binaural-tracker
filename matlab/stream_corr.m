%close all
%figure;

deltas = [];
delta_imax = 30;
delta_i = 1;
reject_outliers = 0;
offset = 0;

delta_min = 10^8;
delta_max = -10^8;


Fs = 48000;
C = 343;

sweep = -audioread('R:\sweep.WAV'); 
calibSpk1 = audioread('R:\calib_spk0.WAV'); 
calibSpk2 = audioread('R:\calib_spk1.WAV'); 

if isempty(sweep) || isempty(calibSpk1) || isempty(calibSpk2)
    pause(0.1);
    return;
end
calibSpk1 = calibSpk1(:,1);
calibSpk2 = calibSpk2(:,1);


noiseRed =1;

deltaSpk1 = find_delay(calibSpk1, sweep,noiseRed );
deltaSpk2 = find_delay(calibSpk2, sweep,noiseRed);
spkDelta = abs(deltaSpk1 - deltaSpk2);
deltaMean = (deltaSpk1+deltaSpk2)/2;


sweep_matched1 = sweep(1:(end-round(deltaSpk1)));
spk_matched1 = calibSpk1((1+round(deltaSpk1)):end);
sweep_matched2 = sweep(1:(end-round(deltaSpk2)));
spk_matched2 = calibSpk2((1+round(deltaSpk2)):end);


% cross validation with sweep only
%assert(abs(find_delay(calibSpk1((1:2^15) + round(deltaSpk1-200)), sweep(1:2^15),noiseRed)-200) < 4.2)
%assert(abs(find_delay(calibSpk2((1:2^15) + round(deltaSpk2-200)), sweep(1:2^15),noiseRed)-200) < 4.2)

corrected_match = [];

 spkDeltaCross1 = abs(find_delay(calibSpk2((1:2^15) +round(deltaMean)),calibSpk1((1:2^15) +round(deltaMean)), 0));
 spkDeltaCross2 = abs(find_delay(calibSpk1((1:2^15) +round(deltaMean)),calibSpk2((1:2^15) +round(deltaMean)), 0));
 
% cross validate speaker offset
if  spkDeltaCross1 < spkDeltaCross2
    spkDeltaCross = spkDeltaCross1;
    corrected_match =  calibSpk2((1+round(spkDeltaCross)):end);
    xcorL = 'xcor (spk2)';
%    assert(abs(abs(find_delay(calibSpk2((1:2^15) +round(deltaMean)),calibSpk1((1:2^15) +round(deltaMean)), 0)) - spkDelta) < 0.2); % this fails if speaker distance is close
   % assert(abs(abs(find_delay(calibSpk2((1:2^15) +round(deltaMean)),calibSpk1((1:2^15) +round(deltaMean)), 1)) - spkDelta) < 0.2); % with noise reduction
else
     spkDeltaCross = spkDeltaCross2;
     
    corrected_match =  calibSpk1((1+round(spkDeltaCross)):end);
    xcorL = 'xcor (spk1)';
    
     % corrected_match = calibSpk1((1+round(spkDeltaCross)):end);
    %assert(abs(abs(find_delay(calibSpk1((1:2^15) +round(deltaMean)),calibSpk2((1:2^15) +round(deltaMean)), 0)) - spkDelta) < 0.2); % this fails if speaker distance is close
%     assert(abs(abs(find_delay(calibSpk1((1:2^15) +round(deltaMean)),calibSpk2((1:2^15) +round(deltaMean)), 1)) - spkDelta) < 0.2); % with noise reduction
end

 


     
ia1 = itaAudio; ia1.timeData = calibSpk1(deltaSpk1/1+1:2^15);
ia2 = itaAudio; ia2.timeData = spk_matched2(1:2^15);
iaS = itaAudio; iaS.timeData = sweep(1:2^15);



player = audioplayer([sweep_matched1, spk_matched1],48000);
%player.playblocking
player = audioplayer([sweep_matched2, spk_matched2],48000);
%player.playblocking

fprintf('Speaker 1 delay: %8.4f samples, %5.2f ms, distance: %5.2f\n', deltaSpk1, deltaSpk1/Fs*1000, deltaSpk1/Fs*C);
fprintf('Speaker 2 delay: %8.4f samples, %5.2f ms, distance: %5.2f m\n', deltaSpk2, deltaSpk2/Fs*1000, deltaSpk2/Fs*C);

fprintf('Speakers distance: %5.2f cm  (%8.4f samples) (x says %5.2f cm, %8.4f samples )\n', spkDelta/Fs*C*100, spkDelta, spkDeltaCross/Fs*C*100, spkDeltaCross);


return;

subplot(2,1,1);
     plot([sweep_matched1(1:2^15+20000) spk_matched1(1:2^15+20000) spk_matched2(1:2^15+20000)]);
    legend('sweep','spk1','spk2');
    
    
   l = round(2^15.5);
subplot(2,1,2);    
    plot([zeros(l,1) calibSpk1(round(deltaMean*1.5)+ (1:l)) calibSpk2(round(deltaMean*1.5)+ (1:l)) corrected_match(round(deltaMean*1.5)+ (1:l))]);
    legend('null','spk1','spk2',xcorL);
    grid on;

return;

figure;
%profile on
for i = 1:10000
    try
        [timeData,Fs] = audioread('R:\sig_mic.WAV'); 
    catch
       continue
    end    
    if isempty(timeData)
        pause(100/1000);
        continue;
    end
    
    ref = timeData(:,1);
    mic = timeData(:,2);
          
    % energy-normalization
    %ref = ref / sqrt(mean(ref .^ 2));
    %mic = mic / sqrt(mean(mic .^ 2));


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
