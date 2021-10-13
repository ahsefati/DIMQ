/*
Copyright (c) 2020 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License 2.0
and Eclipse Distribution License v1.0 which accompany this distribution.

The Eclipse Public License is available at
   https://www.eclipse.org/legal/epl-2.0/
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.

SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

Contributors:
   Roger Light - initial implementation and documentation.
*/

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <dimq.h>
#include <mqtt_protocol.h>
#include "dimq_ctrl.h"

static int run = 1;

static void on_message(struct dimq *dimq, void *obj, const struct dimq_message *msg, const dimq_property *properties)
{
	struct dimq_ctrl *ctrl = obj;

	UNUSED(properties);

	if(ctrl->payload_callback){
		ctrl->payload_callback(ctrl, msg->payloadlen, msg->payload);
	}

	dimq_disconnect_v5(dimq, 0, NULL);
	run = 0;
}


static void on_publish(struct dimq *dimq, void *obj, int mid, int reason_code, const dimq_property *properties)
{
	UNUSED(obj);
	UNUSED(mid);
	UNUSED(properties);

	if(reason_code > 127){
		fprintf(stderr, "Publish error: %s\n", dimq_reason_string(reason_code));
		run = 0;
		dimq_disconnect_v5(dimq, 0, NULL);
	}
}


static void on_subscribe(struct dimq *dimq, void *obj, int mid, int qos_count, const int *granted_qos, const dimq_property *properties)
{
	struct dimq_ctrl *ctrl = obj;

	UNUSED(mid);
	UNUSED(properties);

	if(qos_count == 1){
		if(granted_qos[0] < 128){
			/* Success */
			dimq_publish(dimq, NULL, ctrl->request_topic, (int)strlen(ctrl->payload), ctrl->payload, ctrl->cfg.qos, 0);
			free(ctrl->request_topic);
			ctrl->request_topic = NULL;
			free(ctrl->payload);
			ctrl->payload = NULL;
		}else{
			if(ctrl->cfg.protocol_version == MQTT_PROTOCOL_V5){
				fprintf(stderr, "Subscribe error: %s\n", dimq_reason_string(granted_qos[0]));
			}else{
				fprintf(stderr, "Subscribe error: Subscription refused.\n");
			}
			run = 0;
			dimq_disconnect_v5(dimq, 0, NULL);
		}
	}else{
		run = 0;
		dimq_disconnect_v5(dimq, 0, NULL);
	}
}


static void on_connect(struct dimq *dimq, void *obj, int reason_code, int flags, const dimq_property *properties)
{
	struct dimq_ctrl *ctrl = obj;

	UNUSED(flags);
	UNUSED(properties);

	if(reason_code == 0){
		if(ctrl->response_topic){
			dimq_subscribe(dimq, NULL, ctrl->response_topic, ctrl->cfg.qos);
			free(ctrl->response_topic);
			ctrl->response_topic = NULL;
		}
	}else{
		if(ctrl->cfg.protocol_version == MQTT_PROTOCOL_V5){
			if(reason_code == MQTT_RC_UNSUPPORTED_PROTOCOL_VERSION){
				fprintf(stderr, "Connection error: %s. Try connecting to an MQTT v5 broker, or use MQTT v3.x mode.\n", dimq_reason_string(reason_code));
			}else{
				fprintf(stderr, "Connection error: %s\n", dimq_reason_string(reason_code));
			}
		}else{
			fprintf(stderr, "Connection error: %s\n", dimq_connack_string(reason_code));
		}
		run = 0;
		dimq_disconnect_v5(dimq, 0, NULL);
	}
}


int client_request_response(struct dimq_ctrl *ctrl)
{
	struct dimq *dimq;
	int rc;
	time_t start;

	if(ctrl->cfg.cafile == NULL && ctrl->cfg.capath == NULL){
		fprintf(stderr, "Warning: You are running dimq_ctrl without encryption.\nThis means all of the configuration changes you are making are visible on the network, including passwords.\n\n");
	}

	dimq_lib_init();

	dimq = dimq_new(ctrl->cfg.id, true, ctrl);
	rc = client_opts_set(dimq, &ctrl->cfg);
	if(rc) goto cleanup;

	dimq_connect_v5_callback_set(dimq, on_connect);
	dimq_subscribe_v5_callback_set(dimq, on_subscribe);
	dimq_publish_v5_callback_set(dimq, on_publish);
	dimq_message_v5_callback_set(dimq, on_message);

	rc = client_connect(dimq, &ctrl->cfg);
	if(rc) goto cleanup;

	start = time(NULL);
	while(run && start+10 > time(NULL)){
		dimq_loop(dimq, -1, 1);
	}

cleanup:
	dimq_destroy(dimq);
	dimq_lib_cleanup();
	return rc;
}
