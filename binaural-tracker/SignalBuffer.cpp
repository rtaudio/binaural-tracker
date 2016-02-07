#include "SignalBuffer.h"





void normalize(float *vector, int len) {
	float max = 0;

	for (int i = 0; i < len; i++) {
		float a = fabsf(vector[i]);
		if (a > max)
			max = a;
	}

	if (max == 0.0f || max == 1.0f)
		return;

	float q = 1.0f / max;

	for (int i = 0; i < len; i++) {
		vector[i] = vector[i] * q;
	}
}


void denoise(float *vector, int len) {
	int filterLen = 4;

	for (int i = 0; i < len; i++) {
		float m = 0;
		for (int j = i - filterLen + 1; j <= i; j++) {
			m += (j < 0) ? 0 : vector[j];
		}
		m /= (float)filterLen;
		vector[i] = m;
	}
}



typedef float element;
//   1D MEDIAN FILTER implementation
//     signal - input signal
//     result - output signal
//     N      - length of the signal
void _medianfilter(const element* signal, element* result, int N)
{
	//   Move window through all elements of the signal
	for (int i = 2; i < N - 2; ++i)
	{
		//   Pick up window elements
		element window[5];
		for (int j = 0; j < 5; ++j)
			window[j] = signal[i - 2 + j];
		//   Order elements (only half of them)
		for (int j = 0; j < 3; ++j)
		{
			//   Find position of minimum element
			int min = j;
			for (int k = j + 1; k < 5; ++k)
				if (window[k] < window[min])
					min = k;
			//   Put found minimum element in its place
			const element temp = window[j];
			window[j] = window[min];
			window[min] = temp;
		}
		//   Get result - the middle element
		result[i - 2] = window[2];
	}
}

//   1D MEDIAN FILTER wrapper
//     signal - input signal
//     result - output signal
//     N      - length of the signal
void medianfilter(element* signal, element* result, int N)
{
	//   Check arguments
	if (!signal || N < 1)
		return;
	//   Treat special case N = 1
	if (N == 1)
	{
		if (result)
			result[0] = signal[0];
		return;
	}
	//   Allocate memory for signal extension
	element* extension = new element[N + 4];
	//   Check memory allocation
	if (!extension)
		return;
	//   Create signal extension
	memcpy(extension + 2, signal, N * sizeof(element));
	for (int i = 0; i < 2; ++i)
	{
		extension[i] = signal[1 - i];
		extension[N + 2 + i] = signal[N - 1 - i];
	}
	//   Call median filter implementation
	_medianfilter(extension, result ? result : signal, N + 4);
	//   Free memory
	delete[] extension;
}
