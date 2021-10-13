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
#include "read_handle.h"
#include "util_dimq.h"


int handle__suback(struct dimq *dimq)
{
	uint16_t mid;
	uint8_t qos;
	int *granted_qos;
	int qos_count;
	int i = 0;
	int rc;
	dimq_property *properties = NULL;

	assert(dimq);

	if(dimq__get_state(dimq) != dimq_cs_active){
		return dimq_ERR_PROTOCOL;
	}
	if(dimq->in_packet.command != CMD_SUBACK){
		return dimq_ERR_MALFORMED_PACKET;
	}

#ifdef WITH_BROKER
	if(dimq->bridge == NULL){
		/* Client is not a bridge, so shouldn't be sending SUBACK */
		return dimq_ERR_PROTOCOL;
	}
	log__printf(NULL, dimq_LOG_DEBUG, "Received SUBACK from %s", dimq->id);
#else
	log__printf(dimq, dimq_LOG_DEBUG, "Client %s received SUBACK", dimq->id);
#endif
	rc = packet__read_uint16(&dimq->in_packet, &mid);
	if(rc) return rc;
	if(mid == 0) return dimq_ERR_PROTOCOL;

	if(dimq->protocol == dimq_p_mqtt5){
		rc = property__read_all(CMD_SUBACK, &dimq->in_packet, &properties);
		if(rc) return rc;
	}

	qos_count = (int)(dimq->in_packet.remaining_length - dimq->in_packet.pos);
	granted_qos = dimq__malloc((size_t)qos_count*sizeof(int));
	if(!granted_qos){
#ifdef WITH_BROKER
		dimq_property_free_all(&properties);
#endif
		return dimq_ERR_NOMEM;
	}
	while(dimq->in_packet.pos < dimq->in_packet.remaining_length){
		rc = packet__read_byte(&dimq->in_packet, &qos);
		if(rc){
			dimq__free(granted_qos);
#ifdef WITH_BROKER
			dimq_property_free_all(&properties);
#endif
			return rc;
		}
		granted_qos[i] = (int)qos;
		i++;
	}
#ifdef WITH_BROKER
	/* Immediately free, we don't do anything with Reason String or User Property at the moment */
	dimq_property_free_all(&properties);
#else
	pthread_mutex_lock(&dimq->callback_mutex);
	if(dimq->on_subscribe){
		dimq->in_callback = true;
		dimq->on_subscribe(dimq, dimq->userdata, mid, qos_count, granted_qos);
		dimq->in_callback = false;
	}
	if(dimq->on_subscribe_v5){
		dimq->in_callback = true;
		dimq->on_subscribe_v5(dimq, dimq->userdata, mid, qos_count, granted_qos, properties);
		dimq->in_callback = false;
	}
	pthread_mutex_unlock(&dimq->callback_mutex);
	dimq_property_free_all(&properties);
#endif
	dimq__free(granted_qos);

	return dimq_ERR_SUCCESS;
}

