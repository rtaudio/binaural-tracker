#pragma once

#include "AudioDriver.h"

class Calibration
{
	static const int NumberRepetions = 2;
	static const int WaitBetweenRepetitions = 0; //ms
public:
	Calibration(AudioDriver *driver);
	~Calibration();

private:
	std::vector<jack_port_t*> m_out_ports;
};

