function [ timeData, timestamp] = wav_read_safe( file , prevData)    

    fh = fopen([file '.TS']);
    timestamp = str2double(fgetl(fh));
    fclose(fh);
    
    if isempty(timestamp) || timestamp <= 0
        timeData = [];
        return;
    end
    
   try
        timeData = audioread(file);
    catch
       timeData = [];
       return
   end
   
   if isempty(timeData)
       timeData = [];
       return
    end
    
    if exist('prevData','var') && ~isempty(prevData) && (sum(size(timeData) ~= size(prevData)) > 0 || sum(sum(abs(timeData-prevData))) < 0.00001)
       timeData = [];
       return
    end    
    

end

