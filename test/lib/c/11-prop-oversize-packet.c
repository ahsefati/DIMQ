#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dimq.h>

static int run = -1;
static int sent_mid = -1;

void on_connect(struct dimq *dimq, void *obj, int rc)
{
	if(rc){
		exit(1);
	}else{
		rc = dimq_subscribe(dimq, NULL, "0123456789012345678901234567890", 0);
		if(rc != dimq_ERR_OVERSIZE_PACKET){
			printf("Fail on subscribe\n");
			exit(1);
		}

		rc = dimq_unsubscribe(dimq, NULL, "0123456789012345678901234567890");
		if(rc != dimq_ERR_OVERSIZE_PACKET){
			printf("Fail on unsubscribe\n");
			exit(1);
		}

		rc = dimq_publish(dimq, &sent_mid, "pub/test", strlen("0123456789012345678"), "0123456789012345678", 0, false);
		if(rc != dimq_ERR_OVERSIZE_PACKET){
			printf("Fail on publish 1\n");
			exit(1);
		}
		rc = dimq_publish(dimq, &sent_mid, "pub/test", strlen("012345678901234567"), "012345678901234567", 0, false);
		if(rc != dimq_ERR_SUCCESS){
			printf("Fail on publish 2\n");
			exit(1);
		}
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
	struct dimq *dimq;

	int port = atoi(argv[1]);

	dimq_lib_init();

	dimq = dimq_new("publish-qos0-test", true, NULL);
	if(dimq == NULL){
		return 1;
	}
	dimq_int_option(dimq, dimq_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
	dimq_connect_callback_set(dimq, on_connect);
	dimq_publish_callback_set(dimq, on_publish);

	rc = dimq_connect(dimq, "localhost", port, 60);

	while(run == -1){
		rc = dimq_loop(dimq, -1, 1);
	}
	dimq_destroy(dimq);

	dimq_lib_cleanup();
	return run;
}
