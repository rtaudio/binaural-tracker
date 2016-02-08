#include <fstream>
#include <string>
#include <iostream>

#include "BinauralTracker.h"

#include "AudioUtils.h"
#include "Calibration.h"

#include <algorithm>

#define NUM_CHANNELS 2

BinarualTracker::BinarualTracker()
	:
	m_isRunning(false)
{
	int sampleRate = driver.getSampleRate();

	if (sampleRate != 48000) {
		throw "Only 48 kHz sample rate supported!";
	}

	int maxDelaySamples = sampleRate * MAX_SPEAKER_DISTANCE / C_SOUND + 1;
	int jackBuffer = driver.getBlockSize() *16*2*2;// todo	
	m_fftSize = (uint32_t)pow(2, ceil(log((float)(std::max)(maxDelaySamples, jackBuffer-1)) / log(2)));// make sure its power of 2

	
	m_SignalBuffer = new SignalBuffer(NUM_CHANNELS, m_fftSize);
	m_micBuffer = new SignalBuffer(NUM_CHANNELS, m_fftSize);

	m_signalHistoryPool.updateInterval = sampleRate / 10;
	m_signalHistoryPool.addHist(m_SignalBuffer);
	m_signalHistoryPool.addHist(m_micBuffer);
}

BinarualTracker::~BinarualTracker()
{
	Stop();
}

void BinarualTracker::Start()
{
	AudioDriver::Request req(&driver);
	req
		.addSignal(m_SignalBuffer, "signal", AudioDriver::Connect::ToPlaybackSource)
		.addSignal(m_micBuffer, "mic", AudioDriver::Connect::ToCapture)
		.addObserver(&m_signalHistoryPool)
		.execute();


	m_isRunning = true;
	PlatformThread::Routine updateThread(std::bind(&BinarualTracker::_updateThreadMain, this, std::placeholders::_1));
	m_updateThread = new PlatformThread(updateThread);
}


void BinarualTracker::Stop()
{
	m_isRunning = false;
	m_updateThread->Join();
	delete m_updateThread;
}

void BinarualTracker::Calibrate()
{
	//while (true) {
		Calibration * calib = new Calibration(&driver);
	//}
}


void  BinarualTracker::_updateThreadMain(void *arg)
{
	float *crossSpectra[NUM_CHANNELS];
	float *crossTime[NUM_CHANNELS];
	float *debug_buf[NUM_CHANNELS];

	for (int c = 0; c < NUM_CHANNELS; c++) {
		crossSpectra[c] = new float[(m_fftSize + 1) * 2];// complex!
		crossTime[c] = new float[m_fftSize];
		debug_buf[c] = new float[(m_fftSize + 1) * 2];
	}


	auto fftt = new ITAFFT(ITAFFT::FFT_R2C, m_fftSize, crossSpectra[0], crossTime[0]);
	auto ifftt = new ITAFFT(ITAFFT::IFFT_C2R, m_fftSize, crossSpectra[0], crossTime[0]);


	while (m_isRunning) {
		if (!m_signalHistoryPool.waitForCommit())
			continue;

		printf("UPDATE!\n");

		{
			std::ofstream out("R:\sig_mic.WAV.TS");
			out << m_signalHistoryPool.lastUpdate;
		}

		m_signalHistoryPool.normalizeSignals();
		AudioUtils::wavWrite("r:\sig_mic.WAV", m_signalHistoryPool);


		auto micSpectrumInv = m_micBuffer->getFreq(0, m_fftSize, false);

		const float * sigSpectrum;
		for (int c = 0; c < NUM_CHANNELS; c++) {
			sigSpectrum = m_SignalBuffer->getFreq(c, m_fftSize);
			fm_cmul_x(crossSpectra[c], sigSpectrum, micSpectrumInv, m_fftSize + 1);
			ifftt->execute(crossSpectra[c], crossTime[c]);

			float maxVal = 0;
			int maxI = -1;

			for (int i = 0; i < m_fftSize; i++) {
				if (fabsf(crossTime[c][i]) > maxVal) {
					maxVal = fabsf(crossTime[c][i]);
					maxI = i;
				}
			}

			//printf("micXsignal%d: MAX_I=%6d, val=%3.0f, D=%6d\n", c, maxI, maxVal/1000.0, maxI-26000);

			/*
			fftt->execute()
				iffts[c]->execute((float*)sigSpectrum, debug_buf[c]);
			for (int j = 0; j < m_fftSize; j++) {
				//debug_buf[c][j] = sigSpectrum[j * 2];
			}*/
		}



		/*

			normalize(m_micBuffer->getPtrTS(0), m_fftSize);
			normalize(m_micBuffer->getPtrTS(1), m_fftSize);
			writeAudiofile("r:\sig.WAV", m_SignalBuffer->getPtrTS(0), m_SignalBuffer->getPtrTS(1), m_fftSize, 44100);
			writeAudiofile("r:\mic.WAV", m_micBuffer->getPtrTS(0), m_micBuffer->getPtrTS(1), m_fftSize, 44100);

			writeAudiofile("r:\sig_mic.WAV", m_SignalBuffer->getPtrTS(0), m_micBuffer->getPtrTS(1), m_fftSize, 44100);

			normalize(crossTime[0], m_fftSize); normalize(crossTime[1], m_fftSize);
			writeAudiofile("r:\cross.WAV", crossTime[0], crossTime[1], m_fftSize, 44100);

			ifftt->execute((float*)micSpectrumInv, debug_buf[0]);
			normalize(debug_buf[0], m_fftSize);

			ifftt->execute((float*)sigSpectrum, debug_buf[1]);
			normalize(debug_buf[1], m_fftSize);


			writeAudiofile("r:\mic2f2t.WAV", debug_buf[0], debug_buf[1], m_fftSize, 44100);
			*/

			/*

			writeAudiofile("debug.WAV", debug_buf[0], debug_buf[1], m_fftSize, 44100);

			fftt->execute(m_inputSignalHistoryTime->getPtrT(0), debug_buf[0]);

			for (int j = 0; j < m_fftSize; j++) {
				debug_buf[1][j] = debug_buf[0][j * 2 +1];
			}

			normalize(debug_buf[1], m_fftSize);

			writeAudiofile("debug2.WAV", debug_buf[0], m_fftSize, 44100);
			writeAudiofile("debug22.WAV", debug_buf[1], m_fftSize, 44100);


			ifftt->execute(debug_buf[0], debug_buf[1]);
			for (int j = 0; j < m_fftSize; j++) {
				debug_buf[1][j] = debug_buf[1][j] * 0.00001f;
			}
			writeAudiofile("debug3.WAV", debug_buf[1], m_fftSize, 44100);
			*/

		printf("written\n\n");
		//exit(0);

		m_signalHistoryPool.reset();
	}
}



