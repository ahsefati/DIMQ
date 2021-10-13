#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dimq.h>

static int run = -1;
static int sent_mid;

void on_connect(struct dimq *dimq, void *obj, int rc)
{
	if(rc){
		exit(1);
	}else{
		dimq_publish(dimq, &sent_mid, "psk/test", strlen("message"), "message", 0, false);
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

void on_disconnect(struct dimq *dimq, void *obj, int rc)
{
	run = rc;
}

int main(int argc, char *argv[])
{
	int rc;
	int port;
	struct dimq *dimq;

	dimq_lib_init();

	port = atoi(argv[1]);

	dimq = dimq_new("08-tls-psk-pub", true, NULL);
	dimq_tls_opts_set(dimq, 1, "tlsv1", NULL);
	rc = dimq_tls_psk_set(dimq, "deadbeef", "psk-id", NULL);
	if(rc){
		dimq_destroy(dimq);
		return rc;
	}
	dimq_connect_callback_set(dimq, on_connect);
	dimq_disconnect_callback_set(dimq, on_disconnect);
	dimq_publish_callback_set(dimq, on_publish);

	rc = dimq_connect(dimq, "localhost", port, 60);
	if(rc){
		dimq_destroy(dimq);
		return rc;
	}

	while(run == -1){
		dimq_loop(dimq, -1, 1);
	}

	dimq_destroy(dimq);

	dimq_lib_cleanup();
	return run;
}
