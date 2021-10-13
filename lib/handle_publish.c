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

#include "dimq.h"
#include "dimq_internal.h"
#include "logging_dimq.h"
#include "memory_dimq.h"
#include "mqtt_protocol.h"
#include "messages_dimq.h"
#include "packet_dimq.h"
#include "property_dimq.h"
#include "read_handle.h"
#include "send_dimq.h"
#include "time_dimq.h"
#include "util_dimq.h"


int handle__publish(struct dimq *dimq)
{
	uint8_t header;
	struct dimq_message_all *message;
	int rc = 0;
	uint16_t mid = 0;
	uint16_t slen;
	dimq_property *properties = NULL;

	assert(dimq);

	if(dimq__get_state(dimq) != dimq_cs_active){
		return dimq_ERR_PROTOCOL;
	}

	message = dimq__calloc(1, sizeof(struct dimq_message_all));
	if(!message) return dimq_ERR_NOMEM;

	header = dimq->in_packet.command;

	message->dup = (header & 0x08)>>3;
	message->msg.qos = (header & 0x06)>>1;
	message->msg.retain = (header & 0x01);

	rc = packet__read_string(&dimq->in_packet, &message->msg.topic, &slen);
	if(rc){
		message__cleanup(&message);
		return rc;
	}
	if(!slen){
		message__cleanup(&message);
		return dimq_ERR_PROTOCOL;
	}

	if(message->msg.qos > 0){
		if(dimq->protocol == dimq_p_mqtt5){
			if(dimq->msgs_in.inflight_quota == 0){
				message__cleanup(&message);
				/* FIXME - should send a DISCONNECT here */
				return dimq_ERR_PROTOCOL;
			}
		}

		rc = packet__read_uint16(&dimq->in_packet, &mid);
		if(rc){
			message__cleanup(&message);
			return rc;
		}
		if(mid == 0){
			message__cleanup(&message);
			return dimq_ERR_PROTOCOL;
		}
		message->msg.mid = (int)mid;
	}

	if(dimq->protocol == dimq_p_mqtt5){
		rc = property__read_all(CMD_PUBLISH, &dimq->in_packet, &properties);
		if(rc){
			message__cleanup(&message);
			return rc;
		}
	}

	message->msg.payloadlen = (int)(dimq->in_packet.remaining_length - dimq->in_packet.pos);
	if(message->msg.payloadlen){
		message->msg.payload = dimq__calloc((size_t)message->msg.payloadlen+1, sizeof(uint8_t));
		if(!message->msg.payload){
			message__cleanup(&message);
			dimq_property_free_all(&properties);
			return dimq_ERR_NOMEM;
		}
		rc = packet__read_bytes(&dimq->in_packet, message->msg.payload, (uint32_t)message->msg.payloadlen);
		if(rc){
			message__cleanup(&message);
			dimq_property_free_all(&properties);
			return rc;
		}
	}
	log__printf(dimq, dimq_LOG_DEBUG,
			"Client %s received PUBLISH (d%d, q%d, r%d, m%d, '%s', ... (%ld bytes))",
			dimq->id, message->dup, message->msg.qos, message->msg.retain,
			message->msg.mid, message->msg.topic,
			(long)message->msg.payloadlen);

	message->timestamp = dimq_time();
	switch(message->msg.qos){
		case 0:
			pthread_mutex_lock(&dimq->callback_mutex);
			if(dimq->on_message){
				dimq->in_callback = true;
				dimq->on_message(dimq, dimq->userdata, &message->msg);
				dimq->in_callback = false;
			}
			if(dimq->on_message_v5){
				dimq->in_callback = true;
				dimq->on_message_v5(dimq, dimq->userdata, &message->msg, properties);
				dimq->in_callback = false;
			}
			pthread_mutex_unlock(&dimq->callback_mutex);
			message__cleanup(&message);
			dimq_property_free_all(&properties);
			return dimq_ERR_SUCCESS;
		case 1:
			util__decrement_receive_quota(dimq);
			rc = send__puback(dimq, mid, 0, NULL);
			pthread_mutex_lock(&dimq->callback_mutex);
			if(dimq->on_message){
				dimq->in_callback = true;
				dimq->on_message(dimq, dimq->userdata, &message->msg);
				dimq->in_callback = false;
			}
			if(dimq->on_message_v5){
				dimq->in_callback = true;
				dimq->on_message_v5(dimq, dimq->userdata, &message->msg, properties);
				dimq->in_callback = false;
			}
			pthread_mutex_unlock(&dimq->callback_mutex);
			message__cleanup(&message);
			dimq_property_free_all(&properties);
			return rc;
		case 2:
			message->properties = properties;
			util__decrement_receive_quota(dimq);
			rc = send__pubrec(dimq, mid, 0, NULL);
			pthread_mutex_lock(&dimq->msgs_in.mutex);
			message->state = dimq_ms_wait_for_pubrel;
			message__queue(dimq, message, dimq_md_in);
			pthread_mutex_unlock(&dimq->msgs_in.mutex);
			return rc;
		default:
			message__cleanup(&message);
			dimq_property_free_all(&properties);
			return dimq_ERR_PROTOCOL;
	}
}

