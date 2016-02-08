#include <fstream>
#include <string>
#include <iostream>

#include "Calibration.h"
#include "AudioUtils.h"
#include <jack/jack.h>


Calibration::Calibration(AudioDriver *driver)
{
	int np = driver->getNumPlaybackChannels();
	int nc = driver->getNumCaptureChannels();


	int len = (int)pow(2, 15);	
	// double size because we expect the total system delay to be longer than the pause after the sweep in the sweep wave file
	len *= 2; 
	float *sweep = new float[len];
	AudioUtils::wavRead("sweep15.wav", sweep, len);

	driver->MuteOthers(true);

	AudioDriver::Request req(driver);
	
	SignalBuffer inBuffer(nc, len);
	

	uint32_t firstTime = -1;
	uint32_t ot;

	uint32_t startTime = driver->getClock();

	for (int ri = 0; ri < NumberRepetions; ri++)
	{
		
		for (int ci = 0; ci < np; ci++) {
			std::vector<float*> channels(np, NULL);
			channels[ci] = sweep;

			SignalBuffer playBuffer(channels, len);
			SignalBufferObserver observer(inBuffer, playBuffer);


			// deterministically add exitation signal and the input buffer
			req.addSignal(&inBuffer, "calibration-in", AudioDriver::Connect::ToCapture)
				.addSignal(&playBuffer, "sweep-out", AudioDriver::Connect::ToPlayback)
				.addObserver(&observer).execute();

			// wait until exitation signal has been played and the input buffer is full
			observer.waitForCommit();

			// remove signals and observer
			req.remove(&inBuffer).remove(&playBuffer).remove(&observer).execute();


			std::string suffix = "spk" + std::string(1, '0' + ci) + "_pass" + std::string(1, '0' + ri);

			normalize(sweep, len);
			inBuffer.normalize();
			AudioUtils::wavWrite("R:\calib"+ suffix + ".WAV", inBuffer);

			std::ofstream out("R:\calib" + suffix + ".WAV.TS");
			out << observer.lastUpdate;

			std::cout << "GOT DATA! " << suffix << std::endl;

			ot = observer.lastUpdate;

		}
		
		if (firstTime == -1)
			firstTime = ot;

		if (ri < (NumberRepetions - 1) && WaitBetweenRepetitions)
			usleep(1000 * WaitBetweenRepetitions);
	}

	AudioUtils::wavWrite("R:\sweep.WAV", sweep, len);

	delete[] sweep;

	driver->MuteOthers(false);

	float dt = (float)(ot - firstTime) / (float)driver->getSampleRate() / ((float)NumberRepetions-1.0f);
	std::cout << "Calibration done, " << NumberRepetions << " passes, " << dt << " s/pass" ;
	
}


Calibration::~Calibration()
{
}
