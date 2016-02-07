#pragma once

#include "AudioDriver.h"

class Calibration
{
	static const int NumberRepetions = 3;
	static const int WaitBetweenRepetitions = 4000; //ms
public:
	Calibration(AudioDriver *driver);
	~Calibration();

private:
	std::vector<jack_port_t*> m_out_ports;
};

