/* This provides a crude manner of testing the performance of a broker in messages/s. */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <dimq.h>
#include <stdatomic.h>

#include <msgsps_common.h>

static atomic_int message_count = 0;

void my_message_callback(struct dimq *dimq, void *obj, const struct dimq_message *msg)
{
	message_count++;
}

int main(int argc, char *argv[])
{
	struct dimq *dimq;
	int c;

	dimq_lib_init();

	dimq = dimq_new(NULL, true, NULL);
	dimq_message_callback_set(dimq, my_message_callback);

	dimq_connect(dimq, HOST, PORT, 600);
	dimq_subscribe(dimq, NULL, "perf/test", SUB_QOS);

	dimq_loop_start(dimq);
	while(1){
		sleep(1);
		c = message_count;
		message_count = 0;
		printf("%d\n", c);

	}

	dimq_destroy(dimq);
	dimq_lib_cleanup();

	return 0;
}
