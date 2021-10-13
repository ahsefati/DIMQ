#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <dimq.h>

static int run = -1;

void on_connect(struct dimq *dimq, void *obj, int rc)
{
	exit(1);
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
	dimq_tls_set(dimq, "../ssl/test-fake-root-ca.crt", NULL, "../ssl/client.crt", "../ssl/client.key", NULL);
	dimq_connect_callback_set(dimq, on_connect);

	rc = dimq_connect(dimq, "localhost", port, 60);

	rc = dimq_loop_forever(dimq, -1, 1);
	dimq_destroy(dimq);
	dimq_lib_cleanup();
	if(rc == dimq_ERR_ERRNO && errno == EPROTO){
		return 0;
	}else{
		return 1;
	}
}

