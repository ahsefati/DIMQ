/*
Copyright (c) 2010-2020 Roger Light <roger@atchoo.org>

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

#include <stdio.h>
#include <string.h>

#ifdef WITH_BROKER
#  include "dimq_broker_internal.h"
#endif

#include "dimq.h"
#include "dimq_internal.h"
#include "logging_dimq.h"
#include "messages_dimq.h"
#include "memory_dimq.h"
#include "mqtt_protocol.h"
#include "net_dimq.h"
#include "read_handle.h"
#include "send_dimq.h"
#include "util_dimq.h"
#include "will_dimq.h"

int will__set(struct dimq *dimq, const char *topic, int payloadlen, const void *payload, int qos, bool retain, dimq_property *properties)
{
	int rc = dimq_ERR_SUCCESS;
	dimq_property *p;

	if(!dimq || !topic) return dimq_ERR_INVAL;
	if(payloadlen < 0 || payloadlen > (int)MQTT_MAX_PAYLOAD) return dimq_ERR_PAYLOAD_SIZE;
	if(payloadlen > 0 && !payload) return dimq_ERR_INVAL;

	if(dimq_pub_topic_check(topic)) return dimq_ERR_INVAL;
	if(dimq_validate_utf8(topic, (uint16_t)strlen(topic))) return dimq_ERR_MALFORMED_UTF8;

	if(properties){
		if(dimq->protocol != dimq_p_mqtt5){
			return dimq_ERR_NOT_SUPPORTED;
		}
		p = properties;
		while(p){
			rc = dimq_property_check_command(CMD_WILL, p->identifier);
			if(rc) return rc;
			p = p->next;
		}
	}

	if(dimq->will){
		dimq__free(dimq->will->msg.topic);
		dimq__free(dimq->will->msg.payload);
		dimq_property_free_all(&dimq->will->properties);
		dimq__free(dimq->will);
	}

	dimq->will = dimq__calloc(1, sizeof(struct dimq_message_all));
	if(!dimq->will) return dimq_ERR_NOMEM;
	dimq->will->msg.topic = dimq__strdup(topic);
	if(!dimq->will->msg.topic){
		rc = dimq_ERR_NOMEM;
		goto cleanup;
	}
	dimq->will->msg.payloadlen = payloadlen;
	if(dimq->will->msg.payloadlen > 0){
		if(!payload){
			rc = dimq_ERR_INVAL;
			goto cleanup;
		}
		dimq->will->msg.payload = dimq__malloc(sizeof(char)*(unsigned int)dimq->will->msg.payloadlen);
		if(!dimq->will->msg.payload){
			rc = dimq_ERR_NOMEM;
			goto cleanup;
		}

		memcpy(dimq->will->msg.payload, payload, (unsigned int)payloadlen);
	}
	dimq->will->msg.qos = qos;
	dimq->will->msg.retain = retain;

	dimq->will->properties = properties;

	return dimq_ERR_SUCCESS;

cleanup:
	if(dimq->will){
		dimq__free(dimq->will->msg.topic);
		dimq__free(dimq->will->msg.payload);

		dimq__free(dimq->will);
		dimq->will = NULL;
	}

	return rc;
}

int will__clear(struct dimq *dimq)
{
	if(!dimq->will) return dimq_ERR_SUCCESS;

	dimq__free(dimq->will->msg.topic);
	dimq->will->msg.topic = NULL;

	dimq__free(dimq->will->msg.payload);
	dimq->will->msg.payload = NULL;

	dimq_property_free_all(&dimq->will->properties);

	dimq__free(dimq->will);
	dimq->will = NULL;
	dimq->will_delay_interval = 0;

	return dimq_ERR_SUCCESS;
}

