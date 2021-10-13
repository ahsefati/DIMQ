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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <utlist.h>

#include "dimq_internal.h"
#include "dimq.h"
#include "memory_dimq.h"
#include "messages_dimq.h"
#include "send_dimq.h"
#include "time_dimq.h"
#include "util_dimq.h"

void message__cleanup(struct dimq_message_all **message)
{
	struct dimq_message_all *msg;

	if(!message || !*message) return;

	msg = *message;

	dimq__free(msg->msg.topic);
	dimq__free(msg->msg.payload);
	dimq_property_free_all(&msg->properties);
	dimq__free(msg);
}

void message__cleanup_all(struct dimq *dimq)
{
	struct dimq_message_all *tail, *tmp;

	assert(dimq);

	DL_FOREACH_SAFE(dimq->msgs_in.inflight, tail, tmp){
		DL_DELETE(dimq->msgs_in.inflight, tail);
		message__cleanup(&tail);
	}
	DL_FOREACH_SAFE(dimq->msgs_out.inflight, tail, tmp){
		DL_DELETE(dimq->msgs_out.inflight, tail);
		message__cleanup(&tail);
	}
}

int dimq_message_copy(struct dimq_message *dst, const struct dimq_message *src)
{
	if(!dst || !src) return dimq_ERR_INVAL;

	dst->mid = src->mid;
	dst->topic = dimq__strdup(src->topic);
	if(!dst->topic) return dimq_ERR_NOMEM;
	dst->qos = src->qos;
	dst->retain = src->retain;
	if(src->payloadlen){
		dst->payload = dimq__calloc((unsigned int)src->payloadlen+1, sizeof(uint8_t));
		if(!dst->payload){
			dimq__free(dst->topic);
			return dimq_ERR_NOMEM;
		}
		memcpy(dst->payload, src->payload, (unsigned int)src->payloadlen);
		dst->payloadlen = src->payloadlen;
	}else{
		dst->payloadlen = 0;
		dst->payload = NULL;
	}
	return dimq_ERR_SUCCESS;
}

int message__delete(struct dimq *dimq, uint16_t mid, enum dimq_msg_direction dir, int qos)
{
	struct dimq_message_all *message;
	int rc;
	assert(dimq);

	rc = message__remove(dimq, mid, dir, &message, qos);
	if(rc == dimq_ERR_SUCCESS){
		message__cleanup(&message);
	}
	return rc;
}

void dimq_message_free(struct dimq_message **message)
{
	struct dimq_message *msg;

	if(!message || !*message) return;

	msg = *message;

	dimq__free(msg->topic);
	dimq__free(msg->payload);
	dimq__free(msg);
}

void dimq_message_free_contents(struct dimq_message *message)
{
	if(!message) return;

	dimq__free(message->topic);
	dimq__free(message->payload);
}

int message__queue(struct dimq *dimq, struct dimq_message_all *message, enum dimq_msg_direction dir)
{
	/* dimq->*_message_mutex should be locked before entering this function */
	assert(dimq);
	assert(message);
	assert(message->msg.qos != 0);

	if(dir == dimq_md_out){
		DL_APPEND(dimq->msgs_out.inflight, message);
		dimq->msgs_out.queue_len++;
	}else{
		DL_APPEND(dimq->msgs_in.inflight, message);
		dimq->msgs_in.queue_len++;
	}

	return message__release_to_inflight(dimq, dir);
}

void message__reconnect_reset(struct dimq *dimq, bool update_quota_only)
{
	struct dimq_message_all *message, *tmp;
	assert(dimq);

	pthread_mutex_lock(&dimq->msgs_in.mutex);
	dimq->msgs_in.inflight_quota = dimq->msgs_in.inflight_maximum;
	dimq->msgs_in.queue_len = 0;
	DL_FOREACH_SAFE(dimq->msgs_in.inflight, message, tmp){
		dimq->msgs_in.queue_len++;
		message->timestamp = 0;
		if(message->msg.qos != 2){
			DL_DELETE(dimq->msgs_in.inflight, message);
			message__cleanup(&message);
		}else{
			/* Message state can be preserved here because it should match
			* whatever the client has got. */
			util__decrement_receive_quota(dimq);
		}
	}
	pthread_mutex_unlock(&dimq->msgs_in.mutex);


	pthread_mutex_lock(&dimq->msgs_out.mutex);
	dimq->msgs_out.inflight_quota = dimq->msgs_out.inflight_maximum;
	dimq->msgs_out.queue_len = 0;
	DL_FOREACH_SAFE(dimq->msgs_out.inflight, message, tmp){
		dimq->msgs_out.queue_len++;

		message->timestamp = 0;
		if(dimq->msgs_out.inflight_quota != 0){
			util__decrement_send_quota(dimq);
			if (update_quota_only == false){
				if(message->msg.qos == 1){
					message->state = dimq_ms_publish_qos1;
				}else if(message->msg.qos == 2){
					if(message->state == dimq_ms_wait_for_pubrec){
						message->state = dimq_ms_publish_qos2;
					}else if(message->state == dimq_ms_wait_for_pubcomp){
						message->state = dimq_ms_resend_pubrel;
					}
					/* Should be able to preserve state. */
				}
			}
		}else{
			message->state = dimq_ms_invalid;
		}
	}
	pthread_mutex_unlock(&dimq->msgs_out.mutex);
}


