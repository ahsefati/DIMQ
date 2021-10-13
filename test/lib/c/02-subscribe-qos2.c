#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <dimq.h>

static int run = -1;

void on_connect(struct dimq *dimq, void *obj, int rc)
{
	if(rc){
		exit(1);
	}else{
		dimq_subscribe(dimq, NULL, "qos2/test", 2);
	}
}

void on_disconnect(struct dimq *dimq, void *obj, int rc)
{
	run = rc;
}

void on_subscribe(struct dimq *dimq, void *obj, int mid, int qos_count, const int *granted_qos)
{
	dimq_disconnect(dimq);
}

int main(int argc, char *argv[])
{
	int rc;
	struct dimq *dimq;

	int port = atoi(argv[1]);

	dimq_lib_init();

	dimq = dimq_new("subscribe-qos2-test", true, NULL);
	if(dimq == NULL){
		return 1;
	}
	dimq_connect_callback_set(dimq, on_connect);
	dimq_disconnect_callback_set(dimq, on_disconnect);
	dimq_subscribe_callback_set(dimq, on_subscribe);

	rc = dimq_connect(dimq, "localhost", port, 60);

	while(run == -1){
		dimq_loop(dimq, -1, 1);
	}

	dimq_destroy(dimq);
	dimq_lib_cleanup();
	return run;
}
