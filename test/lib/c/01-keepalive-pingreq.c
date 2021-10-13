#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <dimq.h>

static int run = -1;

void on_connect(struct dimq *dimq, void *obj, int rc)
{
	if(rc){
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	int rc;
	struct dimq *dimq;

	int port = atoi(argv[1]);

	dimq_lib_init();

	dimq = dimq_new("01-keepalive-pingreq", true, NULL);
	if(dimq == NULL){
		return 1;
	}
	dimq_connect_callback_set(dimq, on_connect);

	rc = dimq_connect(dimq, "localhost", port, 5);
	if(rc != 0) return rc;

	while(run == -1){
		rc = dimq_loop(dimq, -1, 1);
		if(rc != 0) break;
	}

	dimq_destroy(dimq);
	dimq_lib_cleanup();
	return run;
}
