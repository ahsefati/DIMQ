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
#include <string.h>

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
#include "util_dimq.h"


int send__subscribe(struct dimq *dimq, int *mid, int topic_count, char *const *const topic, int topic_qos, const dimq_property *properties)
{
	struct dimq__packet *packet = NULL;
	uint32_t packetlen;
	uint16_t local_mid;
	int rc;
	int i;
	size_t tlen;

	assert(dimq);
	assert(topic);

	packetlen = 2;
	if(dimq->protocol == dimq_p_mqtt5){
		packetlen += property__get_remaining_length(properties);
	}
	for(i=0; i<topic_count; i++){
		tlen = strlen(topic[i]);
		if(tlen > UINT16_MAX){
			return dimq_ERR_INVAL;
		}
		packetlen += 2U+(uint16_t)tlen + 1U;
	}

	packet = dimq__calloc(1, sizeof(struct dimq__packet));
	if(!packet) return dimq_ERR_NOMEM;


	packet->command = CMD_SUBSCRIBE | (1<<1);
	packet->remaining_length = packetlen;
	rc = packet__alloc(packet);
	if(rc){
		dimq__free(packet);
		return rc;
	}

	/* Variable header */
	local_mid = dimq__mid_generate(dimq);
	if(mid) *mid = (int)local_mid;
	packet__write_uint16(packet, local_mid);

	if(dimq->protocol == dimq_p_mqtt5){
		property__write_all(packet, properties, true);
	}

	/* Payload */
	for(i=0; i<topic_count; i++){
		packet__write_string(packet, topic[i], (uint16_t)strlen(topic[i]));
		packet__write_byte(packet, (uint8_t)topic_qos);
	}

#ifdef WITH_BROKER
# ifdef WITH_BRIDGE
	log__printf(dimq, dimq_LOG_DEBUG, "Bridge %s sending SUBSCRIBE (Mid: %d, Topic: %s, QoS: %d, Options: 0x%02x)", dimq->id, local_mid, topic[0], topic_qos&0x03, topic_qos&0xFC);
# endif
#else
	for(i=0; i<topic_count; i++){
		log__printf(dimq, dimq_LOG_DEBUG, "Client %s sending SUBSCRIBE (Mid: %d, Topic: %s, QoS: %d, Options: 0x%02x)", dimq->id, local_mid, topic[i], topic_qos&0x03, topic_qos&0xFC);
	}
#endif

	return packet__queue(dimq, packet);
}

