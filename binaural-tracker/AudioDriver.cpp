#include "AudioDriver.h"

#include <fstream>
#include <string>
#include <iostream>


AudioDriver::AudioDriver() :
	m_totalFramesProcessed(0),
	m_running(false),
	m_newActions(false)
{
	memset(m_buffers, 0, sizeof(m_buffers));
	memset(m_bufferPool, 0, sizeof(m_bufferPool));

	memset(m_bufferPortConnections, 0, sizeof(m_bufferPortConnections));


	m_jackClient = initJack();
	m_sampleRate = jack_get_sample_rate(m_jackClient);
	m_blockSize = jack_get_buffer_size(m_jackClient);

	m_running = true;

	m_mutedPorts = std::vector<PortArray>(m_portsPlaybacleNum, std::vector<const char *>());
}


AudioDriver::~AudioDriver()
{
	m_running = false;
	//jack_deactivate(m_jackClient);
	jack_client_close(m_jackClient);
}


void AudioDriver::MuteOthers(bool mute)
{
	PlatformLocalLock ll(m_mtxActionQueue);

	for (int ci = 0; ci < m_portsPlaybacleNum; ci++) {
		if (mute) {
			const char **portsAudioSource = jack_port_get_all_connections(m_jackClient, jack_port_by_name(m_jackClient, m_portsPlayback[ci]));
			if (!portsAudioSource || !portsAudioSource[0]) {
				continue;
			}

			while (auto p = *(portsAudioSource++)) {
				if (jack_disconnect(m_jackClient, p, m_portsPlayback[ci])) {
					throw "Cannot disconnect playback source from playback port!";
				}

				m_mutedPorts[ci].push_back(p);
			}
		}
		else {
			for (auto p : m_mutedPorts[ci]) {
				if (jack_connect(m_jackClient, p, m_portsPlayback[ci])) {
					throw "Cannot re-connect playback source to playback port!";
				}
			}
		}
	}

	commit(); // sync with driver
}

int uniquePtrArrayAdd(void **array, int len, void* ptr)
{
	int iEmpty = 0;
	for (int i = (len - 1); i >= 0; i--) {
		if (array[i] == ptr)
			return -1;
		if (array[i] == NULL)
			iEmpty = i;
	}

	array[iEmpty] = ptr;
	return iEmpty;
}

int uniquePtrArrayIndexOf(void **array, int len, void* ptr)
{
	for (int i = (len - 1); i >= 0; i--) {
		if (array[i] == ptr)
			return i;
	}

	return -1;
}



void AudioDriver::addObserver(SignalBufferObserver *pool)
{
	uniquePtrArrayAdd((void**)m_bufferPool, MAX_SIGNAL_BUFFERS, pool);
	pool->lastUpdate = -1;
}


void AudioDriver::addSignal(SignalBuffer *buffer, std::string name,  Connect connection)
{
	int ib = uniquePtrArrayAdd((void**)m_buffers, MAX_SIGNAL_BUFFERS, buffer);

	if (ib == -1)
		throw "Signal buffer already added!";

	buffer->name = name;

	bool isOutput = connection == Connect::ToPlayback;

	auto ports = newPorts(name, buffer->channels, isOutput);


	for (uint32_t c  = 0; c < buffer->channels; c++) {
		switch (connection) {
		case Connect::ToCapture:
			if (m_portsCapture[c] != NULL) {
				if (jack_connect(m_jackClient, m_portsCapture[c], jack_port_name(ports[c]))) {
					throw "Cannot connect input port";
				}
			}
			break;

		case Connect::ToPlaybackSource:

			// find ports connected to physical playback ports and "hook in"
			if (m_portsPlayback[c] != NULL) {
				const char **portsAudioSource = jack_port_get_all_connections(m_jackClient, jack_port_by_name(m_jackClient, m_portsPlayback[c]));
				if (!portsAudioSource || !portsAudioSource[0]) {
					throw ("Playplack port " + std::string(m_portsPlayback[c]) + " does not have any source.");
				}

				if (jack_connect(m_jackClient, portsAudioSource[0], jack_port_name(ports[c]))) {
					throw "Cannot connect input port to playback source!";
				}
			}

			break;

		case Connect::ToPlayback:
			if (m_portsPlayback[c] != NULL) {
				if (jack_connect(m_jackClient, jack_port_name(ports[c]), m_portsPlayback[c])) {
					throw "Cannot connect to playback port!";
				}
			}
		}

		auto con = getBufferPortConnection(ib, c);
		con->isOutput = isOutput;
		con->port = ports[c];
	}

	buffer->resetIterator();
}


void AudioDriver::removeSignal(SignalBuffer *buffer)
{
	int ib = uniquePtrArrayIndexOf((void**)m_buffers, MAX_SIGNAL_BUFFERS, buffer);
	if (ib < 0)
		throw "Tried to remove unknown SignalBuffer";

	for (uint32_t c = 0; c < buffer->channels; c++) {
		auto con = getBufferPortConnection(ib, c);
		if (!con->port)
			throw "remove: Port = NULL!";

		jack_port_unregister(m_jackClient, con->port);

		con->port = NULL;
		con->isOutput = false;
	}	

	m_buffers[ib] = NULL;

}


