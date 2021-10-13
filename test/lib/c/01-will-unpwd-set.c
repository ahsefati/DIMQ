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

	dimq = dimq_new("01-will-unpwd-set", true, NULL);
	if(dimq == NULL){
		return 1;
	}
	dimq_username_pw_set(dimq, "oibvvwqw", "#'^2hg9a&nm38*us");
	dimq_will_set(dimq, "will-topic", strlen("will message"), "will message", 2, false);

	rc = dimq_connect(dimq, "localhost", port, 60);

	while(run == -1){
		dimq_loop(dimq, -1, 1);
	}
	dimq_destroy(dimq);

	dimq_lib_cleanup();
	return run;
}
