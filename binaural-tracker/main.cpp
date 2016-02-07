#include "BinauralTracker.h"

bool g_isRunning ;

// Called when ctrl-c is pressed
void signalHandler(int sig)
{
	printf("Stopping\n");
	g_isRunning = false;
}


int main(int argc, char **argv) {
	g_isRunning = true;
	signal(SIGINT, signalHandler);

	BinarualTracker *tracker = new BinarualTracker();
	tracker->Calibrate();
	

	while (g_isRunning && tracker->IsRunning()) {
		usleep(1000 * 500);
	}

	return 0;
}