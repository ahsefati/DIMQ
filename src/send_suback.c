/*
Copyright (c) 2009-2020 Roger Light <roger@atchoo.org>

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


int send__suback(struct dimq *context, uint16_t mid, uint32_t payloadlen, const void *payload)
{
	struct dimq__packet *packet = NULL;
	int rc;
	dimq_property *properties = NULL;

	log__printf(NULL, dimq_LOG_DEBUG, "Sending SUBACK to %s", context->id);

	packet = dimq__calloc(1, sizeof(struct dimq__packet));
	if(!packet) return dimq_ERR_NOMEM;

	packet->command = CMD_SUBACK;
	packet->remaining_length = 2+payloadlen;
	if(context->protocol == dimq_p_mqtt5){
		packet->remaining_length += property__get_remaining_length(properties);
	}
	rc = packet__alloc(packet);
	if(rc){
		dimq__free(packet);
		return rc;
	}
	packet__write_uint16(packet, mid);

	if(context->protocol == dimq_p_mqtt5){
		/* We don't use Reason String or User Property yet. */
		property__write_all(packet, properties, true);
	}

	if(payloadlen){
		packet__write_bytes(packet, payload, payloadlen);
	}

	return packet__queue(context, packet);
}
