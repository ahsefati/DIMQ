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
		dimq_publish(dimq, &sent_mid, "pub/qos0/test", strlen("message"), "message", 0, false);
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
