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
		dimq_subscribe(dimq, NULL, "unsubscribe/test", 2);
	}
}

void on_subscribe(struct dimq *dimq, void *obj, int mid, int sub_count, const int *subs)
{
	char *unsubs[] = {"unsubscribe/test", "no-sub"};

	dimq_unsubscribe_multiple(dimq, NULL, 2, unsubs, NULL);
}

void on_disconnect(struct dimq *dimq, void *obj, int rc)
{
	run = rc;
}

void on_unsubscribe(struct dimq *dimq, void *obj, int mid)
{
	dimq_disconnect(dimq);
}


int main(int argc, char *argv[])
{
	int rc;
	struct dimq *dimq;

	int port = atoi(argv[1]);

	dimq_lib_init();

	dimq = dimq_new("unsubscribe-test", true, NULL);
	if(dimq == NULL){
		return 1;
	}
	dimq_int_option(dimq, dimq_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
	dimq_connect_callback_set(dimq, on_connect);
	dimq_disconnect_callback_set(dimq, on_disconnect);
	dimq_subscribe_callback_set(dimq, on_subscribe);
	dimq_unsubscribe_callback_set(dimq, on_unsubscribe);

	rc = dimq_connect(dimq, "localhost", port, 60);

	while(run == -1){
		dimq_loop(dimq, -1, 1);
	}
	dimq_destroy(dimq);

	dimq_lib_cleanup();
	return run;
}
