#pragma once

#include <vector>
#include <queue>

#include <jack/jack.h>

#include "SignalBuffer.h"


class AudioDriver
{
public:
	enum class Connect {
		ToCapture,
		ToPlayback,

		ToPlaybackSource,		
	};

	class Request {
		typedef std::function<void()> Action;

		AudioDriver *driver;
		std::queue<Action> actions;

	public:
		Request(AudioDriver *driver) : driver(driver) {}

		Request &addSignal(SignalBuffer *buffer, std::string name, Connect connection) {
			actions.push(std::bind(&AudioDriver::addSignal, driver, buffer, name, connection));
			return *this;
		}
		Request &addObserver(SignalBufferObserver *observer) {
			actions.push(std::bind(&AudioDriver::addObserver, driver, observer));
			return *this;
		}
		Request &remove(SignalBuffer *buffer) {
			actions.push(std::bind(&AudioDriver::removeSignal, driver, buffer));
			return *this;
		}
		Request &remove(SignalBufferObserver *observer) {
			actions.push(std::bind(&AudioDriver::removeObserver, driver, observer));
			return *this;
		}


		void execute(bool async=false) {
			PlatformLocalLock(driver->m_mtxActionQueue);
			driver->m_evtActionQueueProcessed.Reset();

			while (actions.size()) {
				driver->m_actionQueue.push(actions.front());
				actions.pop();
			}

			driver->m_newActions = true;

			if (!async) {
				driver->m_evtActionQueueProcessed.Wait();
			}
		}
	};

	AudioDriver();
	~AudioDriver();

	inline int getNumCaptureChannels() {return m_portsCaptureNum;}
	inline int getNumPlaybackChannels() {return m_portsPlaybacleNum;}

	inline int getSampleRate() { return m_sampleRate; }
	inline int getBlockSize() { return m_blockSize; }


	inline std::vector<jack_port_t*> NewInput(std::string baseName, int nChannels) {
		return newPorts(baseName, nChannels, false);
	}

	inline std::vector<jack_port_t*> NewOutput(std::string baseName, int nChannels) {
		return newPorts(baseName, nChannels, true);
	}


	void playOnceBlocking(float *samples, int len, int onChannel);

	static const int MAX_SIGNAL_BUFFERS = 16;
	static const int MAX_CHANNELS_PER_BUFFER = 4;
private:
	std::vector<jack_port_t*> newPorts(std::string baseName, int nChannels, bool outputNotInput);


	// API:
	void addSignal(SignalBuffer *buffer, std::string name, Connect connection);
	void addObserver(SignalBufferObserver *pool);
	void removeSignal(SignalBuffer *buffer);
	void removeObserver(SignalBufferObserver *buffer);

	static int jackProcess(jack_nframes_t nframes, void *arg);
	static void jackShutdown(void *arg);

	jack_client_t* initJack();


	//std::vector<SignalBuffer>

	jack_client_t *m_jackClient;
	int m_sampleRate;
	int m_blockSize;


	const char **m_portsCapture;
	const char **m_portsPlayback;
	int m_portsCaptureNum, m_portsPlaybacleNum;

	bool m_running;
	unsigned long m_totalFramesProcessed;

	SignalBuffer *m_buffers[MAX_SIGNAL_BUFFERS];
	SignalBufferObserver *m_bufferPool[MAX_SIGNAL_BUFFERS];


	volatile bool m_newActions;
	PlatformMutex m_mtxActionQueue;
	PlatformThreadEvent m_evtActionQueueProcessed;
	std::queue<std::function<void()>> m_actionQueue;

	struct BufferPortConnection {
		jack_port_t *port;
		bool isOutput;
	};

	BufferPortConnection m_bufferPortConnections[MAX_SIGNAL_BUFFERS*MAX_CHANNELS_PER_BUFFER];

	inline BufferPortConnection *getBufferPortConnection(int bufferId, int channel) {
		return &m_bufferPortConnections[MAX_SIGNAL_BUFFERS * bufferId + channel];
	}
};

