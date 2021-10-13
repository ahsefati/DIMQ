#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dimq.h>

/* dimq_connect_async() test, with dimq_loop_start() called before dimq_connect_async(). */

static int run = -1;
static bool should_run = true;

void on_connect(struct dimq *dimq, void *obj, int rc)
{
	if(rc){
		exit(1);
	}else{
		dimq_subscribe(dimq, NULL, "qos1/test", 1);
	}
}

void on_disconnect(struct dimq *dimq, void *obj, int rc)
{
	run = rc;
}

void on_subscribe(struct dimq *dimq, void *obj, int mid, int qos_count, const int *granted_qos)
{
	//dimq_disconnect(dimq);
	should_run = false;
}

int main(int argc, char *argv[])
{
	int rc;
	struct dimq *dimq;

	int port = atoi(argv[1]);

	dimq_lib_init();

	dimq = dimq_new("subscribe-qos1-test", true, NULL);
	if(dimq == NULL){
		return 1;
	}
	dimq_connect_callback_set(dimq, on_connect);
	dimq_disconnect_callback_set(dimq, on_disconnect);
	dimq_subscribe_callback_set(dimq, on_subscribe);

	rc = dimq_loop_start(dimq);
	if(rc){
		printf("loop_start failed: %s\n", dimq_strerror(rc));
		return rc;
	}
	
	rc = dimq_connect_async(dimq, "localhost", port, 60);
	if(rc){
		printf("connect_async failed: %s\n", dimq_strerror(rc));
		return rc;
	}

	/* 50 millis to be system polite */
	struct timespec tv = { 0, 50e6 };
	while(should_run){
		nanosleep(&tv, NULL);
	}

	dimq_disconnect(dimq);
	dimq_loop_stop(dimq, false);
	dimq_destroy(dimq);

	dimq_lib_cleanup();
	return run;
}
