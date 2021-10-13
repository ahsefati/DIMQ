#include <stdlib.h>
#include <stdio.h>
#include "dimq.h"

#define COUNT 3

int main(int argc, char *argv[])
{
	int rc;
	int i;
	struct dimq_message *msg;

	dimq_lib_init();

	rc = dimq_subscribe_simple(
			&msg, COUNT, true,
			"irc/#", 0,
			"test.dimq.org", 1883,
			NULL, 60, true,
			NULL, NULL,
			NULL, NULL);

	if(rc){
		printf("Error: %s\n", dimq_strerror(rc));
		dimq_lib_cleanup();
		return rc;
	}

	for(i=0; i<COUNT; i++){
		printf("%s %s\n", msg[i].topic, (char *)msg[i].payload);
		dimq_message_free_contents(&msg[i]);
	}
	free(msg);

	dimq_lib_cleanup();

	return 0;
}

