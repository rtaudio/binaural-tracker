[timeData,Fs] = audioread('R:\sig_mic.WAV'); 
    ref = timeData(:,1);
    mic = timeData(:,2);
  %close all;  
  
  
%ref = smooth_signal(ref);
%mic = smooth_signal(mic);
    
    
%cpsd(mic,ref);

%cpsd(ref,ref);

pref = pwelch(ref);
pmic = pwelch(mic);
epsilon = 0.00001;
ph = (pref ./ (pmic+ epsilon)).^2;

% figure;
%     loglog(pref);
%     hold on;
%     loglog(pmic);
%     loglog(ph);
%     
%     legend('ref', 'mic', 'H');
    

[ mic_denoised,H ] = reduce_noise( mic, ref );


iaH = itaAudio;
iaH.freqData = H;
iaH = ita_normalize_dat(iaH);
iaH = ita_zerophase(iaH);
iaH.timeData = [iaH.timeData(end/2:end); iaH.timeData(1:end/2-1)];

iaRef = itaAudio%
iaRef.timeData = ref;

iaMic = itaAudio;
iaMic.timeData = mic;


iaMicFiltered = ita_normalize_dat(ita_convolve(iaMic, iaH))*10;

player = audioplayer([mic_denoised/10],48000);
player.playblocking


iaMic = ita_smooth_frequency(iaMic);

player = audioplayer([iaMic.timeData/10],48000);
player.playblocking



%iaH.plot_all;


%iaMic.plot_all;
%iaMicFiltered.plot_all;


%player = audioplayer([iaMic.timeData],48000);
%player.playblocking

