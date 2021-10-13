/*
Copyright (c) 2010-2020 Roger Light <roger@atchoo.org>

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

#include <string.h>

#include "dimq.h"
#include "dimq_internal.h"
#include "memory_dimq.h"
#include "messages_dimq.h"
#include "mqtt_protocol.h"
#include "net_dimq.h"
#include "packet_dimq.h"
#include "send_dimq.h"
#include "util_dimq.h"


int dimq_publish(struct dimq *dimq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain)
{
	return dimq_publish_v5(dimq, mid, topic, payloadlen, payload, qos, retain, NULL);
}

int dimq_publish_v5(struct dimq *dimq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain, const dimq_property *properties)
{
	struct dimq_message_all *message;
	uint16_t local_mid;
	const dimq_property *p;
	const dimq_property *outgoing_properties = NULL;
	dimq_property *properties_copy = NULL;
	dimq_property local_property;
	bool have_topic_alias;
	int rc;
	size_t tlen = 0;
	uint32_t remaining_length;

	if(!dimq || qos<0 || qos>2) return dimq_ERR_INVAL;
	if(dimq->protocol != dimq_p_mqtt5 && properties) return dimq_ERR_NOT_SUPPORTED;
	if(qos > dimq->max_qos) return dimq_ERR_QOS_NOT_SUPPORTED;

	if(!dimq->retain_available){
		retain = false;
	}

	if(properties){
		if(properties->client_generated){
			outgoing_properties = properties;
		}else{
			memcpy(&local_property, properties, sizeof(dimq_property));
			local_property.client_generated = true;
			local_property.next = NULL;
			outgoing_properties = &local_property;
		}
		rc = dimq_property_check_all(CMD_PUBLISH, outgoing_properties);
		if(rc) return rc;
	}

	if(!topic || STREMPTY(topic)){
		if(topic) topic = NULL;

		if(dimq->protocol == dimq_p_mqtt5){
			p = outgoing_properties;
			have_topic_alias = false;
			while(p){
				if(p->identifier == MQTT_PROP_TOPIC_ALIAS){
					have_topic_alias = true;
					break;
				}
				p = p->next;
			}
			if(have_topic_alias == false){
				return dimq_ERR_INVAL;
			}
		}else{
			return dimq_ERR_INVAL;
		}
	}else{
		tlen = strlen(topic);
		if(dimq_validate_utf8(topic, (int)tlen)) return dimq_ERR_MALFORMED_UTF8;
		if(payloadlen < 0 || payloadlen > (int)MQTT_MAX_PAYLOAD) return dimq_ERR_PAYLOAD_SIZE;
		if(dimq_pub_topic_check(topic) != dimq_ERR_SUCCESS){
			return dimq_ERR_INVAL;
		}
	}

	if(dimq->maximum_packet_size > 0){
		remaining_length = 1 + 2+(uint32_t)tlen + (uint32_t)payloadlen + property__get_length_all(outgoing_properties);
		if(qos > 0){
			remaining_length++;
		}
		if(packet__check_oversize(dimq, remaining_length)){
			return dimq_ERR_OVERSIZE_PACKET;
		}
	}

	local_mid = dimq__mid_generate(dimq);
	if(mid){
		*mid = local_mid;
	}

	if(qos == 0){
		return send__publish(dimq, local_mid, topic, (uint32_t)payloadlen, payload, (uint8_t)qos, retain, false, outgoing_properties, NULL, 0);
	}else{
		if(outgoing_properties){
			rc = dimq_property_copy_all(&properties_copy, outgoing_properties);
			if(rc) return rc;
		}
		message = dimq__calloc(1, sizeof(struct dimq_message_all));
		if(!message){
			dimq_property_free_all(&properties_copy);
			return dimq_ERR_NOMEM;
		}

		message->next = NULL;
		message->timestamp = dimq_time();
		message->msg.mid = local_mid;
		if(topic){
			message->msg.topic = dimq__strdup(topic);
			if(!message->msg.topic){
				message__cleanup(&message);
				dimq_property_free_all(&properties_copy);
				return dimq_ERR_NOMEM;
			}
		}
		if(payloadlen){
			message->msg.payloadlen = payloadlen;
			message->msg.payload = dimq__malloc((unsigned int)payloadlen*sizeof(uint8_t));
			if(!message->msg.payload){
				message__cleanup(&message);
				dimq_property_free_all(&properties_copy);
				return dimq_ERR_NOMEM;
			}
			memcpy(message->msg.payload, payload, (uint32_t)payloadlen*sizeof(uint8_t));
		}else{
			message->msg.payloadlen = 0;
			message->msg.payload = NULL;
		}
		message->msg.qos = (uint8_t)qos;
		message->msg.retain = retain;
		message->dup = false;
		message->properties = properties_copy;

		pthread_mutex_lock(&dimq->msgs_out.mutex);
		message->state = dimq_ms_invalid;
		rc = message__queue(dimq, message, dimq_md_out);
		pthread_mutex_unlock(&dimq->msgs_out.mutex);
		return rc;
	}
}


int dimq_subscribe(struct dimq *dimq, int *mid, const char *sub, int qos)
{
	return dimq_subscribe_multiple(dimq, mid, 1, (char *const *const)&sub, qos, 0, NULL);
}


int dimq_subscribe_v5(struct dimq *dimq, int *mid, const char *sub, int qos, int options, const dimq_property *properties)
{
	return dimq_subscribe_multiple(dimq, mid, 1, (char *const *const)&sub, qos, options, properties);
}


int dimq_subscribe_multiple(struct dimq *dimq, int *mid, int sub_count, char *const *const sub, int qos, int options, const dimq_property *properties)
{
	const dimq_property *outgoing_properties = NULL;
	dimq_property local_property;
	int i;
	int rc;
	uint32_t remaining_length = 0;
	int slen;

	if(!dimq || !sub_count || !sub) return dimq_ERR_INVAL;
	if(dimq->protocol != dimq_p_mqtt5 && properties) return dimq_ERR_NOT_SUPPORTED;
	if(qos < 0 || qos > 2) return dimq_ERR_INVAL;
	if((options & 0x30) == 0x30 || (options & 0xC0) != 0) return dimq_ERR_INVAL;
	if(dimq->sock == INVALID_SOCKET) return dimq_ERR_NO_CONN;

	if(properties){
		if(properties->client_generated){
			outgoing_properties = properties;
		}else{
			memcpy(&local_property, properties, sizeof(dimq_property));
			local_property.client_generated = true;
			local_property.next = NULL;
			outgoing_properties = &local_property;
		}
		rc = dimq_property_check_all(CMD_SUBSCRIBE, outgoing_properties);
		if(rc) return rc;
	}

	for(i=0; i<sub_count; i++){
		if(dimq_sub_topic_check(sub[i])) return dimq_ERR_INVAL;
		slen = (int)strlen(sub[i]);
		if(dimq_validate_utf8(sub[i], slen)) return dimq_ERR_MALFORMED_UTF8;
		remaining_length += 2+(uint32_t)slen + 1;
	}

	if(dimq->maximum_packet_size > 0){
		remaining_length += 2 + property__get_length_all(outgoing_properties);
		if(packet__check_oversize(dimq, remaining_length)){
			return dimq_ERR_OVERSIZE_PACKET;
		}
	}
	if(dimq->protocol == dimq_p_mqtt311 || dimq->protocol == dimq_p_mqtt31){
		options = 0;
	}

	return send__subscribe(dimq, mid, sub_count, sub, qos|options, outgoing_properties);
}


int dimq_unsubscribe(struct dimq *dimq, int *mid, const char *sub)
{
	return dimq_unsubscribe_multiple(dimq, mid, 1, (char *const *const)&sub, NULL);
}

int dimq_unsubscribe_v5(struct dimq *dimq, int *mid, const char *sub, const dimq_property *properties)
{
	return dimq_unsubscribe_multiple(dimq, mid, 1, (char *const *const)&sub, properties);
}

int dimq_unsubscribe_multiple(struct dimq *dimq, int *mid, int sub_count, char *const *const sub, const dimq_property *properties)
{
	const dimq_property *outgoing_properties = NULL;
	dimq_property local_property;
	int rc;
	int i;
	uint32_t remaining_length = 0;
	int slen;

	if(!dimq) return dimq_ERR_INVAL;
	if(dimq->protocol != dimq_p_mqtt5 && properties) return dimq_ERR_NOT_SUPPORTED;
	if(dimq->sock == INVALID_SOCKET) return dimq_ERR_NO_CONN;

	if(properties){
		if(properties->client_generated){
			outgoing_properties = properties;
		}else{
			memcpy(&local_property, properties, sizeof(dimq_property));
			local_property.client_generated = true;
			local_property.next = NULL;
			outgoing_properties = &local_property;
		}
		rc = dimq_property_check_all(CMD_UNSUBSCRIBE, outgoing_properties);
		if(rc) return rc;
	}

	for(i=0; i<sub_count; i++){
		if(dimq_sub_topic_check(sub[i])) return dimq_ERR_INVAL;
		slen = (int)strlen(sub[i]);
		if(dimq_validate_utf8(sub[i], slen)) return dimq_ERR_MALFORMED_UTF8;
		remaining_length += 2U + (uint32_t)slen;
	}

	if(dimq->maximum_packet_size > 0){
		remaining_length += 2U + property__get_length_all(outgoing_properties);
		if(packet__check_oversize(dimq, remaining_length)){
			return dimq_ERR_OVERSIZE_PACKET;
		}
	}

	return send__unsubscribe(dimq, mid, sub_count, sub, outgoing_properties);
}

