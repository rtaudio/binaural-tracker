#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdint.h>
#include <signal.h> 
#include <string>

#include <jack/jack.h>
#include <jack/types.h>
#include <jack/ringbuffer.h>

#include <platform.h>
#include <ITAFunctors.h>
#include <ITANumericUtils.h>
#include <ITAStringUtils.h>
#include <ITAFFT.h>
#include <FastMath.h>

#include "AudioDriver.h"
#include "SignalBuffer.h"


#define MAX_SPEAKER_DISTANCE 5 //meters
#define C_SOUND 343


class BinarualTracker {
public:
	BinarualTracker();
	void Calibrate();
	inline bool IsRunning() { return m_isRunning; }

	PlatformThread *m_updateThread;

private:
	AudioDriver driver;

	SignalBuffer *m_SignalBuffer;
	SignalBuffer *m_micBuffer;
	SignalBufferObserver m_signalHistoryPool;

	jack_port_t** m_portArraySignal;
	jack_port_t** m_portArrayMic;

	void _updateThreadMain(void *arg);


	bool m_isRunning;
	int m_fftSize;
};