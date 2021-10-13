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

#include "dimq.h"
#include "logging_dimq.h"
#include "memory_dimq.h"
#include "messages_dimq.h"
#include "mqtt_protocol.h"
#include "net_dimq.h"
#include "packet_dimq.h"
#include "property_dimq.h"
#include "read_handle.h"

static void connack_callback(struct dimq *dimq, uint8_t reason_code, uint8_t connect_flags, const dimq_property *properties)
{
	log__printf(dimq, dimq_LOG_DEBUG, "Client %s received CONNACK (%d)", dimq->id, reason_code);
	if(reason_code == MQTT_RC_SUCCESS){
		dimq->reconnects = 0;
	}
	pthread_mutex_lock(&dimq->callback_mutex);
	if(dimq->on_connect){
		dimq->in_callback = true;
		dimq->on_connect(dimq, dimq->userdata, reason_code);
		dimq->in_callback = false;
	}
	if(dimq->on_connect_with_flags){
		dimq->in_callback = true;
		dimq->on_connect_with_flags(dimq, dimq->userdata, reason_code, connect_flags);
		dimq->in_callback = false;
	}
	if(dimq->on_connect_v5){
		dimq->in_callback = true;
		dimq->on_connect_v5(dimq, dimq->userdata, reason_code, connect_flags, properties);
		dimq->in_callback = false;
	}
	pthread_mutex_unlock(&dimq->callback_mutex);
}


int handle__connack(struct dimq *dimq)
{
	uint8_t connect_flags;
	uint8_t reason_code;
	int rc;
	dimq_property *properties = NULL;
	char *clientid = NULL;

	assert(dimq);
	if(dimq->in_packet.command != CMD_CONNACK){
		return dimq_ERR_MALFORMED_PACKET;
	}

	rc = packet__read_byte(&dimq->in_packet, &connect_flags);
	if(rc) return rc;
	rc = packet__read_byte(&dimq->in_packet, &reason_code);
	if(rc) return rc;

	if(dimq->protocol == dimq_p_mqtt5){
		rc = property__read_all(CMD_CONNACK, &dimq->in_packet, &properties);

		if(rc == dimq_ERR_PROTOCOL && reason_code == CONNACK_REFUSED_PROTOCOL_VERSION){
			/* This could occur because we are connecting to a v3.x broker and
			 * it has replied with "unacceptable protocol version", but with a
			 * v3 CONNACK. */

			connack_callback(dimq, MQTT_RC_UNSUPPORTED_PROTOCOL_VERSION, connect_flags, NULL);
			return rc;
		}else if(rc){
			return rc;
		}
	}

	dimq_property_read_string(properties, MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER, &clientid, false);
	if(clientid){
		if(dimq->id){
			/* We've been sent a client identifier but already have one. This
			 * shouldn't happen. */
			free(clientid);
			dimq_property_free_all(&properties);
			return dimq_ERR_PROTOCOL;
		}else{
			dimq->id = clientid;
			clientid = NULL;
		}
	}

	dimq_property_read_byte(properties, MQTT_PROP_RETAIN_AVAILABLE, &dimq->retain_available, false);
	dimq_property_read_byte(properties, MQTT_PROP_MAXIMUM_QOS, &dimq->max_qos, false);
	dimq_property_read_int16(properties, MQTT_PROP_RECEIVE_MAXIMUM, &dimq->msgs_out.inflight_maximum, false);
	dimq_property_read_int16(properties, MQTT_PROP_SERVER_KEEP_ALIVE, &dimq->keepalive, false);
	dimq_property_read_int32(properties, MQTT_PROP_MAXIMUM_PACKET_SIZE, &dimq->maximum_packet_size, false);

	dimq->msgs_out.inflight_quota = dimq->msgs_out.inflight_maximum;
	message__reconnect_reset(dimq, true);

	connack_callback(dimq, reason_code, connect_flags, properties);
	dimq_property_free_all(&properties);

	switch(reason_code){
		case 0:
			pthread_mutex_lock(&dimq->state_mutex);
			if(dimq->state != dimq_cs_disconnecting){
				dimq->state = dimq_cs_active;
			}
			pthread_mutex_unlock(&dimq->state_mutex);
			message__retry_check(dimq);
			return dimq_ERR_SUCCESS;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			return dimq_ERR_CONN_REFUSED;
		default:
			return dimq_ERR_PROTOCOL;
	}
}

