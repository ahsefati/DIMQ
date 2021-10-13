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

#include "dimq.h"
#include "dimq_internal.h"


void dimq_connect_callback_set(struct dimq *dimq, void (*on_connect)(struct dimq *, void *, int))
{
	pthread_mutex_lock(&dimq->callback_mutex);
	dimq->on_connect = on_connect;
	pthread_mutex_unlock(&dimq->callback_mutex);
}

void dimq_connect_with_flags_callback_set(struct dimq *dimq, void (*on_connect)(struct dimq *, void *, int, int))
{
	pthread_mutex_lock(&dimq->callback_mutex);
	dimq->on_connect_with_flags = on_connect;
	pthread_mutex_unlock(&dimq->callback_mutex);
}

void dimq_connect_v5_callback_set(struct dimq *dimq, void (*on_connect)(struct dimq *, void *, int, int, const dimq_property *))
{
	pthread_mutex_lock(&dimq->callback_mutex);
	dimq->on_connect_v5 = on_connect;
	pthread_mutex_unlock(&dimq->callback_mutex);
}

void dimq_disconnect_callback_set(struct dimq *dimq, void (*on_disconnect)(struct dimq *, void *, int))
{
	pthread_mutex_lock(&dimq->callback_mutex);
	dimq->on_disconnect = on_disconnect;
	pthread_mutex_unlock(&dimq->callback_mutex);
}

void dimq_disconnect_v5_callback_set(struct dimq *dimq, void (*on_disconnect)(struct dimq *, void *, int, const dimq_property *))
{
	pthread_mutex_lock(&dimq->callback_mutex);
	dimq->on_disconnect_v5 = on_disconnect;
	pthread_mutex_unlock(&dimq->callback_mutex);
}

void dimq_publish_callback_set(struct dimq *dimq, void (*on_publish)(struct dimq *, void *, int))
{
	pthread_mutex_lock(&dimq->callback_mutex);
	dimq->on_publish = on_publish;
	pthread_mutex_unlock(&dimq->callback_mutex);
}

void dimq_publish_v5_callback_set(struct dimq *dimq, void (*on_publish)(struct dimq *, void *, int, int, const dimq_property *props))
{
	pthread_mutex_lock(&dimq->callback_mutex);
	dimq->on_publish_v5 = on_publish;
	pthread_mutex_unlock(&dimq->callback_mutex);
}

void dimq_message_callback_set(struct dimq *dimq, void (*on_message)(struct dimq *, void *, const struct dimq_message *))
{
	pthread_mutex_lock(&dimq->callback_mutex);
	dimq->on_message = on_message;
	pthread_mutex_unlock(&dimq->callback_mutex);
}

void dimq_message_v5_callback_set(struct dimq *dimq, void (*on_message)(struct dimq *, void *, const struct dimq_message *, const dimq_property *props))
{
	pthread_mutex_lock(&dimq->callback_mutex);
	dimq->on_message_v5 = on_message;
	pthread_mutex_unlock(&dimq->callback_mutex);
}

void dimq_subscribe_callback_set(struct dimq *dimq, void (*on_subscribe)(struct dimq *, void *, int, int, const int *))
{
	pthread_mutex_lock(&dimq->callback_mutex);
	dimq->on_subscribe = on_subscribe;
	pthread_mutex_unlock(&dimq->callback_mutex);
}

void dimq_subscribe_v5_callback_set(struct dimq *dimq, void (*on_subscribe)(struct dimq *, void *, int, int, const int *, const dimq_property *props))
{
	pthread_mutex_lock(&dimq->callback_mutex);
	dimq->on_subscribe_v5 = on_subscribe;
	pthread_mutex_unlock(&dimq->callback_mutex);
}

void dimq_unsubscribe_callback_set(struct dimq *dimq, void (*on_unsubscribe)(struct dimq *, void *, int))
{
	pthread_mutex_lock(&dimq->callback_mutex);
	dimq->on_unsubscribe = on_unsubscribe;
	pthread_mutex_unlock(&dimq->callback_mutex);
}

void dimq_unsubscribe_v5_callback_set(struct dimq *dimq, void (*on_unsubscribe)(struct dimq *, void *, int, const dimq_property *props))
{
	pthread_mutex_lock(&dimq->callback_mutex);
	dimq->on_unsubscribe_v5 = on_unsubscribe;
	pthread_mutex_unlock(&dimq->callback_mutex);
}

void dimq_log_callback_set(struct dimq *dimq, void (*on_log)(struct dimq *, void *, int, const char *))
{
	pthread_mutex_lock(&dimq->log_callback_mutex);
	dimq->on_log = on_log;
	pthread_mutex_unlock(&dimq->log_callback_mutex);
}

