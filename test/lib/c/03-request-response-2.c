#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dimq.h>
#include <mqtt_protocol.h>

static int run = -1;
static int sent_mid = -1;

void on_connect(struct dimq *dimq, void *obj, int rc)
{
	if(rc){
		exit(1);
	}else{
		dimq_subscribe(dimq, NULL, "request/topic", 0);
	}
}

void on_message_v5(struct dimq *dimq, void *obj, const struct dimq_message *msg, const dimq_property *props)
{
	const dimq_property *p_resp, *p_corr = NULL;
	char *resp_topic = NULL;
	int rc;

	if(!strcmp(msg->topic, "request/topic")){
		p_resp = dimq_property_read_string(props, MQTT_PROP_RESPONSE_TOPIC, &resp_topic, false);
		if(p_resp){
			p_corr = dimq_property_read_binary(props, MQTT_PROP_CORRELATION_DATA, NULL, NULL, false);
			rc = dimq_publish_v5(dimq, NULL, resp_topic, strlen("a response"), "a response", 0, false, p_corr);
			free(resp_topic);
		}
	}
}

void on_publish(struct dimq *dimq, void *obj, int mid)
{
	run = 0;
}


int main(int argc, char *argv[])
{
	int rc;
	struct dimq *dimq;
	int ver = PROTOCOL_VERSION_v5;

	int port = atoi(argv[1]);

	dimq_lib_init();

	dimq = dimq_new("response-test", true, NULL);
	if(dimq == NULL){
		return 1;
	}
	dimq_opts_set(dimq, dimq_OPT_PROTOCOL_VERSION, &ver);
	dimq_connect_callback_set(dimq, on_connect);
	dimq_publish_callback_set(dimq, on_publish);
	dimq_message_v5_callback_set(dimq, on_message_v5);

	rc = dimq_connect(dimq, "localhost", port, 60);

	while(run == -1){
		rc = dimq_loop(dimq, -1, 1);
	}
	dimq_destroy(dimq);

	dimq_lib_cleanup();
	return run;
}
