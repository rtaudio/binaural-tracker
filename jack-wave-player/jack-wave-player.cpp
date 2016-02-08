// jack-wave-player.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include <jack/jack.h>
#include <jack/types.h>

#include "ITAAudiofileReader.h"
#include "FastMath.h"

/** @file simple_client.c
*
* @brief This simple client demonstrates the most basic features of JACK
* as they would be used by many applications.
*/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include <jack/jack.h>

jack_port_t *output_port1, *output_port2;
jack_client_t *client;


ITAAudiofileProperties g_props, g_propsCarrier;
std::vector<float*> g_data, g_dataCarrier;
unsigned long g_waveIndex = 0;

/**
* The process callback for this JACK application is called in a
* special realtime thread once for each audio cycle.
*
* This client does nothing more than copy data from its input
* port to its output port. It will exit when stopped by
* the user (e.g. using Ctrl-C on a unix-ish operating system)
*/
int
process(jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *out1, *out2;

	out1 = (jack_default_audio_sample_t *)jack_port_get_buffer(output_port1, nframes);
	out2 = (jack_default_audio_sample_t *)jack_port_get_buffer(output_port2, nframes);

	int wi = (g_waveIndex) % (g_props.uiLength- nframes);
	int wiCarrier = (g_waveIndex) % (g_propsCarrier.uiLength - nframes);

	memcpy(out1, g_data[0] + wi, nframes*sizeof(jack_nframes_t));
	//if (g_props.uiChannels > 1)
	//	memcpy(out2, g_data[1] + wi, nframes*sizeof(jack_nframes_t));


	//fm_add(out1, g_dataCarrier[0] + wiCarrier, nframes);
	//if (g_props.uiChannels > 1)
	//	fm_add(out2, g_dataCarrier[1] + wiCarrier, nframes);

	g_waveIndex += nframes;


	return 0;
}

/**
* JACK calls this shutdown_callback if the server ever shuts down or
* decides to disconnect the client.
*/
void
jack_shutdown(void *arg)
{
	exit(1);
}




int
main(int argc, char *argv[])
{
	const char **ports;
	const char *client_name = "wave";
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;


	

	g_data = readAudiofile(argv[1], g_props);

	g_dataCarrier = readAudiofile("tracking_carrier.wav", g_propsCarrier);

	/* open a client connection to the JACK server */

	client = jack_client_open(client_name, options, &status, server_name);
	if (client == NULL) {
		fprintf(stderr, "jack_client_open() failed, "
			"status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf(stderr, "Unable to connect to JACK server\n");
		}
		exit(1);
	}
	if (status & JackServerStarted) {
		fprintf(stderr, "JACK server started\n");
	}
	if (status & JackNameNotUnique) {
		client_name = jack_get_client_name(client);
		fprintf(stderr, "unique name `%s' assigned\n", client_name);
	}

	/* tell the JACK server to call `process()' whenever
	there is work to be done.
	*/

	jack_set_process_callback(client, process, 0);

	/* tell the JACK server to call `jack_shutdown()' if
	it ever shuts down, either entirely, or if it
	just decides to stop calling us.
	*/

	jack_on_shutdown(client, jack_shutdown, 0);

	/* display the current sample rate.
	*/


	/* create two ports */

	output_port1 = jack_port_register(client, "output1",
		JACK_DEFAULT_AUDIO_TYPE,
		JackPortIsOutput, 0);
	output_port2 = jack_port_register(client, "output2",
		JACK_DEFAULT_AUDIO_TYPE,
		JackPortIsOutput, 0);


	/* Tell the JACK server that we are ready to roll.  Our
	* process() callback will start running now. */

	if (jack_activate(client)) {
		fprintf(stderr, "cannot activate client");
		exit(1);
	}

	/* Connect the ports.  You can't do this before the client is
	* activated, because we can't make connections to clients
	* that aren't running.  Note the confusing (but necessary)
	* orientation of the driver backend ports: playback ports are
	* "input" to the backend, and capture ports are "output" from
	* it.
	*/


	ports = jack_get_ports(client, NULL, NULL,
		JackPortIsPhysical | JackPortIsInput);
	if (ports == NULL) {
		fprintf(stderr, "no physical playback ports\n");
		exit(1);
	}

	if (jack_connect(client, jack_port_name(output_port1), ports[0])) {
		fprintf(stderr, "cannot connect output ports\n");
	}


	if (jack_connect(client, jack_port_name(output_port2), ports[1])) {
		fprintf(stderr, "cannot connect output ports\n");
	}



	/* keep running until stopped by the user */

	while (true) { Sleep(1); }

	/* this is never reached but if the program
	had some other way to exit besides being killed,
	they would be important to call.
	*/

	jack_client_close(client);
	exit(0);
}