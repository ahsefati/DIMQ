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
	}
}

void on_message(struct dimq *dimq, void *obj, const struct dimq_message *msg)
{
	if(msg->mid != 56){
		printf("Invalid mid (%d)\n", msg->mid);
		exit(1);
	}
	if(msg->qos != 2){
		printf("Invalid qos (%d)\n", msg->qos);
		exit(1);
	}
	if(strcmp(msg->topic, "len/qos2/test")){
		printf("Invalid topic (%s)\n", msg->topic);
		exit(1);
	}
	if(strcmp(msg->payload, "message")){
		printf("Invalid payload (%s)\n", (char *)msg->payload);
		exit(1);
	}
	if(msg->payloadlen != 7){
		printf("Invalid payloadlen (%d)\n", msg->payloadlen);
		exit(1);
	}
	if(msg->retain != false){
		printf("Invalid retain (%d)\n", msg->retain);
		exit(1);
	}

	dimq_disconnect(dimq);
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

	dimq = dimq_new("publish-qos2-test", true, &run);
	if(dimq == NULL){
		return 1;
	}
	dimq_int_option(dimq, dimq_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
	dimq_connect_callback_set(dimq, on_connect);
	dimq_disconnect_callback_set(dimq, on_disconnect);
	dimq_message_callback_set(dimq, on_message);

	rc = dimq_connect(dimq, "localhost", port, 60);

	while(run == -1){
		dimq_loop(dimq, 100, 1);
	}

	dimq_destroy(dimq);
	dimq_lib_cleanup();
	return run;
}
