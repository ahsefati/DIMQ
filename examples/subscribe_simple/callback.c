#include <stdlib.h>
#include <stdio.h>
#include "dimq.h"

int on_message(struct dimq *dimq, void *userdata, const struct dimq_message *msg)
{
	printf("%s %s (%d)\n", msg->topic, (const char *)msg->payload, msg->payloadlen);
	return 0;
}


int main(int argc, char *argv[])
{
	int rc;

	dimq_lib_init();

	rc = dimq_subscribe_callback(
			on_message, NULL,
			"irc/#", 0,
			"test.dimq.org", 1883,
			NULL, 60, true,
			NULL, NULL,
			NULL, NULL);

	if(rc){
		printf("Error: %s\n", dimq_strerror(rc));
	}

	dimq_lib_cleanup();

	return rc;
}

