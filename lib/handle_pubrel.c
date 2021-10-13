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


int handle__pubrel(struct dimq *dimq)
{
	uint8_t reason_code;
	uint16_t mid;
#ifndef WITH_BROKER
	struct dimq_message_all *message = NULL;
#endif
	int rc;
	dimq_property *properties = NULL;

	assert(dimq);

	if(dimq__get_state(dimq) != dimq_cs_active){
		return dimq_ERR_PROTOCOL;
	}
	if(dimq->protocol != dimq_p_mqtt31 && dimq->in_packet.command != (CMD_PUBREL|2)){
		return dimq_ERR_MALFORMED_PACKET;
	}

	if(dimq->protocol != dimq_p_mqtt31){
		if((dimq->in_packet.command&0x0F) != 0x02){
			return dimq_ERR_PROTOCOL;
		}
	}
	rc = packet__read_uint16(&dimq->in_packet, &mid);
	if(rc) return rc;
	if(mid == 0) return dimq_ERR_PROTOCOL;

	if(dimq->protocol == dimq_p_mqtt5 && dimq->in_packet.remaining_length > 2){
		rc = packet__read_byte(&dimq->in_packet, &reason_code);
		if(rc) return rc;

		if(reason_code != MQTT_RC_SUCCESS && reason_code != MQTT_RC_PACKET_ID_NOT_FOUND){
			return dimq_ERR_PROTOCOL;
		}

		if(dimq->in_packet.remaining_length > 3){
			rc = property__read_all(CMD_PUBREL, &dimq->in_packet, &properties);
			if(rc) return rc;
		}
	}

	if(dimq->in_packet.pos < dimq->in_packet.remaining_length){
#ifdef WITH_BROKER
		dimq_property_free_all(&properties);
#endif
		return dimq_ERR_MALFORMED_PACKET;
	}

#ifdef WITH_BROKER
	log__printf(NULL, dimq_LOG_DEBUG, "Received PUBREL from %s (Mid: %d)", dimq->id, mid);

	/* Immediately free, we don't do anything with Reason String or User Property at the moment */
	dimq_property_free_all(&properties);

	rc = db__message_release_incoming(dimq, mid);
	if(rc == dimq_ERR_NOT_FOUND){
		/* Message not found. Still send a PUBCOMP anyway because this could be
		 * due to a repeated PUBREL after a client has reconnected. */
	}else if(rc != dimq_ERR_SUCCESS){
		return rc;
	}

	rc = send__pubcomp(dimq, mid, NULL);
	if(rc) return rc;
#else
	log__printf(dimq, dimq_LOG_DEBUG, "Client %s received PUBREL (Mid: %d)", dimq->id, mid);

	rc = send__pubcomp(dimq, mid, NULL);
	if(rc){
		message__remove(dimq, mid, dimq_md_in, &message, 2);
		return rc;
	}

	rc = message__remove(dimq, mid, dimq_md_in, &message, 2);
	if(rc == dimq_ERR_SUCCESS){
		/* Only pass the message on if we have removed it from the queue - this
		 * prevents multiple callbacks for the same message. */
		pthread_mutex_lock(&dimq->callback_mutex);
		if(dimq->on_message){
			dimq->in_callback = true;
			dimq->on_message(dimq, dimq->userdata, &message->msg);
			dimq->in_callback = false;
		}
		if(dimq->on_message_v5){
			dimq->in_callback = true;
			dimq->on_message_v5(dimq, dimq->userdata, &message->msg, message->properties);
			dimq->in_callback = false;
		}
		pthread_mutex_unlock(&dimq->callback_mutex);
		dimq_property_free_all(&properties);
		message__cleanup(&message);
	}else if(rc == dimq_ERR_NOT_FOUND){
		return dimq_ERR_SUCCESS;
	}else{
		return rc;
	}
#endif

	return dimq_ERR_SUCCESS;
}

