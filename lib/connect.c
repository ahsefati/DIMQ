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
#include "logging_dimq.h"
#include "messages_dimq.h"
#include "memory_dimq.h"
#include "packet_dimq.h"
#include "mqtt_protocol.h"
#include "net_dimq.h"
#include "send_dimq.h"
#include "socks_dimq.h"
#include "util_dimq.h"

static char alphanum[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

static int dimq__reconnect(struct dimq *dimq, bool blocking);
static int dimq__connect_init(struct dimq *dimq, const char *host, int port, int keepalive);


static int dimq__connect_init(struct dimq *dimq, const char *host, int port, int keepalive)
{
	int i;
	int rc;

	if(!dimq) return dimq_ERR_INVAL;
	if(!host || port < 0 || port > UINT16_MAX) return dimq_ERR_INVAL;
	if(keepalive != 0 && (keepalive < 5 || keepalive > UINT16_MAX)) return dimq_ERR_INVAL;

	/* Only MQTT v3.1 requires a client id to be sent */
	if(dimq->id == NULL && (dimq->protocol == dimq_p_mqtt31)){
		dimq->id = (char *)dimq__calloc(24, sizeof(char));
		if(!dimq->id){
			return dimq_ERR_NOMEM;
		}
		dimq->id[0] = 'm';
		dimq->id[1] = 'o';
		dimq->id[2] = 's';
		dimq->id[3] = 'q';
		dimq->id[4] = '-';

		rc = util__random_bytes(&dimq->id[5], 18);
		if(rc) return rc;

		for(i=5; i<23; i++){
			dimq->id[i] = alphanum[(dimq->id[i]&0x7F)%(sizeof(alphanum)-1)];
		}
	}

	dimq__free(dimq->host);
	dimq->host = dimq__strdup(host);
	if(!dimq->host) return dimq_ERR_NOMEM;
	dimq->port = (uint16_t)port;

	dimq->keepalive = (uint16_t)keepalive;
	dimq->msgs_in.inflight_quota = dimq->msgs_in.inflight_maximum;
	dimq->msgs_out.inflight_quota = dimq->msgs_out.inflight_maximum;
	dimq->retain_available = 1;

	return dimq_ERR_SUCCESS;
}


int dimq_connect(struct dimq *dimq, const char *host, int port, int keepalive)
{
	return dimq_connect_bind(dimq, host, port, keepalive, NULL);
}


int dimq_connect_bind(struct dimq *dimq, const char *host, int port, int keepalive, const char *bind_address)
{
	return dimq_connect_bind_v5(dimq, host, port, keepalive, bind_address, NULL);
}

int dimq_connect_bind_v5(struct dimq *dimq, const char *host, int port, int keepalive, const char *bind_address, const dimq_property *properties)
{
	int rc;

	if(bind_address){
		rc = dimq_string_option(dimq, dimq_OPT_BIND_ADDRESS, bind_address);
		if(rc) return rc;
	}

	dimq_property_free_all(&dimq->connect_properties);
	if(properties){
		rc = dimq_property_check_all(CMD_CONNECT, properties);
		if(rc) return rc;

		rc = dimq_property_copy_all(&dimq->connect_properties, properties);
		if(rc) return rc;
		dimq->connect_properties->client_generated = true;
	}

	rc = dimq__connect_init(dimq, host, port, keepalive);
	if(rc) return rc;

	dimq__set_state(dimq, dimq_cs_new);

	return dimq__reconnect(dimq, true);
}


int dimq_connect_async(struct dimq *dimq, const char *host, int port, int keepalive)
{
	return dimq_connect_bind_async(dimq, host, port, keepalive, NULL);
}


int dimq_connect_bind_async(struct dimq *dimq, const char *host, int port, int keepalive, const char *bind_address)
{
	int rc;

	if(bind_address){
		rc = dimq_string_option(dimq, dimq_OPT_BIND_ADDRESS, bind_address);
		if(rc) return rc;
	}

	rc = dimq__connect_init(dimq, host, port, keepalive);
	if(rc) return rc;

	return dimq__reconnect(dimq, false);
}


int dimq_reconnect_async(struct dimq *dimq)
{
	return dimq__reconnect(dimq, false);
}


int dimq_reconnect(struct dimq *dimq)
{
	return dimq__reconnect(dimq, true);
}


static int dimq__reconnect(struct dimq *dimq, bool blocking)
{
	const dimq_property *outgoing_properties = NULL;
	dimq_property local_property;
	int rc;

	if(!dimq) return dimq_ERR_INVAL;
	if(!dimq->host) return dimq_ERR_INVAL;

	if(dimq->connect_properties){
		if(dimq->protocol != dimq_p_mqtt5) return dimq_ERR_NOT_SUPPORTED;

		if(dimq->connect_properties->client_generated){
			outgoing_properties = dimq->connect_properties;
		}else{
			memcpy(&local_property, dimq->connect_properties, sizeof(dimq_property));
			local_property.client_generated = true;
			local_property.next = NULL;
			outgoing_properties = &local_property;
		}
		rc = dimq_property_check_all(CMD_CONNECT, outgoing_properties);
		if(rc) return rc;
	}

	pthread_mutex_lock(&dimq->msgtime_mutex);
	dimq->last_msg_in = dimq_time();
	dimq->next_msg_out = dimq->last_msg_in + dimq->keepalive;
	pthread_mutex_unlock(&dimq->msgtime_mutex);

	dimq->ping_t = 0;

	packet__cleanup(&dimq->in_packet);

	packet__cleanup_all(dimq);

	message__reconnect_reset(dimq, false);

	if(dimq->sock != INVALID_SOCKET){
        net__socket_close(dimq);
    }

#ifdef WITH_SOCKS
	if(dimq->socks5_host){
		rc = net__socket_connect(dimq, dimq->socks5_host, dimq->socks5_port, dimq->bind_address, blocking);
	}else
#endif
	{
		rc = net__socket_connect(dimq, dimq->host, dimq->port, dimq->bind_address, blocking);
	}
	if(rc>0){
		dimq__set_state(dimq, dimq_cs_connect_pending);
		return rc;
	}

#ifdef WITH_SOCKS
	if(dimq->socks5_host){
		dimq__set_state(dimq, dimq_cs_socks5_new);
		return socks5__send(dimq);
	}else
#endif
	{
		dimq__set_state(dimq, dimq_cs_connected);
		rc = send__connect(dimq, dimq->keepalive, dimq->clean_start, outgoing_properties);
		if(rc){
			packet__cleanup_all(dimq);
			net__socket_close(dimq);
			dimq__set_state(dimq, dimq_cs_new);
		}
		return rc;
	}
}


int dimq_disconnect(struct dimq *dimq)
{
	return dimq_disconnect_v5(dimq, 0, NULL);
}

int dimq_disconnect_v5(struct dimq *dimq, int reason_code, const dimq_property *properties)
{
	const dimq_property *outgoing_properties = NULL;
	dimq_property local_property;
	int rc;
	if(!dimq) return dimq_ERR_INVAL;
	if(dimq->protocol != dimq_p_mqtt5 && properties) return dimq_ERR_NOT_SUPPORTED;
	if(reason_code < 0 || reason_code > UINT8_MAX) return dimq_ERR_INVAL;

	if(properties){
		if(properties->client_generated){
			outgoing_properties = properties;
		}else{
			memcpy(&local_property, properties, sizeof(dimq_property));
			local_property.client_generated = true;
			local_property.next = NULL;
			outgoing_properties = &local_property;
		}
		rc = dimq_property_check_all(CMD_DISCONNECT, outgoing_properties);
		if(rc) return rc;
	}

	dimq__set_state(dimq, dimq_cs_disconnected);
	if(dimq->sock == INVALID_SOCKET){
		return dimq_ERR_NO_CONN;
	}else{
		return send__disconnect(dimq, (uint8_t)reason_code, outgoing_properties);
	}
}


void do_client_disconnect(struct dimq *dimq, int reason_code, const dimq_property *properties)
{
	dimq__set_state(dimq, dimq_cs_disconnected);
	net__socket_close(dimq);

	/* Free data and reset values */
	pthread_mutex_lock(&dimq->out_packet_mutex);
	dimq->current_out_packet = dimq->out_packet;
	if(dimq->out_packet){
		dimq->out_packet = dimq->out_packet->next;
		if(!dimq->out_packet){
			dimq->out_packet_last = NULL;
		}
		dimq->out_packet_count--;
	}
	pthread_mutex_unlock(&dimq->out_packet_mutex);

	pthread_mutex_lock(&dimq->msgtime_mutex);
	dimq->next_msg_out = dimq_time() + dimq->keepalive;
	pthread_mutex_unlock(&dimq->msgtime_mutex);

	pthread_mutex_lock(&dimq->callback_mutex);
	if(dimq->on_disconnect){
		dimq->in_callback = true;
		dimq->on_disconnect(dimq, dimq->userdata, reason_code);
		dimq->in_callback = false;
	}
	if(dimq->on_disconnect_v5){
		dimq->in_callback = true;
		dimq->on_disconnect_v5(dimq, dimq->userdata, reason_code, properties);
		dimq->in_callback = false;
	}
	pthread_mutex_unlock(&dimq->callback_mutex);
	pthread_mutex_unlock(&dimq->current_out_packet_mutex);
}

