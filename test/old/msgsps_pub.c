/* This provides a crude manner of testing the performance of a broker in messages/s. */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dimq.h>
#include <stdatomic.h>

#include <msgsps_common.h>

static atomic_int message_count = 0;

void my_publish_callback(struct dimq *dimq, void *obj, int mid)
{
	message_count++;
}

int main(int argc, char *argv[])
{
	struct dimq *dimq;
	int i;
	uint8_t buf[MESSAGE_SIZE];
	
	dimq_lib_init();

	dimq = dimq_new(NULL, true, NULL);
	dimq_publish_callback_set(dimq, my_publish_callback);
	dimq_connect(dimq, HOST, PORT, 600);
	dimq_loop_start(dimq);

	i=0;
	while(1){
		dimq_publish(dimq, NULL, "perf/test", sizeof(buf), buf, PUB_QOS, false);
		usleep(100);
		i++;
		if(i == 10000){
			/* Crude "messages per second" count */
			i = message_count;
			message_count = 0;
			printf("%d\n", i);
			i = 0;
		}
	}
	dimq_loop_stop(dimq, false);
	dimq_destroy(dimq);
	dimq_lib_cleanup();

	return 0;
}
