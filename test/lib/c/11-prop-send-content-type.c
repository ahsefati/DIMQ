#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dimq.h>
#include <mqtt_protocol.h>

static int run = -1;
static int sent_mid = -1;

void on_connect(struct dimq *dimq, void *obj, int rc)
{
	int rc2;
	dimq_property *proplist = NULL;

	if(rc){
		exit(1);
	}else{
		rc2 = dimq_property_add_string(&proplist, MQTT_PROP_CONTENT_TYPE, "application/json");
		dimq_publish_v5(dimq, &sent_mid, "prop/qos0", strlen("message"), "message", 0, false, proplist);
		dimq_property_free_all(&proplist);
	}
}

void on_publish(struct dimq *dimq, void *obj, int mid)
{
	if(mid == sent_mid){
		dimq_disconnect(dimq);
		run = 0;
	}else{
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	int rc;
	int tmp;
	struct dimq *dimq;

	int port = atoi(argv[1]);

	dimq_lib_init();

	dimq = dimq_new("prop-test", true, NULL);
	if(dimq == NULL){
		return 1;
	}
	dimq_connect_callback_set(dimq, on_connect);
	dimq_publish_callback_set(dimq, on_publish);
	tmp = MQTT_PROTOCOL_V5;
	dimq_opts_set(dimq, dimq_OPT_PROTOCOL_VERSION, &tmp);

	rc = dimq_connect(dimq, "localhost", port, 60);

	while(run == -1){
		rc = dimq_loop(dimq, -1, 1);
	}

	dimq_destroy(dimq);
	dimq_lib_cleanup();
	return run;
}
