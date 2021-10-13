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
	if(rc){
		exit(1);
	}else{
		dimq_subscribe(dimq, NULL, "response/topic", 0);
	}
}

void on_subscribe(struct dimq *dimq, void *obj, int mid, int qos_count, const int *granted_qos)
{
	dimq_property *props = NULL;
	dimq_property_add_string(&props, MQTT_PROP_RESPONSE_TOPIC, "response/topic");
	dimq_publish_v5(dimq, NULL, "request/topic", 6, "action", 0, 0, props);
	dimq_property_free_all(&props);
}

void on_message(struct dimq *dimq, void *obj, const struct dimq_message *msg)
{
	if(!strcmp(msg->payload, "a response")){
		run = 0;
	}else{
		run = 1;
	}
}

int main(int argc, char *argv[])
{
	int rc;
	struct dimq *dimq;
	int ver = PROTOCOL_VERSION_v5;

	int port = atoi(argv[1]);

	dimq_lib_init();

	dimq = dimq_new("request-test", true, NULL);
	if(dimq == NULL){
		return 1;
	}
	dimq_opts_set(dimq, dimq_OPT_PROTOCOL_VERSION, &ver);
	dimq_connect_callback_set(dimq, on_connect);
	dimq_subscribe_callback_set(dimq, on_subscribe);
	dimq_message_callback_set(dimq, on_message);

	rc = dimq_connect(dimq, "localhost", port, 60);

	while(run == -1){
		rc = dimq_loop(dimq, -1, 1);
	}
	dimq_destroy(dimq);

	dimq_lib_cleanup();
	return run;
}
