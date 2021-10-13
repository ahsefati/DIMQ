#include <errno.h>
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
		dimq_disconnect(dimq);
	}
}

void on_disconnect(struct dimq *dimq, void *obj, int rc)
{
	run = rc;
}

int main(int argc, char *argv[])
{
	int rc;
	struct dimq *dimq;

	int port = atoi(argv[1]);

	dimq_lib_init();

	dimq = dimq_new("08-ssl-connect-crt-auth", true, NULL);
	if(dimq == NULL){
		return 1;
	}
	dimq_tls_set(dimq, "../ssl/test-root-ca.crt", "../ssl/certs", "../ssl/client.crt", "../ssl/client.key", NULL);
	dimq_connect_callback_set(dimq, on_connect);
	dimq_disconnect_callback_set(dimq, on_disconnect);

	rc = dimq_connect(dimq, "localhost", port, 60);

	while(run == -1){
		dimq_loop(dimq, -1, 1);
	}
	dimq_destroy(dimq);

	dimq_lib_cleanup();
	return run;
}
