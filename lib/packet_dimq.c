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
#include <errno.h>
#include <string.h>

#ifdef WITH_BROKER
#  include "dimq_broker_internal.h"
#  ifdef WITH_WEBSOCKETS
#    include <libwebsockets.h>
#  endif
#else
#  include "read_handle.h"
#endif

#include "memory_dimq.h"
#include "mqtt_protocol.h"
#include "net_dimq.h"
#include "packet_dimq.h"
#include "read_handle.h"
#include "util_dimq.h"
#ifdef WITH_BROKER
#  include "sys_tree.h"
#  include "send_dimq.h"
#else
#  define G_BYTES_RECEIVED_INC(A)
#  define G_BYTES_SENT_INC(A)
#  define G_MSGS_SENT_INC(A)
#  define G_PUB_MSGS_SENT_INC(A)
#endif

int packet__alloc(struct dimq__packet *packet)
{
	uint8_t remaining_bytes[5], byte;
	uint32_t remaining_length;
	int i;

	assert(packet);

	remaining_length = packet->remaining_length;
	packet->payload = NULL;
	packet->remaining_count = 0;
	do{
		byte = remaining_length % 128;
		remaining_length = remaining_length / 128;
		/* If there are more digits to encode, set the top bit of this digit */
		if(remaining_length > 0){
			byte = byte | 0x80;
		}
		remaining_bytes[packet->remaining_count] = byte;
		packet->remaining_count++;
	}while(remaining_length > 0 && packet->remaining_count < 5);
	if(packet->remaining_count == 5) return dimq_ERR_PAYLOAD_SIZE;
	packet->packet_length = packet->remaining_length + 1 + (uint8_t)packet->remaining_count;
#ifdef WITH_WEBSOCKETS
	packet->payload = dimq__malloc(sizeof(uint8_t)*packet->packet_length + LWS_PRE);
#else
	packet->payload = dimq__malloc(sizeof(uint8_t)*packet->packet_length);
#endif
	if(!packet->payload) return dimq_ERR_NOMEM;

	packet->payload[0] = packet->command;
	for(i=0; i<packet->remaining_count; i++){
		packet->payload[i+1] = remaining_bytes[i];
	}
	packet->pos = 1U + (uint8_t)packet->remaining_count;

	return dimq_ERR_SUCCESS;
}

void packet__cleanup(struct dimq__packet *packet)
{
	if(!packet) return;

	/* Free data and reset values */
	packet->command = 0;
	packet->remaining_count = 0;
	packet->remaining_mult = 1;
	packet->remaining_length = 0;
	dimq__free(packet->payload);
	packet->payload = NULL;
	packet->to_process = 0;
	packet->pos = 0;
}


void packet__cleanup_all_no_locks(struct dimq *dimq)
{
	struct dimq__packet *packet;

	/* Out packet cleanup */
	if(dimq->out_packet && !dimq->current_out_packet){
		dimq->current_out_packet = dimq->out_packet;
		dimq->out_packet = dimq->out_packet->next;
	}
	while(dimq->current_out_packet){
		packet = dimq->current_out_packet;
		/* Free data and reset values */
		dimq->current_out_packet = dimq->out_packet;
		if(dimq->out_packet){
			dimq->out_packet = dimq->out_packet->next;
		}

		packet__cleanup(packet);
		dimq__free(packet);
	}
	dimq->out_packet_count = 0;

	packet__cleanup(&dimq->in_packet);
}

void packet__cleanup_all(struct dimq *dimq)
{
	pthread_mutex_lock(&dimq->current_out_packet_mutex);
	pthread_mutex_lock(&dimq->out_packet_mutex);

	packet__cleanup_all_no_locks(dimq);

	pthread_mutex_unlock(&dimq->out_packet_mutex);
	pthread_mutex_unlock(&dimq->current_out_packet_mutex);
}


