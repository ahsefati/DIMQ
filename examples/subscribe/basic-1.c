/*
 * This example shows how to write a client that subscribes to a topic and does
 * not do anything other than handle the messages that are received.
 */

#include <dimq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/* Callback called when the client receives a CONNACK message from the broker. */
void on_connect(struct dimq *dimq, void *obj, int reason_code)
{
	int rc;
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

	/* Making subscriptions in the on_connect() callback means that if the
	 * connection drops and is automatically resumed by the client, then the
	 * subscriptions will be recreated when the client reconnects. */
	rc = dimq_subscribe(dimq, NULL, "example/temperature", 1);
	if(rc != dimq_ERR_SUCCESS){
		fprintf(stderr, "Error subscribing: %s\n", dimq_strerror(rc));
		/* We might as well disconnect if we were unable to subscribe */
		dimq_disconnect(dimq);
	}
}


/* Callback called when the broker sends a SUBACK in response to a SUBSCRIBE. */
void on_subscribe(struct dimq *dimq, void *obj, int mid, int qos_count, const int *granted_qos)
{
	int i;
	bool have_subscription = false;

	/* In this example we only subscribe to a single topic at once, but a
	 * SUBSCRIBE can contain many topics at once, so this is one way to check
	 * them all. */
	for(i=0; i<qos_count; i++){
		printf("on_subscribe: %d:granted qos = %d\n", i, granted_qos[i]);
		if(granted_qos[i] <= 2){
			have_subscription = true;
		}
	}
	if(have_subscription == false){
		/* The broker rejected all of our subscriptions, we know we only sent
		 * the one SUBSCRIBE, so there is no point remaining connected. */
		fprintf(stderr, "Error: All subscriptions rejected.\n");
		dimq_disconnect(dimq);
	}
}


/* Callback called when the client receives a message. */
void on_message(struct dimq *dimq, void *obj, const struct dimq_message *msg)
{
	/* This blindly prints the payload, but the payload can be anything so take care. */
	printf("%s %d %s\n", msg->topic, msg->qos, (char *)msg->payload);
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
	dimq_subscribe_callback_set(dimq, on_subscribe);
	dimq_message_callback_set(dimq, on_message);

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

	/* Run the network loop in a blocking call. The only thing we do in this
	 * example is to print incoming messages, so a blocking call here is fine.
	 *
	 * This call will continue forever, carrying automatic reconnections if
	 * necessary, until the user calls dimq_disconnect().
	 */
	dimq_loop_forever(dimq, -1, 1);

	dimq_lib_cleanup();
	return 0;
}

