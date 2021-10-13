#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dimq.h>

static int run = -1;

void on_connect(struct dimq *dimq, void *obj, int rc)
{
	if(rc){
		printf("Connect error: %d\n", rc);
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	int rc;
	struct dimq *dimq;

	int port = atoi(argv[1]);

	dimq_lib_init();

	dimq = dimq_new("publish-qos1-test", true, &run);
	if(dimq == NULL){
		return 1;
	}
	dimq_connect_callback_set(dimq, on_connect);

	rc = dimq_connect(dimq, "localhost", port, 5);

	while(run == -1){
		rc = dimq_loop(dimq, 300, 1);
		if(rc){
			exit(0);
		}
	}

	dimq_lib_cleanup();
	return 0;
}
