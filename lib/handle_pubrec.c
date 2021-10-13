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

int handle__pubrec(struct dimq *dimq)
{
	uint8_t reason_code = 0;
	uint16_t mid;
	int rc;
	dimq_property *properties = NULL;

	assert(dimq);

	if(dimq__get_state(dimq) != dimq_cs_active){
		return dimq_ERR_PROTOCOL;
	}
	if(dimq->in_packet.command != CMD_PUBREC){
		return dimq_ERR_MALFORMED_PACKET;
	}

	rc = packet__read_uint16(&dimq->in_packet, &mid);
	if(rc) return rc;
	if(mid == 0) return dimq_ERR_PROTOCOL;

	if(dimq->protocol == dimq_p_mqtt5 && dimq->in_packet.remaining_length > 2){
		rc = packet__read_byte(&dimq->in_packet, &reason_code);
		if(rc) return rc;

		if(reason_code != MQTT_RC_SUCCESS
				&& reason_code != MQTT_RC_NO_MATCHING_SUBSCRIBERS
				&& reason_code != MQTT_RC_UNSPECIFIED
				&& reason_code != MQTT_RC_IMPLEMENTATION_SPECIFIC
				&& reason_code != MQTT_RC_NOT_AUTHORIZED
				&& reason_code != MQTT_RC_TOPIC_NAME_INVALID
				&& reason_code != MQTT_RC_PACKET_ID_IN_USE
				&& reason_code != MQTT_RC_QUOTA_EXCEEDED){

			return dimq_ERR_PROTOCOL;
		}

		if(dimq->in_packet.remaining_length > 3){
			rc = property__read_all(CMD_PUBREC, &dimq->in_packet, &properties);
			if(rc) return rc;

			/* Immediately free, we don't do anything with Reason String or User Property at the moment */
			dimq_property_free_all(&properties);
		}
	}

	if(dimq->in_packet.pos < dimq->in_packet.remaining_length){
#ifdef WITH_BROKER
		dimq_property_free_all(&properties);
#endif
		return dimq_ERR_MALFORMED_PACKET;
	}

#ifdef WITH_BROKER
	log__printf(NULL, dimq_LOG_DEBUG, "Received PUBREC from %s (Mid: %d)", dimq->id, mid);

	if(reason_code < 0x80){
		rc = db__message_update_outgoing(dimq, mid, dimq_ms_wait_for_pubcomp, 2);
	}else{
		return db__message_delete_outgoing(dimq, mid, dimq_ms_wait_for_pubrec, 2);
	}
#else

	log__printf(dimq, dimq_LOG_DEBUG, "Client %s received PUBREC (Mid: %d)", dimq->id, mid);

	if(reason_code < 0x80 || dimq->protocol != dimq_p_mqtt5){
		rc = message__out_update(dimq, mid, dimq_ms_wait_for_pubcomp, 2);
	}else{
		if(!message__delete(dimq, mid, dimq_md_out, 2)){
			/* Only inform the client the message has been sent once. */
			pthread_mutex_lock(&dimq->callback_mutex);
			if(dimq->on_publish_v5){
				dimq->in_callback = true;
				dimq->on_publish_v5(dimq, dimq->userdata, mid, reason_code, properties);
				dimq->in_callback = false;
			}
			pthread_mutex_unlock(&dimq->callback_mutex);
		}
		util__increment_send_quota(dimq);
		pthread_mutex_lock(&dimq->msgs_out.mutex);
		message__release_to_inflight(dimq, dimq_md_out);
		pthread_mutex_unlock(&dimq->msgs_out.mutex);
		return dimq_ERR_SUCCESS;
	}
#endif
	if(rc == dimq_ERR_NOT_FOUND){
		log__printf(dimq, dimq_LOG_WARNING, "Warning: Received PUBREC from %s for an unknown packet identifier %d.", dimq->id, mid);
	}else if(rc != dimq_ERR_SUCCESS){
		return rc;
	}
	rc = send__pubrel(dimq, mid, NULL);
	if(rc) return rc;

	return dimq_ERR_SUCCESS;
}

