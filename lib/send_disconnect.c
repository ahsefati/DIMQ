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

#include <assert.h>

#ifdef WITH_BROKER
#  include "dimq_broker_internal.h"
#endif

#include "dimq.h"
#include "dimq_internal.h"
#include "logging_dimq.h"
#include "memory_dimq.h"
#include "mqtt_protocol.h"
#include "packet_dimq.h"
#include "property_dimq.h"
#include "send_dimq.h"


int send__disconnect(struct dimq *dimq, uint8_t reason_code, const dimq_property *properties)
{
	struct dimq__packet *packet = NULL;
	int rc;

	assert(dimq);
#ifdef WITH_BROKER
#  ifdef WITH_BRIDGE
	if(dimq->bridge){
		log__printf(dimq, dimq_LOG_DEBUG, "Bridge %s sending DISCONNECT", dimq->id);
	}else
#  else
	{
		log__printf(dimq, dimq_LOG_DEBUG, "Sending DISCONNECT to %s (rc%d)", dimq->id, reason_code);
	}
#  endif
#else
	log__printf(dimq, dimq_LOG_DEBUG, "Client %s sending DISCONNECT", dimq->id);
#endif
	assert(dimq);
	packet = dimq__calloc(1, sizeof(struct dimq__packet));
	if(!packet) return dimq_ERR_NOMEM;

	packet->command = CMD_DISCONNECT;
	if(dimq->protocol == dimq_p_mqtt5 && (reason_code != 0 || properties)){
		packet->remaining_length = 1;
		if(properties){
			packet->remaining_length += property__get_remaining_length(properties);
		}
	}else{
		packet->remaining_length = 0;
	}

	rc = packet__alloc(packet);
	if(rc){
		dimq__free(packet);
		return rc;
	}
	if(dimq->protocol == dimq_p_mqtt5 && (reason_code != 0 || properties)){
		packet__write_byte(packet, reason_code);
		if(properties){
			property__write_all(packet, properties, true);
		}
	}

	return packet__queue(dimq, packet);
}

