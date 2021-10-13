#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dimq.h>

static int run = -1;

void on_connect(struct dimq *dimq, void *obj, int rc, int flags, const dimq_property *properties)
{
	int i;

	if(rc){
		exit(1);
	}

	for(i=0; i<6; i++){
		dimq_publish_v5(dimq, NULL, "topic", 5, "12345", 1, false, NULL);
	}
}

void on_publish(struct dimq *dimq, void *obj, int mid, int reason_code, const dimq_property *properties)
{
	if(mid == 6){
		dimq_disconnect(dimq);
		run = 0;
	}
}

int main(int argc, char *argv[])
{
	int rc;
	struct dimq *dimq;
	dimq_property *props = NULL;

	int port = atoi(argv[1]);

	dimq_lib_init();

	dimq = dimq_new("publish-qos1-test", true, &run);
	if(dimq == NULL){
		return 1;
	}
	dimq_int_option(dimq, dimq_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);

	dimq_connect_v5_callback_set(dimq, on_connect);
	dimq_publish_v5_callback_set(dimq, on_publish);

	rc = dimq_connect_bind_v5(dimq, "localhost", port, 60, NULL, NULL);

	while(run == -1){
		dimq_loop(dimq, 300, 1);
	}

	dimq_destroy(dimq);
	dimq_lib_cleanup();
	return run;
}
