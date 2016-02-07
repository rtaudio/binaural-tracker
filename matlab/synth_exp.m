
Fs = 48000;

synth = sin(0:

player = audioplayer(synth,Fs);
player.playblocking