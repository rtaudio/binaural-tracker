#pragma once

#include <string>

class SignalBuffer;

class AudioUtils
{
public:
	AudioUtils();
	~AudioUtils();

	


	static uint32_t wavRead(std::string fileName, float *ch0, uint32_t maxLength);

	static void wavWrite(std::string fileName, float *ch0, uint32_t length);
	static void wavWrite(std::string fileName, float *ch0, float *ch1, uint32_t length);
	static void wavWrite(std::string fileName, SignalBuffer &signal);
};

