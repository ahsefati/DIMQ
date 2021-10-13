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
	int rc2;
	dimq_property *proplist = NULL;

	if(rc){
		exit(1);
	}
}


void on_message_v5(struct dimq *dimq, void *obj, const struct dimq_message *msg, const dimq_property *properties)
{
	int rc;
	char *str;

	if(properties){
		if(dimq_property_read_string(properties, MQTT_PROP_CONTENT_TYPE, &str, false)){
			rc = strcmp(str, "plain/text");
			free(str);

			if(rc == 0){
				if(dimq_property_read_string(properties, MQTT_PROP_RESPONSE_TOPIC, &str, false)){
					rc = strcmp(str, "msg/123");
					free(str);

					if(rc == 0){
						if(msg->qos == 2){
							dimq_publish(dimq, NULL, "ok", 2, "ok", 0, 0);
							return;
						}
					}
				}
			}
		}
	}

	/* No matching message, so quit with an error */
	exit(1);
}


void on_publish(struct dimq *dimq, void *obj, int mid)
{
	run = 0;
}

int main(int argc, char *argv[])
{
	int rc;
	int tmp;
	struct dimq *dimq;

	int port = atoi(argv[1]);

	dimq_lib_init();

	dimq = dimq_new("prop-test", true, NULL);
	if(dimq == NULL){
		return 1;
	}
	dimq_connect_callback_set(dimq, on_connect);
	dimq_message_v5_callback_set(dimq, on_message_v5);
	dimq_int_option(dimq, dimq_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);

	rc = dimq_connect(dimq, "localhost", port, 60);

	while(run == -1){
		rc = dimq_loop(dimq, -1, 1);
	}

	dimq_destroy(dimq);
	dimq_lib_cleanup();
	return run;
}
