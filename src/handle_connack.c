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

#include <stdio.h>
#include <string.h>

#include "dimq_broker_internal.h"
#include "memory_dimq.h"
#include "mqtt_protocol.h"
#include "packet_dimq.h"
#include "send_dimq.h"
#include "util_dimq.h"

int handle__connack(struct dimq *context)
{
	int rc;
	uint8_t connect_acknowledge;
	uint8_t reason_code;
	dimq_property *properties = NULL;
	uint32_t maximum_packet_size;
	uint8_t retain_available;
	uint16_t server_keepalive;
	uint8_t max_qos = 255;

	if(context == NULL){
		return dimq_ERR_INVAL;
	}
	if(context->bridge == NULL){
		return dimq_ERR_PROTOCOL;
	}
	if(context->in_packet.command != CMD_CONNACK){
		return dimq_ERR_MALFORMED_PACKET;
	}
	log__printf(NULL, dimq_LOG_DEBUG, "Received CONNACK on connection %s.", context->id);
	if(packet__read_byte(&context->in_packet, &connect_acknowledge)) return dimq_ERR_MALFORMED_PACKET;
	if(packet__read_byte(&context->in_packet, &reason_code)) return dimq_ERR_MALFORMED_PACKET;

	if(context->protocol == dimq_p_mqtt5){
		if(context->in_packet.remaining_length == 2 && reason_code == CONNACK_REFUSED_PROTOCOL_VERSION){
			/* We have connected to a MQTT v3.x broker that doesn't support MQTT v5.0
			 * It has correctly replied with a CONNACK code of a bad protocol version.
			 */
			log__printf(NULL, dimq_LOG_NOTICE,
					"Warning: Remote bridge %s does not support MQTT v5.0, reconnecting using MQTT v3.1.1.",
					context->bridge->name);

			context->protocol = dimq_p_mqtt311;
			context->bridge->protocol_version = dimq_p_mqtt311;
			return dimq_ERR_PROTOCOL;
		}

		rc = property__read_all(CMD_CONNACK, &context->in_packet, &properties);
		if(rc) return rc;

		/* maximum-qos */
		dimq_property_read_byte(properties, MQTT_PROP_MAXIMUM_QOS,
					&max_qos, false);

		/* maximum-packet-size */
		if(dimq_property_read_int32(properties, MQTT_PROP_MAXIMUM_PACKET_SIZE,
					&maximum_packet_size, false)){

			if(context->maximum_packet_size == 0 || context->maximum_packet_size > maximum_packet_size){
				context->maximum_packet_size = maximum_packet_size;
			}
		}

		/* receive-maximum */
		dimq_property_read_int16(properties, MQTT_PROP_RECEIVE_MAXIMUM,
				&context->msgs_out.inflight_maximum, false);
		context->msgs_out.inflight_quota = context->msgs_out.inflight_maximum;

		/* retain-available */
		if(dimq_property_read_byte(properties, MQTT_PROP_RETAIN_AVAILABLE,
					&retain_available, false)){

			/* Only use broker provided value if the local config is set to available==true */
			if(context->retain_available){
				context->retain_available = retain_available;
			}
		}

		/* server-keepalive */
		if(dimq_property_read_int16(properties, MQTT_PROP_SERVER_KEEP_ALIVE,
					&server_keepalive, false)){

			context->keepalive = server_keepalive;
		}

		dimq_property_free_all(&properties);
	}
	dimq_property_free_all(&properties); /* FIXME - TEMPORARY UNTIL PROPERTIES PROCESSED */

	if(reason_code == MQTT_RC_SUCCESS){
#ifdef WITH_BRIDGE
		if(context->bridge){
			rc = bridge__on_connect(context);
			if(rc) return rc;
		}
#endif
		if(max_qos != 255){
			context->max_qos = max_qos;
		}
		dimq__set_state(context, dimq_cs_active);
		rc = db__message_write_queued_out(context);
		if(rc) return rc;
		rc = db__message_write_inflight_out_all(context);
		return rc;
	}else{
		if(context->protocol == dimq_p_mqtt5){
			switch(reason_code){
				case MQTT_RC_RETAIN_NOT_SUPPORTED:
					context->retain_available = 0;
					log__printf(NULL, dimq_LOG_ERR, "Connection Refused: retain not available (will retry)");
					return dimq_ERR_CONN_LOST;
				case MQTT_RC_QOS_NOT_SUPPORTED:
					if(max_qos == 255){
						if(context->max_qos != 0){
							context->max_qos--;
						}
					}else{
						context->max_qos = max_qos;
					}
					log__printf(NULL, dimq_LOG_ERR, "Connection Refused: QoS not supported (will retry)");
					return dimq_ERR_CONN_LOST;
				default:
					log__printf(NULL, dimq_LOG_ERR, "Connection Refused: %s", dimq_reason_string(reason_code));
					return dimq_ERR_CONN_LOST;
			}
		}else{
			switch(reason_code){
				case CONNACK_REFUSED_PROTOCOL_VERSION:
					if(context->bridge){
						context->bridge->try_private_accepted = false;
					}
					log__printf(NULL, dimq_LOG_ERR, "Connection Refused: unacceptable protocol version");
					return dimq_ERR_CONN_LOST;
				case CONNACK_REFUSED_IDENTIFIER_REJECTED:
					log__printf(NULL, dimq_LOG_ERR, "Connection Refused: identifier rejected");
					return dimq_ERR_CONN_LOST;
				case CONNACK_REFUSED_SERVER_UNAVAILABLE:
					log__printf(NULL, dimq_LOG_ERR, "Connection Refused: broker unavailable");
					return dimq_ERR_CONN_LOST;
				case CONNACK_REFUSED_BAD_USERNAME_PASSWORD:
					log__printf(NULL, dimq_LOG_ERR, "Connection Refused: broker unavailable");
					return dimq_ERR_CONN_LOST;
				case CONNACK_REFUSED_NOT_AUTHORIZED:
					log__printf(NULL, dimq_LOG_ERR, "Connection Refused: not authorised");
					return dimq_ERR_CONN_LOST;
				default:
					log__printf(NULL, dimq_LOG_ERR, "Connection Refused: unknown reason");
					return dimq_ERR_CONN_LOST;
			}
		}
	}
	return dimq_ERR_CONN_LOST;
}

