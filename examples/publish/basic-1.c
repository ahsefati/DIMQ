/*
 * This example shows how to publish messages from outside of the dimq network loop.
 */

#include <dimq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/* Callback called when the client receives a CONNACK message from the broker. */
void on_connect(struct dimq *dimq, void *obj, int reason_code)
{
	/* Print out the connection result. dimq_connack_string() produces an
	 * appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
	 * clients is dimq_reason_string().
	 */
	printf("on_connect: %s\n", dimq_connack_string(reason_code));
	if(reason_code != 0){
		/* If the connection fails for any reason, we don't want to keep on
		 * retrying in this example, so disconnect. Without this, the client
		 * will attempt to reconnect. */
		dimq_disconnect(dimq);
	}

	/* You may wish to set a flag here to indicate to your application that the
	 * client is now connected. */
}


/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
void on_publish(struct dimq *dimq, void *obj, int mid)
{
	printf("Message with mid %d has been published.\n", mid);
}


int get_temperature(void)
{
	sleep(1); /* Prevent a storm of messages - this pretend sensor works at 1Hz */
	return random()%100;
}

/* This function pretends to read some data from a sensor and publish it.*/
void publish_sensor_data(struct dimq *dimq)
{
	char payload[20];
	int temp;
	int rc;

	/* Get our pretend data */
	temp = get_temperature();
	/* Print it to a string for easy human reading - payload format is highly
	 * application dependent. */
	snprintf(payload, sizeof(payload), "%d", temp);

	/* Publish the message
	 * dimq - our client instance
	 * *mid = NULL - we don't want to know what the message id for this message is
	 * topic = "example/temperature" - the topic on which this message will be published
	 * payloadlen = strlen(payload) - the length of our payload in bytes
	 * payload - the actual payload
	 * qos = 2 - publish with QoS 2 for this example
	 * retain = false - do not use the retained message feature for this message
	 */
	rc = dimq_publish(dimq, NULL, "example/temperature", strlen(payload), payload, 2, false);
	if(rc != dimq_ERR_SUCCESS){
		fprintf(stderr, "Error publishing: %s\n", dimq_strerror(rc));
	}
}


int main(int argc, char *argv[])
{
	struct dimq *dimq;
	int rc;

	/* Required before calling other dimq functions */
	dimq_lib_init();

	/* Create a new client instance.
	 * id = NULL -> ask the broker to generate a client id for us
	 * clean session = true -> the broker should remove old sessions when we connect
	 * obj = NULL -> we aren't passing any of our private data for callbacks
	 */
	dimq = dimq_new(NULL, true, NULL);
	if(dimq == NULL){
		fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}

	/* Configure callbacks. This should be done before connecting ideally. */
	dimq_connect_callback_set(dimq, on_connect);
	dimq_publish_callback_set(dimq, on_publish);

	/* Connect to test.dimq.org on port 1883, with a keepalive of 60 seconds.
	 * This call makes the socket connection only, it does not complete the MQTT
	 * CONNECT/CONNACK flow, you should use dimq_loop_start() or
	 * dimq_loop_forever() for processing net traffic. */
	rc = dimq_connect(dimq, "test.dimq.org", 1883, 60);
	if(rc != dimq_ERR_SUCCESS){
		dimq_destroy(dimq);
		fprintf(stderr, "Error: %s\n", dimq_strerror(rc));
		return 1;
	}

	/* Run the network loop in a background thread, this call returns quickly. */
	rc = dimq_loop_start(dimq);
	if(rc != dimq_ERR_SUCCESS){
		dimq_destroy(dimq);
		fprintf(stderr, "Error: %s\n", dimq_strerror(rc));
		return 1;
	}

	/* At this point the client is connected to the network socket, but may not
	 * have completed CONNECT/CONNACK.
	 * It is fairly safe to start queuing messages at this point, but if you
	 * want to be really sure you should wait until after a successful call to
	 * the connect callback.
	 * In this case we know it is 1 second before we start publishing.
	 */
	while(1){
		publish_sensor_data(dimq);
	}

	dimq_lib_cleanup();
	return 0;
}

