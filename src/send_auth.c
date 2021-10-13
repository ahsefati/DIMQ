/*
Copyright (c) 2019-2020 Roger Light <roger@atchoo.org>

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

#include "dimq_broker_internal.h"
#include "mqtt_protocol.h"
#include "memory_dimq.h"
#include "packet_dimq.h"
#include "property_dimq.h"
#include "util_dimq.h"

int send__auth(struct dimq *context, uint8_t reason_code, const void *auth_data, uint16_t auth_data_len)
{
	struct dimq__packet *packet = NULL;
	int rc;
	dimq_property *properties = NULL;
	uint32_t remaining_length;

	if(context->auth_method == NULL) return dimq_ERR_INVAL;
	if(context->protocol != dimq_p_mqtt5) return dimq_ERR_PROTOCOL;

	log__printf(NULL, dimq_LOG_DEBUG, "Sending AUTH to %s (rc%d, %s)", context->id, reason_code, context->auth_method);

	remaining_length = 1;

	rc = dimq_property_add_string(&properties, MQTT_PROP_AUTHENTICATION_METHOD, context->auth_method);
	if(rc){
		dimq_property_free_all(&properties);
		return rc;
	}

	if(auth_data != NULL && auth_data_len > 0){
		rc = dimq_property_add_binary(&properties, MQTT_PROP_AUTHENTICATION_DATA, auth_data, auth_data_len);
		if(rc){
			dimq_property_free_all(&properties);
			return rc;
		}
	}

	remaining_length += property__get_remaining_length(properties);

	if(packet__check_oversize(context, remaining_length)){
		dimq_property_free_all(&properties);
		dimq__free(packet);
		return dimq_ERR_OVERSIZE_PACKET;
	}

	packet = dimq__calloc(1, sizeof(struct dimq__packet));
	if(!packet) return dimq_ERR_NOMEM;

	packet->command = CMD_AUTH;
	packet->remaining_length = remaining_length;

	rc = packet__alloc(packet);
	if(rc){
		dimq_property_free_all(&properties);
		dimq__free(packet);
		return rc;
	}
	packet__write_byte(packet, reason_code);
	property__write_all(packet, properties, true);
	dimq_property_free_all(&properties);

	return packet__queue(context, packet);
}

