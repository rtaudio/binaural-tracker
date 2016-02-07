function [ ts ] = smooth_signal( t )
    ia = itaAudio;
    ia.timeData = t;
    ia = ita_smooth_frequency(ia);
    ts = ia.timeData;
end

