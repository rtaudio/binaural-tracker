Fs = 48000;
sweep = ita_generate_sweep('fftDegree' , 16, 'samplingRate', Fs);
size(sweep.timeData)
sweep
ita_write_wav(sweep, 'sweep16.wav');


sweep = ita_generate_sweep('fftDegree' , 15, 'samplingRate', Fs);
size(sweep.timeData)
sweep
ita_write_wav(sweep, 'sweep15.wav');