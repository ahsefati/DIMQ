#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dimq.h>

static int run = -1;

void on_connect(struct dimq *dimq, void *obj, int rc)
{
	if(rc){
		exit(1);
	}else{
		rc = dimq_publish(dimq, NULL, "maximum/qos/qos2", strlen("message"), "message", 2, false);
		if(rc != dimq_ERR_QOS_NOT_SUPPORTED) run = 1;
		rc = dimq_publish(dimq, NULL, "maximum/qos/qos1", strlen("message"), "message", 1, false);
		if(rc != dimq_ERR_QOS_NOT_SUPPORTED) run = 1;
		rc = dimq_publish(dimq, NULL, "maximum/qos/qos0", strlen("message"), "message", 0, false);
		if(rc != dimq_ERR_SUCCESS) run = 1;
	}
}

void on_publish(struct dimq *dimq, void *obj, int mid)
{
	if(mid == 1){
		dimq_disconnect(dimq);
	}
}

void on_disconnect(struct dimq *dimq, void *obj, int rc)
{
	run = 0;
}

int main(int argc, char *argv[])
{
	int rc;
	struct dimq *dimq;

	int port = atoi(argv[1]);

	dimq_lib_init();

	dimq = dimq_new("publish-qos2-test", true, NULL);
	if(dimq == NULL){
		return 1;
	}
	dimq_int_option(dimq, dimq_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
	dimq_connect_callback_set(dimq, on_connect);
	dimq_disconnect_callback_set(dimq, on_disconnect);
	dimq_publish_callback_set(dimq, on_publish);

	rc = dimq_connect(dimq, "localhost", port, 60);

	while(run == -1){
		dimq_loop(dimq, 50, 1);
	}

	dimq_destroy(dimq);
	dimq_lib_cleanup();
	return run;
}