int message__release_to_inflight(struct dimq *dimq, enum dimq_msg_direction dir)
{
	/* dimq->*_message_mutex should be locked before entering this function */
	struct dimq_message_all *cur, *tmp;
	int rc = dimq_ERR_SUCCESS;

	if(dir == dimq_md_out){
		DL_FOREACH_SAFE(dimq->msgs_out.inflight, cur, tmp){
			if(dimq->msgs_out.inflight_quota > 0){
				if(cur->msg.qos > 0 && cur->state == dimq_ms_invalid){
					if(cur->msg.qos == 1){
						cur->state = dimq_ms_wait_for_puback;
					}else if(cur->msg.qos == 2){
						cur->state = dimq_ms_wait_for_pubrec;
					}
					rc = send__publish(dimq, (uint16_t)cur->msg.mid, cur->msg.topic, (uint32_t)cur->msg.payloadlen, cur->msg.payload, (uint8_t)cur->msg.qos, cur->msg.retain, cur->dup, cur->properties, NULL, 0);
					if(rc){
						return rc;
					}
					util__decrement_send_quota(dimq);
				}
			}else{
				return dimq_ERR_SUCCESS;
			}
		}
	}

	return rc;
}


int message__remove(struct dimq *dimq, uint16_t mid, enum dimq_msg_direction dir, struct dimq_message_all **message, int qos)
{
	struct dimq_message_all *cur, *tmp;
	bool found = false;
	assert(dimq);
	assert(message);

	if(dir == dimq_md_out){
		pthread_mutex_lock(&dimq->msgs_out.mutex);

		DL_FOREACH_SAFE(dimq->msgs_out.inflight, cur, tmp){
			if(found == false && cur->msg.mid == mid){
				if(cur->msg.qos != qos){
					pthread_mutex_unlock(&dimq->msgs_out.mutex);
					return dimq_ERR_PROTOCOL;
				}
				DL_DELETE(dimq->msgs_out.inflight, cur);

				*message = cur;
				dimq->msgs_out.queue_len--;
				found = true;
				break;
			}
		}
		pthread_mutex_unlock(&dimq->msgs_out.mutex);
		if(found){
			return dimq_ERR_SUCCESS;
		}else{
			return dimq_ERR_NOT_FOUND;
		}
	}else{
		pthread_mutex_lock(&dimq->msgs_in.mutex);
		DL_FOREACH_SAFE(dimq->msgs_in.inflight, cur, tmp){
			if(cur->msg.mid == mid){
				if(cur->msg.qos != qos){
					pthread_mutex_unlock(&dimq->msgs_in.mutex);
					return dimq_ERR_PROTOCOL;
				}
				DL_DELETE(dimq->msgs_in.inflight, cur);
				*message = cur;
				dimq->msgs_in.queue_len--;
				found = true;
				break;
			}
		}

		pthread_mutex_unlock(&dimq->msgs_in.mutex);
		if(found){
			return dimq_ERR_SUCCESS;
		}else{
			return dimq_ERR_NOT_FOUND;
		}
	}
}

void message__retry_check(struct dimq *dimq)
{
	struct dimq_message_all *msg;
	time_t now = dimq_time();
	assert(dimq);

#ifdef WITH_THREADING
	pthread_mutex_lock(&dimq->msgs_out.mutex);
#endif

	DL_FOREACH(dimq->msgs_out.inflight, msg){
		switch(msg->state){
			case dimq_ms_publish_qos1:
			case dimq_ms_publish_qos2:
				msg->timestamp = now;
				msg->dup = true;
				send__publish(dimq, (uint16_t)msg->msg.mid, msg->msg.topic, (uint32_t)msg->msg.payloadlen, msg->msg.payload, (uint8_t)msg->msg.qos, msg->msg.retain, msg->dup, msg->properties, NULL, 0);
				break;
			case dimq_ms_wait_for_pubrel:
				msg->timestamp = now;
				msg->dup = true;
				send__pubrec(dimq, (uint16_t)msg->msg.mid, 0, NULL);
				break;
			case dimq_ms_resend_pubrel:
			case dimq_ms_wait_for_pubcomp:
				msg->timestamp = now;
				msg->dup = true;
				send__pubrel(dimq, (uint16_t)msg->msg.mid, NULL);
				break;
			default:
				break;
		}
	}
#ifdef WITH_THREADING
	pthread_mutex_unlock(&dimq->msgs_out.mutex);
#endif
}


void dimq_message_retry_set(struct dimq *dimq, unsigned int message_retry)
{
	UNUSED(dimq);
	UNUSED(message_retry);
}

int message__out_update(struct dimq *dimq, uint16_t mid, enum dimq_msg_state state, int qos)
{
	struct dimq_message_all *message, *tmp;
	assert(dimq);

	pthread_mutex_lock(&dimq->msgs_out.mutex);
	DL_FOREACH_SAFE(dimq->msgs_out.inflight, message, tmp){
		if(message->msg.mid == mid){
			if(message->msg.qos != qos){
				pthread_mutex_unlock(&dimq->msgs_out.mutex);
				return dimq_ERR_PROTOCOL;
			}
			message->state = state;
			message->timestamp = dimq_time();
			pthread_mutex_unlock(&dimq->msgs_out.mutex);
			return dimq_ERR_SUCCESS;
		}
	}
	pthread_mutex_unlock(&dimq->msgs_out.mutex);
	return dimq_ERR_NOT_FOUND;
}

int dimq_max_inflight_messages_set(struct dimq *dimq, unsigned int max_inflight_messages)
{
	return dimq_int_option(dimq, dimq_OPT_SEND_MAXIMUM, (int)max_inflight_messages);
}

