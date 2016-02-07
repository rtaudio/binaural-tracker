#pragma once
#include <stdint.h>
#include <vector>

#include<platform.h>
#include <ITAFFT.h>

void normalize(float *vector, int len);
void medianfilter(float* signal, float* result, int N);
void denoise(float *vector, int len);

struct SignalBuffer {
	std::string name;

	float *m_timeQueue;
	uint32_t m_timeQueuePointer;

	float *m_timeStage;

	float *m_freq;
	uint32_t size, channels;

	ITAFFT *m_fft;

	

	inline float * getPtrTQ(uint32_t c) {
		return &m_timeQueue[(size + 0)*c];
	}

	inline float * getPtrTS(uint32_t c) {
		return &m_timeStage[(size + 1)*c];
	}


	inline float * getPtrF(uint32_t c) {
		return &m_freq[(size + 1) * 2 * c]; // complex!
	}


	SignalBuffer(uint32_t nChannels, uint32_t size) : size(size), channels(nChannels), m_timeQueuePointer(0) {
		m_timeQueue = new float[nChannels*(size + 0)];
		m_timeStage = new float[nChannels*(size + 1)];

		m_freq = new float[nChannels*(size + 1) * 2]; // complex!

		memset(m_timeQueue, 0, nChannels*(size + 0)*sizeof(float));
		memset(m_freq, 0, nChannels*(size + 1) * 2 * sizeof(float));

		// pass dummy in/out pointers
		m_fft = new ITAFFT(ITAFFT::FFT_R2C, (int)size, getPtrTS(0), getPtrF(0));
	}

	SignalBuffer(float *samples, int len) : size(len), channels(1), m_timeQueuePointer(0)
	{
		m_timeQueue = new float[1 * (len + 1)];
		m_timeStage = NULL;
		memcpy(m_timeQueue, samples, len*sizeof(float));
	}

	SignalBuffer(std::vector<float *> samples, int len) : size(len), channels(samples.size()), m_timeQueuePointer(0)
	{
		m_timeQueue = new float[samples.size() * (len + 1)];
		m_timeStage = NULL;

		int c = 0;
		for (auto ch : samples) {
			if(ch)
				memcpy(getPtrTQ(c), ch, len*sizeof(float));
			else
				memset(getPtrTQ(c), 0, len*sizeof(float));
			c++;
		}
	}

	// returns number of samples until full
	void addBlock(uint32_t channel, float *block, uint32_t length) {
		if (channel >= channels)
			throw "Invalid channel number!";

		if (length > size || length == 0)
			throw "Invalid block size!";

		uint32_t untilEnd = size - m_timeQueuePointer;


		if (length > untilEnd) {
			// wrap around: need to copy in two ops
			memcpy(&getPtrTQ(channel)[m_timeQueuePointer], block, untilEnd*sizeof(float));
			memcpy(&getPtrTQ(channel)[0], block + untilEnd, (length - untilEnd)*sizeof(float));
		}
		else {
			memcpy(&getPtrTQ(channel)[m_timeQueuePointer], block, length*sizeof(float));
		}

		if (channel == channels - 1) {
			m_timeQueuePointer += length;
			m_timeQueuePointer = m_timeQueuePointer % size;
		}
	}

	void getBlock(uint32_t channel, float *block, uint32_t length) {
		if (channel >= channels)
			throw "Invalid channel number!";

		if (length > size || length == 0)
			throw "Invalid block size!";

		uint32_t untilEnd = size - m_timeQueuePointer;


		if (length > untilEnd) {
			// wrap around: need to copy in two ops
			memcpy(block, &getPtrTQ(channel)[m_timeQueuePointer], untilEnd*sizeof(float));
			memcpy(block + untilEnd, &getPtrTQ(channel)[0], (length - untilEnd)*sizeof(float));
		}
		else {
			memcpy(block, &getPtrTQ(channel)[m_timeQueuePointer], length*sizeof(float));
		}

		if (channel == channels - 1) {
			m_timeQueuePointer += length;
			m_timeQueuePointer = m_timeQueuePointer % size;
		}
	}

