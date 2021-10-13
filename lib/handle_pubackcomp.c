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
#include <stdio.h>
#include <string.h>

#ifdef WITH_BROKER
#  include "dimq_broker_internal.h"
#endif

#include "dimq.h"
#include "logging_dimq.h"
#include "memory_dimq.h"
#include "messages_dimq.h"
#include "mqtt_protocol.h"
#include "net_dimq.h"
#include "packet_dimq.h"
#include "read_handle.h"
#include "send_dimq.h"
#include "util_dimq.h"


int handle__pubackcomp(struct dimq *dimq, const char *type)
{
	uint8_t reason_code = 0;
	uint16_t mid;
	int rc;
	dimq_property *properties = NULL;
	int qos;

	assert(dimq);

	if(dimq__get_state(dimq) != dimq_cs_active){
		return dimq_ERR_PROTOCOL;
	}
	if(dimq->protocol != dimq_p_mqtt31){
		if((dimq->in_packet.command&0x0F) != 0x00){
			return dimq_ERR_MALFORMED_PACKET;
		}
	}

	pthread_mutex_lock(&dimq->msgs_out.mutex);
	util__increment_send_quota(dimq);
	pthread_mutex_unlock(&dimq->msgs_out.mutex);

	rc = packet__read_uint16(&dimq->in_packet, &mid);
	if(rc) return rc;
	if(type[3] == 'A'){ /* pubAck or pubComp */
		if(dimq->in_packet.command != CMD_PUBACK){
			return dimq_ERR_MALFORMED_PACKET;
		}
		qos = 1;
	}else{
		if(dimq->in_packet.command != CMD_PUBCOMP){
			return dimq_ERR_MALFORMED_PACKET;
		}
		qos = 2;
	}
	if(mid == 0){
		return dimq_ERR_PROTOCOL;
	}

	if(dimq->protocol == dimq_p_mqtt5 && dimq->in_packet.remaining_length > 2){
		rc = packet__read_byte(&dimq->in_packet, &reason_code);
		if(rc){
			return rc;
		}

		if(dimq->in_packet.remaining_length > 3){
			rc = property__read_all(CMD_PUBACK, &dimq->in_packet, &properties);
			if(rc) return rc;
		}
		if(type[3] == 'A'){ /* pubAck or pubComp */
			if(reason_code != MQTT_RC_SUCCESS
					&& reason_code != MQTT_RC_NO_MATCHING_SUBSCRIBERS
					&& reason_code != MQTT_RC_UNSPECIFIED
					&& reason_code != MQTT_RC_IMPLEMENTATION_SPECIFIC
					&& reason_code != MQTT_RC_NOT_AUTHORIZED
					&& reason_code != MQTT_RC_TOPIC_NAME_INVALID
					&& reason_code != MQTT_RC_PACKET_ID_IN_USE
					&& reason_code != MQTT_RC_QUOTA_EXCEEDED
					&& reason_code != MQTT_RC_PAYLOAD_FORMAT_INVALID
					){

				return dimq_ERR_PROTOCOL;
			}
		}else{
			if(reason_code != MQTT_RC_SUCCESS
					&& reason_code != MQTT_RC_PACKET_ID_NOT_FOUND
					){

				return dimq_ERR_PROTOCOL;
			}
		}
	}
	if(dimq->in_packet.pos < dimq->in_packet.remaining_length){
#ifdef WITH_BROKER
		dimq_property_free_all(&properties);
#endif
		return dimq_ERR_MALFORMED_PACKET;
	}

#ifdef WITH_BROKER
	log__printf(NULL, dimq_LOG_DEBUG, "Received %s from %s (Mid: %d, RC:%d)", type, dimq->id, mid, reason_code);

	/* Immediately free, we don't do anything with Reason String or User Property at the moment */
	dimq_property_free_all(&properties);

	rc = db__message_delete_outgoing(dimq, mid, dimq_ms_wait_for_pubcomp, qos);
	if(rc == dimq_ERR_NOT_FOUND){
		log__printf(dimq, dimq_LOG_WARNING, "Warning: Received %s from %s for an unknown packet identifier %d.", type, dimq->id, mid);
		return dimq_ERR_SUCCESS;
	}else{
		return rc;
	}
#else
	log__printf(dimq, dimq_LOG_DEBUG, "Client %s received %s (Mid: %d, RC:%d)", dimq->id, type, mid, reason_code);

	rc = message__delete(dimq, mid, dimq_md_out, qos);
	if(rc == dimq_ERR_SUCCESS){
		/* Only inform the client the message has been sent once. */
		pthread_mutex_lock(&dimq->callback_mutex);
		if(dimq->on_publish){
			dimq->in_callback = true;
			dimq->on_publish(dimq, dimq->userdata, mid);
			dimq->in_callback = false;
		}
		if(dimq->on_publish_v5){
			dimq->in_callback = true;
			dimq->on_publish_v5(dimq, dimq->userdata, mid, reason_code, properties);
			dimq->in_callback = false;
		}
		pthread_mutex_unlock(&dimq->callback_mutex);
		dimq_property_free_all(&properties);
	}else if(rc != dimq_ERR_NOT_FOUND){
		return rc;
	}
	pthread_mutex_lock(&dimq->msgs_out.mutex);
	message__release_to_inflight(dimq, dimq_md_out);
	pthread_mutex_unlock(&dimq->msgs_out.mutex);

	return dimq_ERR_SUCCESS;
#endif
}

