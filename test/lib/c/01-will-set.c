#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dimq.h>

static int run = -1;
int main(int argc, char *argv[])
{
	int rc;
	struct dimq *dimq;

	int port = atoi(argv[1]);

	dimq_lib_init();

	dimq = dimq_new("01-will-set", true, NULL);
	if(dimq == NULL){
		return 1;
	}
	dimq_will_set(dimq, "topic/on/unexpected/disconnect", strlen("will message"), "will message", 1, true);

	rc = dimq_connect(dimq, "localhost", port, 60);

	while(run == -1){
		dimq_loop(dimq, -1, 1);
	}
	dimq_destroy(dimq);

	dimq_lib_cleanup();
	return run;
}
