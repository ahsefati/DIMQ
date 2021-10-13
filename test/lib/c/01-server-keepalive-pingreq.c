#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <dimq.h>

static int run = -1;

void on_connect(struct dimq *dimq, void *obj, int rc)
{
	if(rc){
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	int rc;
	struct dimq *dimq;

	int port = atoi(argv[1]);

	dimq_lib_init();

	dimq = dimq_new("01-server-keepalive-pingreq", true, NULL);
	if(dimq == NULL){
		return 1;
	}
	dimq_int_option(dimq, dimq_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
	dimq_connect_callback_set(dimq, on_connect);

	rc = dimq_connect(dimq, "localhost", port, 60);

	while(run == -1){
		dimq_loop(dimq, -1, 1);
	}

	dimq_destroy(dimq);
	dimq_lib_cleanup();
	return run;
}