int packet__queue(struct dimq *dimq, struct dimq__packet *packet)
{
#ifndef WITH_BROKER
	char sockpair_data = 0;
#endif
	assert(dimq);
	assert(packet);

	packet->pos = 0;
	packet->to_process = packet->packet_length;

	packet->next = NULL;
	pthread_mutex_lock(&dimq->out_packet_mutex);
	if(dimq->out_packet){
		dimq->out_packet_last->next = packet;
	}else{
		dimq->out_packet = packet;
	}
	dimq->out_packet_last = packet;
	dimq->out_packet_count++;
	pthread_mutex_unlock(&dimq->out_packet_mutex);
#ifdef WITH_BROKER
#  ifdef WITH_WEBSOCKETS
	if(dimq->wsi){
		lws_callback_on_writable(dimq->wsi);
		return dimq_ERR_SUCCESS;
	}else{
		return packet__write(dimq);
	}
#  else
	return packet__write(dimq);
#  endif
#else

	/* Write a single byte to sockpairW (connected to sockpairR) to break out
	 * of select() if in threaded mode. */
	if(dimq->sockpairW != INVALID_SOCKET){
#ifndef WIN32
		if(write(dimq->sockpairW, &sockpair_data, 1)){
		}
#else
		send(dimq->sockpairW, &sockpair_data, 1, 0);
#endif
	}

	if(dimq->in_callback == false && dimq->threaded == dimq_ts_none){
		return packet__write(dimq);
	}else{
		return dimq_ERR_SUCCESS;
	}
#endif
}


int packet__check_oversize(struct dimq *dimq, uint32_t remaining_length)
{
	uint32_t len;

	if(dimq->maximum_packet_size == 0) return dimq_ERR_SUCCESS;

	len = remaining_length + packet__varint_bytes(remaining_length);
	if(len > dimq->maximum_packet_size){
		return dimq_ERR_OVERSIZE_PACKET;
	}else{
		return dimq_ERR_SUCCESS;
	}
}


int packet__write(struct dimq *dimq)
{
	ssize_t write_length;
	struct dimq__packet *packet;
	enum dimq_client_state state;

	if(!dimq) return dimq_ERR_INVAL;
	if(dimq->sock == INVALID_SOCKET) return dimq_ERR_NO_CONN;

	pthread_mutex_lock(&dimq->current_out_packet_mutex);
	pthread_mutex_lock(&dimq->out_packet_mutex);
	if(dimq->out_packet && !dimq->current_out_packet){
		dimq->current_out_packet = dimq->out_packet;
		dimq->out_packet = dimq->out_packet->next;
		if(!dimq->out_packet){
			dimq->out_packet_last = NULL;
		}
		dimq->out_packet_count--;
	}
	pthread_mutex_unlock(&dimq->out_packet_mutex);

#ifdef WITH_BROKER
	if(dimq->current_out_packet){
	   mux__add_out(dimq);
	}
#endif

	state = dimq__get_state(dimq);
#if defined(WITH_TLS) && !defined(WITH_BROKER)
	if(state == dimq_cs_connect_pending || dimq->want_connect){
#else
	if(state == dimq_cs_connect_pending){
#endif
		pthread_mutex_unlock(&dimq->current_out_packet_mutex);
		return dimq_ERR_SUCCESS;
	}

	while(dimq->current_out_packet){
		packet = dimq->current_out_packet;

		while(packet->to_process > 0){
			write_length = net__write(dimq, &(packet->payload[packet->pos]), packet->to_process);
			if(write_length > 0){
				G_BYTES_SENT_INC(write_length);
				packet->to_process -= (uint32_t)write_length;
				packet->pos += (uint32_t)write_length;
			}else{
#ifdef WIN32
				errno = WSAGetLastError();
#endif
				if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK
#ifdef WIN32
						|| errno == WSAENOTCONN
#endif
						){
					pthread_mutex_unlock(&dimq->current_out_packet_mutex);
					return dimq_ERR_SUCCESS;
				}else{
					pthread_mutex_unlock(&dimq->current_out_packet_mutex);
					switch(errno){
						case COMPAT_ECONNRESET:
							return dimq_ERR_CONN_LOST;
						case COMPAT_EINTR:
							return dimq_ERR_SUCCESS;
						default:
							return dimq_ERR_ERRNO;
					}
				}
			}
		}

		G_MSGS_SENT_INC(1);
		if(((packet->command)&0xF6) == CMD_PUBLISH){
			G_PUB_MSGS_SENT_INC(1);
#ifndef WITH_BROKER
			pthread_mutex_lock(&dimq->callback_mutex);
			if(dimq->on_publish){
				/* This is a QoS=0 message */
				dimq->in_callback = true;
				dimq->on_publish(dimq, dimq->userdata, packet->mid);
				dimq->in_callback = false;
			}
			if(dimq->on_publish_v5){
				/* This is a QoS=0 message */
				dimq->in_callback = true;
				dimq->on_publish_v5(dimq, dimq->userdata, packet->mid, 0, NULL);
				dimq->in_callback = false;
			}
			pthread_mutex_unlock(&dimq->callback_mutex);
		}else if(((packet->command)&0xF0) == CMD_DISCONNECT){
			do_client_disconnect(dimq, dimq_ERR_SUCCESS, NULL);
			packet__cleanup(packet);
			dimq__free(packet);
			return dimq_ERR_SUCCESS;
#endif
		}else if(((packet->command)&0xF0) == CMD_PUBLISH){
			G_PUB_MSGS_SENT_INC(1);
		}

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

		packet__cleanup(packet);
		dimq__free(packet);

#ifdef WITH_BROKER
		dimq->next_msg_out = db.now_s + dimq->keepalive;
#else
		pthread_mutex_lock(&dimq->msgtime_mutex);
		dimq->next_msg_out = dimq_time() + dimq->keepalive;
		pthread_mutex_unlock(&dimq->msgtime_mutex);
#endif
	}
#ifdef WITH_BROKER
	if (dimq->current_out_packet == NULL) {
		mux__remove_out(dimq);
	}
#endif
	pthread_mutex_unlock(&dimq->current_out_packet_mutex);
	return dimq_ERR_SUCCESS;
}


