[timeData,Fs] = audioread('R:\sig_mic.WAV'); 

ref = timeData(:,1);
mic = timeData(:,2);

[nos1 nos2] = WienerNoiseReduction(mic,Fs,ref);

player = audioplayer([nos1/10],Fs);
player.playblocking