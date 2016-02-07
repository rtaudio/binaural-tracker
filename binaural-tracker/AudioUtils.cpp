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


void AudioUtils::wavWrite(std::string fileName, float *ch0, float *ch1, uint32_t length)
{
	writeAudiofile(fileName, ch0, ch1, length, 48000);
}
