#include <stdlib.h>
#include <stdio.h>
#include "dimq.h"

int main(int argc, char *argv[])
{
	int rc;
	struct dimq_message *msg;

	dimq_lib_init();

	rc = dimq_subscribe_simple(
			&msg, 1, true,
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

	printf("%s %s\n", msg->topic, (char *)msg->payload);
	dimq_message_free(&msg);

	dimq_lib_cleanup();

	return 0;
}