int packet__read(struct dimq *dimq)
{
	uint8_t byte;
	ssize_t read_length;
	int rc = 0;
	enum dimq_client_state state;

	if(!dimq){
		return dimq_ERR_INVAL;
	}
	if(dimq->sock == INVALID_SOCKET){
		return dimq_ERR_NO_CONN;
	}

	state = dimq__get_state(dimq);
	if(state == dimq_cs_connect_pending){
		return dimq_ERR_SUCCESS;
	}

	/* This gets called if pselect() indicates that there is network data
	 * available - ie. at least one byte.  What we do depends on what data we
	 * already have.
	 * If we've not got a command, attempt to read one and save it. This should
	 * always work because it's only a single byte.
	 * Then try to read the remaining length. This may fail because it is may
	 * be more than one byte - will need to save data pending next read if it
	 * does fail.
	 * Then try to read the remaining payload, where 'payload' here means the
	 * combined variable header and actual payload. This is the most likely to
	 * fail due to longer length, so save current data and current position.
	 * After all data is read, send to dimq__handle_packet() to deal with.
	 * Finally, free the memory and reset everything to starting conditions.
	 */
	if(!dimq->in_packet.command){
		read_length = net__read(dimq, &byte, 1);
		if(read_length == 1){
			dimq->in_packet.command = byte;
#ifdef WITH_BROKER
			G_BYTES_RECEIVED_INC(1);
			/* Clients must send CONNECT as their first command. */
			if(!(dimq->bridge) && state == dimq_cs_connected && (byte&0xF0) != CMD_CONNECT){
				return dimq_ERR_PROTOCOL;
			}
#endif
		}else{
			if(read_length == 0){
				return dimq_ERR_CONN_LOST; /* EOF */
			}
#ifdef WIN32
			errno = WSAGetLastError();
#endif
			if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
				return dimq_ERR_SUCCESS;
			}else{
				switch(errno){
					case COMPAT_ECONNRESET:
						return dimq_ERR_CONN_LOST;
					case COMPAT_EINTR:
						return dimq_ERR_SUCCESS;
					default:
						return dimq_ERR_ERRNO;
				}
			}
		}
	}
	/* remaining_count is the number of bytes that the remaining_length
	 * parameter occupied in this incoming packet. We don't use it here as such
	 * (it is used when allocating an outgoing packet), but we must be able to
	 * determine whether all of the remaining_length parameter has been read.
	 * remaining_count has three states here:
	 *   0 means that we haven't read any remaining_length bytes
	 *   <0 means we have read some remaining_length bytes but haven't finished
	 *   >0 means we have finished reading the remaining_length bytes.
	 */
	if(dimq->in_packet.remaining_count <= 0){
		do{
			read_length = net__read(dimq, &byte, 1);
			if(read_length == 1){
				dimq->in_packet.remaining_count--;
				/* Max 4 bytes length for remaining length as defined by protocol.
				 * Anything more likely means a broken/malicious client.
				 */
				if(dimq->in_packet.remaining_count < -4){
					return dimq_ERR_MALFORMED_PACKET;
				}

				G_BYTES_RECEIVED_INC(1);
				dimq->in_packet.remaining_length += (byte & 127) * dimq->in_packet.remaining_mult;
				dimq->in_packet.remaining_mult *= 128;
			}else{
				if(read_length == 0){
					return dimq_ERR_CONN_LOST; /* EOF */
				}
#ifdef WIN32
				errno = WSAGetLastError();
#endif
				if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
					return dimq_ERR_SUCCESS;
				}else{
					switch(errno){
						case COMPAT_ECONNRESET:
							return dimq_ERR_CONN_LOST;
						case COMPAT_EINTR:
							return dimq_ERR_SUCCESS;
						default:
							return dimq_ERR_ERRNO;
					}
				}
			}
		}while((byte & 128) != 0);
		/* We have finished reading remaining_length, so make remaining_count
		 * positive. */
		dimq->in_packet.remaining_count = (int8_t)(dimq->in_packet.remaining_count * -1);