void AudioDriver::removeObserver(SignalBufferObserver *observer)
{
	int io = uniquePtrArrayIndexOf((void**)m_bufferPool, MAX_SIGNAL_BUFFERS, observer);
	if (io < 0)
		throw "Tried to remove unknown SignalBufferObserver";
	m_bufferPool[io] = NULL;
}



jack_client_t * AudioDriver::initJack()
{
	int portNum;
	jack_status_t status;


	jack_client_t *client = jack_client_open("binaural-tracker", JackNullOption, &status, NULL);
	if (client == NULL) {
		throw  "jack_client_open() failed";
	}

	jack_set_process_callback(client, &AudioDriver::jackProcess, this);
	jack_on_shutdown(client, &AudioDriver::jackShutdown, this);

	m_portsCapture = jack_get_ports(client, NULL, NULL, JackPortIsOutput | JackPortIsPhysical);
	if (m_portsCapture == NULL) {
		throw "No physical capture ports";
	}

	portNum = 0;
	while (m_portsCapture[++portNum]) {}
	m_portsCaptureNum = portNum;

	m_portsPlayback = jack_get_ports(client, NULL, NULL, JackPortIsInput | JackPortIsPhysical);
	if (m_portsPlayback == NULL) {
		throw "No physical playback ports";
	}

	portNum = 0;
	while (m_portsPlayback[++portNum]) {}
	m_portsPlaybacleNum = portNum;


	if (jack_activate(client)) {
		throw "Could not activate client";
	}


	return client;
}


std::vector<jack_port_t*> AudioDriver::newPorts(std::string baseName, int nChannels, bool outputNotInput)
{
	std::vector<jack_port_t*> ports;
	ports.reserve(nChannels);

	std::string namePattern = baseName + "%d";

	// Register ports
	for (int c = 0; c < nChannels; c++)
	{
		char portName[100];
		sprintf_s(portName, namePattern.c_str(), c);
		ports.push_back(jack_port_register(m_jackClient, portName, JACK_DEFAULT_AUDIO_TYPE, outputNotInput ? JackPortIsOutput : JackPortIsInput, 0));
		if (ports[c] == NULL) {
			throw "Could not create output port";
		}
	}

	return ports;
}


int AudioDriver::jackProcess(jack_nframes_t nframes, void *arg)
{
	AudioDriver *ad = (AudioDriver*)arg;

	if (!ad->m_running)
		return 1;

	if (ad->m_newActions) {	
		printf("AudioDriver: processing queue...\n");

		while (ad->m_actionQueue.size()) {
			try {
				ad->m_actionQueue.front()();
			}
			catch (const std::string &ex) {
				std::cout << "AudioDriver exception:" << ex;
			}
			ad->m_actionQueue.pop();
		}

		ad->m_newActions = false;
		ad->m_evtActionQueueProcessed.Signal();
	}


	for (int ib = 0; ib < MAX_SIGNAL_BUFFERS; ib++) {
		SignalBuffer *signalBuffer = ad->m_buffers[ib];

		if (!signalBuffer)
			continue;

		for (uint32_t ic = 0; ic < signalBuffer->channels; ic++) {
			auto con = ad->getBufferPortConnection(ib, ic);

			if (!con->port)
				continue;

			jack_default_audio_sample_t * block = (jack_default_audio_sample_t *)jack_port_get_buffer(con->port, nframes);

			if (block == NULL) {
				continue;
				//throw "Block is NULL!";
			}

			if (con->isOutput) {
				signalBuffer->getBlock(ic, block, nframes);
			} else {
				signalBuffer->addBlock(ic, block, nframes);
			}
		}
	}

	for (int ip = 0; ip < MAX_SIGNAL_BUFFERS; ip++) {
		auto bufferPool = ad->m_bufferPool[ip];

		if (!bufferPool)
			continue;

		if (bufferPool->lastUpdate == -1)
			bufferPool->lastUpdate = ad->m_totalFramesProcessed;

		if (bufferPool->updateInterval >= 0 && (ad->m_totalFramesProcessed - bufferPool->lastUpdate) > bufferPool->updateInterval) {
			bufferPool->lastUpdate = ad->m_totalFramesProcessed;
			if (!bufferPool->commit()) {
				printf("History comit failed! Update thread is too slow.\n");
				bufferPool->lastUpdate += bufferPool->updateInterval; // add penalty time
			}
		}
	}


	ad->m_totalFramesProcessed += nframes;

	return 0;
}

void AudioDriver::jackShutdown(void *arg)
{
	printf("Jack shutdown\n");
	auto ad = (AudioDriver*)arg;
	ad->m_running = false;
}
