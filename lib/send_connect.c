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

#include "logging_dimq.h"
#include "memory_dimq.h"
#include "dimq.h"
#include "dimq_internal.h"
#include "mqtt_protocol.h"
#include "packet_dimq.h"
#include "property_dimq.h"
#include "send_dimq.h"

int send__connect(struct dimq *dimq, uint16_t keepalive, bool clean_session, const dimq_property *properties)
{
	struct dimq__packet *packet = NULL;
	uint32_t payloadlen;
	uint8_t will = 0;
	uint8_t byte;
	int rc;
	uint8_t version;
	char *clientid, *username, *password;
	uint32_t headerlen;
	uint32_t proplen = 0, varbytes;
	dimq_property *local_props = NULL;
	uint16_t receive_maximum;

	assert(dimq);

	if(dimq->protocol == dimq_p_mqtt31 && !dimq->id) return dimq_ERR_PROTOCOL;

#if defined(WITH_BROKER) && defined(WITH_BRIDGE)
	if(dimq->bridge){
		clientid = dimq->bridge->remote_clientid;
		username = dimq->bridge->remote_username;
		password = dimq->bridge->remote_password;
	}else{
		clientid = dimq->id;
		username = dimq->username;
		password = dimq->password;
	}
#else
	clientid = dimq->id;
	username = dimq->username;
	password = dimq->password;
#endif

	if(dimq->protocol == dimq_p_mqtt5){
		/* Generate properties from options */
		if(!dimq_property_read_int16(properties, MQTT_PROP_RECEIVE_MAXIMUM, &receive_maximum, false)){
			rc = dimq_property_add_int16(&local_props, MQTT_PROP_RECEIVE_MAXIMUM, dimq->msgs_in.inflight_maximum);
			if(rc) return rc;
		}else{
			dimq->msgs_in.inflight_maximum = receive_maximum;
			dimq->msgs_in.inflight_quota = receive_maximum;
		}

		version = MQTT_PROTOCOL_V5;
		headerlen = 10;
		proplen = 0;
		proplen += property__get_length_all(properties);
		proplen += property__get_length_all(local_props);
		varbytes = packet__varint_bytes(proplen);
		headerlen += proplen + varbytes;
	}else if(dimq->protocol == dimq_p_mqtt311){
		version = MQTT_PROTOCOL_V311;
		headerlen = 10;
	}else if(dimq->protocol == dimq_p_mqtt31){
		version = MQTT_PROTOCOL_V31;
		headerlen = 12;
	}else{
		return dimq_ERR_INVAL;
	}

	packet = dimq__calloc(1, sizeof(struct dimq__packet));
	if(!packet) return dimq_ERR_NOMEM;

	if(clientid){
		payloadlen = (uint32_t)(2U+strlen(clientid));
	}else{
		payloadlen = 2U;
	}
#ifdef WITH_BROKER
	if(dimq->will && (dimq->bridge == NULL || dimq->bridge->notifications_local_only == false)){
#else
	if(dimq->will){
#endif
		will = 1;
		assert(dimq->will->msg.topic);

		payloadlen += (uint32_t)(2+strlen(dimq->will->msg.topic) + 2+(uint32_t)dimq->will->msg.payloadlen);
		if(dimq->protocol == dimq_p_mqtt5){
			payloadlen += property__get_remaining_length(dimq->will->properties);
		}
	}

	/* After this check we can be sure that the username and password are
	 * always valid for the current protocol, so there is no need to check
	 * username before checking password. */
	if(dimq->protocol == dimq_p_mqtt31 || dimq->protocol == dimq_p_mqtt311){
		if(password != NULL && username == NULL){
			dimq__free(packet);
			return dimq_ERR_INVAL;
		}
	}

	if(username){
		payloadlen += (uint32_t)(2+strlen(username));
	}
	if(password){
		payloadlen += (uint32_t)(2+strlen(password));
	}

	packet->command = CMD_CONNECT;
	packet->remaining_length = headerlen + payloadlen;
	rc = packet__alloc(packet);
	if(rc){
		dimq__free(packet);
		return rc;
	}

	/* Variable header */
	if(version == MQTT_PROTOCOL_V31){
		packet__write_string(packet, PROTOCOL_NAME_v31, (uint16_t)strlen(PROTOCOL_NAME_v31));
	}else{
		packet__write_string(packet, PROTOCOL_NAME, (uint16_t)strlen(PROTOCOL_NAME));
	}
#if defined(WITH_BROKER) && defined(WITH_BRIDGE)
	if(dimq->bridge && dimq->bridge->protocol_version != dimq_p_mqtt5 && dimq->bridge->try_private && dimq->bridge->try_private_accepted){
		version |= 0x80;
	}else{
	}
#endif
	packet__write_byte(packet, version);
	byte = (uint8_t)((clean_session&0x1)<<1);
	if(will){
		byte = byte | (uint8_t)(((dimq->will->msg.qos&0x3)<<3) | ((will&0x1)<<2));
		if(dimq->retain_available){
			byte |= (uint8_t)((dimq->will->msg.retain&0x1)<<5);
		}
	}
	if(username){
		byte = byte | 0x1<<7;
	}
	if(dimq->password){
		byte = byte | 0x1<<6;
	}
	packet__write_byte(packet, byte);
	packet__write_uint16(packet, keepalive);

	if(dimq->protocol == dimq_p_mqtt5){
		/* Write properties */
		packet__write_varint(packet, proplen);
		property__write_all(packet, properties, false);
		property__write_all(packet, local_props, false);
	}
	dimq_property_free_all(&local_props);

	/* Payload */
	if(clientid){
		packet__write_string(packet, clientid, (uint16_t)strlen(clientid));
	}else{
		packet__write_uint16(packet, 0);
	}
	if(will){
		if(dimq->protocol == dimq_p_mqtt5){
			/* Write will properties */
			property__write_all(packet, dimq->will->properties, true);
		}
		packet__write_string(packet, dimq->will->msg.topic, (uint16_t)strlen(dimq->will->msg.topic));
		packet__write_string(packet, (const char *)dimq->will->msg.payload, (uint16_t)dimq->will->msg.payloadlen);
	}

	if(username){
		packet__write_string(packet, username, (uint16_t)strlen(username));
	}
	if(password){
		packet__write_string(packet, password, (uint16_t)strlen(password));
	}

	dimq->keepalive = keepalive;
#ifdef WITH_BROKER
# ifdef WITH_BRIDGE
	log__printf(dimq, dimq_LOG_DEBUG, "Bridge %s sending CONNECT", clientid);
# endif
#else
	log__printf(dimq, dimq_LOG_DEBUG, "Client %s sending CONNECT", clientid);
#endif
	return packet__queue(dimq, packet);
}

