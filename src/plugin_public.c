/*
Copyright (c) 2016-2020 Roger Light <roger@atchoo.org>

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

#include "dimq_broker_internal.h"
#include "dimq_internal.h"
#include "dimq_broker.h"
#include "memory_dimq.h"
#include "mqtt_protocol.h"
#include "send_dimq.h"
#include "util_dimq.h"
#include "utlist.h"

#ifdef WITH_TLS
#  include <openssl/ssl.h>
#endif

const char *dimq_client_address(const struct dimq *client)
{
	if(client){
		return client->address;
	}else{
		return NULL;
	}
}


bool dimq_client_clean_session(const struct dimq *client)
{
	if(client){
		return client->clean_start;
	}else{
		return true;
	}
}


const char *dimq_client_id(const struct dimq *client)
{
	if(client){
		return client->id;
	}else{
		return NULL;
	}
}


int dimq_client_keepalive(const struct dimq *client)
{
	if(client){
		return client->keepalive;
	}else{
		return -1;
	}
}


void *dimq_client_certificate(const struct dimq *client)
{
#ifdef WITH_TLS
	if(client && client->ssl){
		return SSL_get_peer_certificate(client->ssl);
	}else{
		return NULL;
	}
#else
	UNUSED(client);

	return NULL;
#endif
}


int dimq_client_protocol(const struct dimq *client)
{
#ifdef WITH_WEBSOCKETS
	if(client && client->wsi){
		return mp_websockets;
	}else
#else
	UNUSED(client);
#endif
	{
		return mp_mqtt;
	}
}


int dimq_client_protocol_version(const struct dimq *client)
{
	if(client){
		switch(client->protocol){
			case dimq_p_mqtt31:
				return 3;
			case dimq_p_mqtt311:
				return 4;
			case dimq_p_mqtt5:
				return 5;
			default:
				return 0;
		}
	}else{
		return 0;
	}
}


int dimq_client_sub_count(const struct dimq *client)
{
	if(client){
		return client->sub_count;
	}else{
		return 0;
	}
}


const char *dimq_client_username(const struct dimq *client)
{
	if(client){
#ifdef WITH_BRIDGE
		if(client->bridge){
			return client->bridge->local_username;
		}else
#endif
		{
			return client->username;
		}
	}else{
		return NULL;
	}
}


int dimq_broker_publish(
		const char *clientid,
		const char *topic,
		int payloadlen,
		void *payload,
		int qos,
		bool retain,
		dimq_property *properties)
{
	struct dimq_message_v5 *msg;

	if(topic == NULL
			|| payloadlen < 0
			|| (payloadlen > 0 && payload == NULL)
			|| qos < 0 || qos > 2){

		return dimq_ERR_INVAL;
	}

	msg = dimq__malloc(sizeof(struct dimq_message_v5));
	if(msg == NULL) return dimq_ERR_NOMEM;
	
	msg->next = NULL;
	msg->prev = NULL;
	if(clientid){
		msg->clientid = dimq__strdup(clientid);
		if(msg->clientid == NULL){
			dimq__free(msg);
			return dimq_ERR_NOMEM;
		}
	}else{
		msg->clientid = NULL;
	}
	msg->topic = dimq__strdup(topic);
	if(msg->topic == NULL){
		dimq__free(msg->clientid);
		dimq__free(msg);
		return dimq_ERR_NOMEM;
	}
	msg->payloadlen = payloadlen;
	msg->payload = payload;
	msg->qos = qos;
	msg->retain = retain;
	msg->properties = properties;

	DL_APPEND(db.plugin_msgs, msg);

	return dimq_ERR_SUCCESS;
}


int dimq_broker_publish_copy(
		const char *clientid,
		const char *topic,
		int payloadlen,
		const void *payload,
		int qos,
		bool retain,
		dimq_property *properties)
{
	void *payload_out;

	if(topic == NULL
			|| payloadlen < 0
			|| (payloadlen > 0 && payload == NULL)
			|| qos < 0 || qos > 2){

		return dimq_ERR_INVAL;
	}

	payload_out = calloc(1, (size_t)(payloadlen+1));
	if(payload_out == NULL){
		return dimq_ERR_NOMEM;
	}
	memcpy(payload_out, payload, (size_t)payloadlen);

	return dimq_broker_publish(
			clientid,
			topic,
			payloadlen,
			payload_out,
			qos,
			retain,
			properties);
}


int dimq_set_username(struct dimq *client, const char *username)
{
	char *u_dup;
	char *old;
	int rc;

	if(!client) return dimq_ERR_INVAL;

	if(username){
		u_dup = dimq__strdup(username);
		if(!u_dup) return dimq_ERR_NOMEM;
	}else{
		u_dup = NULL;
	}

	old = client->username;
	client->username = u_dup;

	rc = acl__find_acls(client);
	if(rc){
		client->username = old;
		dimq__free(u_dup);
		return rc;
	}else{
		dimq__free(old);
		return dimq_ERR_SUCCESS;
	}
}


/* Check to see whether durable clients still have rights to their subscriptions. */
static void check_subscription_acls(struct dimq *context)
{
	int i;
	int rc;
	uint8_t reason;

	for(i=0; i<context->sub_count; i++){
		if(context->subs[i] == NULL){
			continue;
		}
		rc = dimq_acl_check(context,
				context->subs[i]->topic_filter,
				0,
				NULL,
				0, /* FIXME */
				false,
				dimq_ACL_SUBSCRIBE);

		if(rc != dimq_ERR_SUCCESS){
			sub__remove(context, context->subs[i]->topic_filter, db.subs, &reason);
		}
	}
}



static void disconnect_client(struct dimq *context, bool with_will)
{
	if(context->protocol == dimq_p_mqtt5){
		send__disconnect(context, MQTT_RC_ADMINISTRATIVE_ACTION, NULL);
	}
	if(with_will == false){
		dimq__set_state(context, dimq_cs_disconnecting);
	}
	if(context->session_expiry_interval > 0){
		check_subscription_acls(context);
	}
	do_disconnect(context, dimq_ERR_ADMINISTRATIVE_ACTION);
}

int dimq_kick_client_by_clientid(const char *clientid, bool with_will)
{
	struct dimq *ctxt, *ctxt_tmp;

	if(clientid == NULL){
		HASH_ITER(hh_sock, db.contexts_by_sock, ctxt, ctxt_tmp){
			disconnect_client(ctxt, with_will);
		}
		return dimq_ERR_SUCCESS;
	}else{
		HASH_FIND(hh_id, db.contexts_by_id, clientid, strlen(clientid), ctxt);
		if(ctxt){
			disconnect_client(ctxt, with_will);
			return dimq_ERR_SUCCESS;
		}else{
			return dimq_ERR_NOT_FOUND;
		}
	}
}

int dimq_kick_client_by_username(const char *username, bool with_will)
{
	struct dimq *ctxt, *ctxt_tmp;

	if(username == NULL){
		HASH_ITER(hh_sock, db.contexts_by_sock, ctxt, ctxt_tmp){
			if(ctxt->username == NULL){
				disconnect_client(ctxt, with_will);
			}
		}
	}else{
		HASH_ITER(hh_sock, db.contexts_by_sock, ctxt, ctxt_tmp){
			if(ctxt->username != NULL && !strcmp(ctxt->username, username)){
				disconnect_client(ctxt, with_will);
			}
		}
	}
	return dimq_ERR_SUCCESS;
}