#ifdef WITH_BROKER
		switch(dimq->in_packet.command & 0xF0){
			case CMD_CONNECT:
				if(dimq->in_packet.remaining_length > 100000){ /* Arbitrary limit, make configurable */
					return dimq_ERR_MALFORMED_PACKET;
				}
				break;

			case CMD_PUBACK:
			case CMD_PUBREC:
			case CMD_PUBREL:
			case CMD_PUBCOMP:
			case CMD_UNSUBACK:
				if(dimq->protocol != dimq_p_mqtt5 && dimq->in_packet.remaining_length != 2){
					return dimq_ERR_MALFORMED_PACKET;
				}
				break;

			case CMD_PINGREQ:
			case CMD_PINGRESP:
				if(dimq->in_packet.remaining_length != 0){
					return dimq_ERR_MALFORMED_PACKET;
				}
				break;

			case CMD_DISCONNECT:
				if(dimq->protocol != dimq_p_mqtt5 && dimq->in_packet.remaining_length != 0){
					return dimq_ERR_MALFORMED_PACKET;
				}
				break;
		}

		if(db.config->max_packet_size > 0 && dimq->in_packet.remaining_length+1 > db.config->max_packet_size){
			if(dimq->protocol == dimq_p_mqtt5){
				send__disconnect(dimq, MQTT_RC_PACKET_TOO_LARGE, NULL);
			}
			return dimq_ERR_OVERSIZE_PACKET;
		}
#else
		/* FIXME - client case for incoming message received from broker too large */
#endif
		if(dimq->in_packet.remaining_length > 0){
			dimq->in_packet.payload = dimq__malloc(dimq->in_packet.remaining_length*sizeof(uint8_t));
			if(!dimq->in_packet.payload){
				return dimq_ERR_NOMEM;
			}
			dimq->in_packet.to_process = dimq->in_packet.remaining_length;
		}
	}
	while(dimq->in_packet.to_process>0){
		read_length = net__read(dimq, &(dimq->in_packet.payload[dimq->in_packet.pos]), dimq->in_packet.to_process);
		if(read_length > 0){
			G_BYTES_RECEIVED_INC(read_length);
			dimq->in_packet.to_process -= (uint32_t)read_length;
			dimq->in_packet.pos += (uint32_t)read_length;
		}else{
#ifdef WIN32
			errno = WSAGetLastError();
#endif
			if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
				if(dimq->in_packet.to_process > 1000){
					/* Update last_msg_in time if more than 1000 bytes left to
					 * receive. Helps when receiving large messages.
					 * This is an arbitrary limit, but with some consideration.
					 * If a client can't send 1000 bytes in a second it
					 * probably shouldn't be using a 1 second keep alive. */
#ifdef WITH_BROKER
					keepalive__update(dimq);
#else
					pthread_mutex_lock(&dimq->msgtime_mutex);
					dimq->last_msg_in = dimq_time();
					pthread_mutex_unlock(&dimq->msgtime_mutex);
#endif
				}
				return dimq_ERR_SUCCESS;
			}else{
				switch(errno){
					case COMPAT_ECONNRESET:
						return dimq_ERR_CONN_LOST;
					case COMPAT_EINTR:
						return dimq_ERR_SUCCESS;
					default:
						return dimq_ERR_ERRNO;
				}
			}
		}
	}

	/* All data for this packet is read. */
	dimq->in_packet.pos = 0;
#ifdef WITH_BROKER
	G_MSGS_RECEIVED_INC(1);
	if(((dimq->in_packet.command)&0xF5) == CMD_PUBLISH){
		G_PUB_MSGS_RECEIVED_INC(1);
	}
#endif
	rc = handle__packet(dimq);

	/* Free data and reset values */
	packet__cleanup(&dimq->in_packet);

#ifdef WITH_BROKER
	keepalive__update(dimq);
#else
	pthread_mutex_lock(&dimq->msgtime_mutex);
	dimq->last_msg_in = dimq_time();
	pthread_mutex_unlock(&dimq->msgtime_mutex);
#endif
	return rc;
}
