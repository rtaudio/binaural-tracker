#include "AudioUtils.h"
#include <ITAAudiofileReader.h>
#include <ITAAudiofileWriter.h>

#include "SignalBuffer.h"

#include <algorithm>

AudioUtils::AudioUtils()
{
}


AudioUtils::~AudioUtils()
{
}


uint32_t AudioUtils::wavRead(std::string fileName, float *ch0, uint32_t maxLength)
{
	ITAAudiofileProperties props;
	auto data = readAudiofile(fileName, props);

	memset(ch0, 0, sizeof(float)*maxLength);

	int n = (std::min)(maxLength, props.uiLength);
	memcpy(ch0, data[0], n*sizeof(float));

	// readAudiofile creates buffers
	for (auto v : data) {
		delete[] v;
	}

	return n;
}


void AudioUtils::wavWrite(std::string fileName, float *ch0, uint32_t length)
{
	writeAudiofile(fileName, ch0, length, 48000);
}

void AudioUtils::wavWrite(std::string fileName, SignalBuffer &signal)
{
	writeAudiofile(fileName, signal.getPtrTS(0), signal.getPtrTS(1), signal.size, 48000);
}

void AudioUtils::wavWrite(std::string fileName, SignalBufferObserver &observer)
{
	auto allData = observer.getTimeStageAll();
	ITAAudiofileProperties props;
	props.uiChannels = allData.size();
	props.uiLength = observer.m_hists[0]->size;
	props.dSamplerate = 48000;
	props.eDomain = ITA_TIME_DOMAIN;
	props.eQuantization = ITA_INT16;
	props.sComment = "";
	for (auto h : observer.m_hists) {
		props.sComment += h->name+", ";
	}
	props.sComment = props.sComment.substr(0, props.sComment.length() - 2);

	writeAudiofile(fileName, props, allData);
}


void AudioUtils::wavWrite(std::string fileName, float *ch0, float *ch1, uint32_t length)
{
	writeAudiofile(fileName, ch0, ch1, length, 48000);
}