	void stage() {
		if (!m_timeStage)
			return;

		for (uint32_t c = 0; c < channels; c++)
		{
			if (m_timeQueuePointer == 0) {
				memcpy(getPtrTS(c), getPtrTQ(c), size*sizeof(float));
			}
			else {
				uint32_t untilEnd = size - m_timeQueuePointer;
				memcpy(getPtrTS(c), &getPtrTQ(c)[m_timeQueuePointer], untilEnd*sizeof(float));
				memcpy(getPtrTS(c) + untilEnd, getPtrTQ(c), (size - untilEnd)*sizeof(float));
			}
		}
	}

	void preProcessTime(int channel)
	{
		// time-domain-pre-processing
		//medianfilter(getPtrT(channel), NULL, size);
		//denoise(getPtrT(channel), size);
		//denoise(getPtrT(channel), size);
		::normalize(getPtrTS(channel), size);
	}

	void preProcessFreq(int channel, bool complexConjugate)
	{
		float * f = getPtrF(channel);


		// freq-domain pre-processing
		uint32_t rampUnti = size / 64;
		uint32_t maskUntil = rampUnti / 2;


		// completely mask [0, size/256]
		for (uint32_t i = 0; i < maskUntil; i++) {
			f[(i * 2) + 0] = 0;
			f[(i * 2) + 1] = 0;
		}


		// linear rampk [size/256, size/6]
		for (uint32_t i = maskUntil; i < size / 128; i++) {
			f[(i * 2) + 0] *= ((float)i - (float)maskUntil) / (float)(rampUnti - maskUntil);
			f[(i * 2) + 1] *= ((float)i - (float)maskUntil) / (float)(rampUnti - maskUntil);
		}

		if (complexConjugate) {
			for (uint32_t i = 0; i < size; i++) {
				f[(i * 2) + 1] = -f[(i * 2) + 1];
			}
		}
	}

	const float *getFreq(uint32_t channel, uint32_t length, bool complexConjugate = false) {
		if (channel >= channels)
			throw "Invalid channel number!";

		if (length != size)
			throw "Invalid length!";

		preProcessTime(channel);

		m_fft->execute(getPtrTS(channel), getPtrF(channel));

		preProcessFreq(channel, complexConjugate);


		return getPtrF(channel);
	}

	void resetIterator() {
		m_timeQueuePointer = 0;
	}

	void normalize() {
		for (uint32_t c = 0; c < channels; c++) {
			::normalize(getPtrTS(c), size);
		}
	}
};

struct SignalBufferObserver {
	PlatformThreadEvent m_evCommit;
	std::vector<SignalBuffer*> m_hists;

	uint32_t updateInterval;
	uint32_t lastUpdate;

	volatile bool commiting;


	SignalBufferObserver(SignalBuffer *buf = NULL) :commiting(false), updateInterval(-1), lastUpdate(-1) {
		if (buf) {
			m_hists.push_back(buf);
			updateInterval = buf->size;
		}
	}

	SignalBufferObserver(SignalBuffer &buf) :commiting(false), updateInterval(-1), lastUpdate(-1) {
		m_hists.push_back(&buf);
		updateInterval = buf.size;
	}

	SignalBufferObserver(SignalBuffer &buf, SignalBuffer &buf2) :commiting(false), updateInterval(-1), lastUpdate(-1) {
		if (buf.size != buf2.size)
			throw "Cannot observer signals of different sizes!";

		m_hists.push_back(&buf);
		m_hists.push_back(&buf2);
		updateInterval = buf.size;
	}

	void addHist(SignalBuffer* h) {
		m_hists.push_back(h);
	}

	bool commit() {
		if (commiting)
			return false;

		commiting = true;

		for (auto h : m_hists) {
			h->stage();
		}

		m_evCommit.Signal();
		return true;
	}

	void reset() {
		if (!commiting)
			throw "Called commited() but not commiting!";

		m_evCommit.Reset();

		// why would we need to reset the it?
		//for (auto h : m_hists) {
//			h->resetIterator();
		//}

		commiting = false;
	}

	bool waitForCommit() {
		if (updateInterval == -1)
			throw "Waiting for a signal with undefined interval!";
		return m_evCommit.Wait() && commiting;
	}
};


