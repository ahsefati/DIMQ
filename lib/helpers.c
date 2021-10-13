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

#include <errno.h>
#include <stdbool.h>

#include "dimq.h"
#include "dimq_internal.h"

struct userdata__callback {
	const char *topic;
	int (*callback)(struct dimq *, void *, const struct dimq_message *);
	void *userdata;
	int qos;
};

struct userdata__simple {
	struct dimq_message *messages;
	int max_msg_count;
	int message_count;
	bool want_retained;
};


static void on_connect(struct dimq *dimq, void *obj, int rc)
{
	struct userdata__callback *userdata = obj;

	UNUSED(rc);

	dimq_subscribe(dimq, NULL, userdata->topic, userdata->qos);
}


static void on_message_callback(struct dimq *dimq, void *obj, const struct dimq_message *message)
{
	int rc;
	struct userdata__callback *userdata = obj;

	rc = userdata->callback(dimq, userdata->userdata, message);
	if(rc){
		dimq_disconnect(dimq);
	}
}

static int on_message_simple(struct dimq *dimq, void *obj, const struct dimq_message *message)
{
	struct userdata__simple *userdata = obj;
	int rc;

	if(userdata->max_msg_count == 0){
		return 0;
	}

	/* Don't process stale retained messages if 'want_retained' was false */
	if(!userdata->want_retained && message->retain){
		return 0;
	}

	userdata->max_msg_count--;

	rc = dimq_message_copy(&userdata->messages[userdata->message_count], message);
	if(rc){
		return rc;
	}
	userdata->message_count++;
	if(userdata->max_msg_count == 0){
		dimq_disconnect(dimq);
	}
	return 0;
}


libdimq_EXPORT int dimq_subscribe_simple(
		struct dimq_message **messages,
		int msg_count,
		bool want_retained,
		const char *topic,
		int qos,
		const char *host,
		int port,
		const char *client_id,
		int keepalive,
		bool clean_session,
		const char *username,
		const char *password,
		const struct libdimq_will *will,
		const struct libdimq_tls *tls)
{
	struct userdata__simple userdata;
	int rc;
	int i;

	if(!topic || msg_count < 1 || !messages){
		return dimq_ERR_INVAL;
	}

	*messages = NULL;

	userdata.messages = calloc(sizeof(struct dimq_message), (size_t)msg_count);
	if(!userdata.messages){
		return dimq_ERR_NOMEM;
	}
	userdata.message_count = 0;
	userdata.max_msg_count = msg_count;
	userdata.want_retained = want_retained;

	rc = dimq_subscribe_callback(
			on_message_simple, &userdata,
			topic, qos,
			host, port,
			client_id, keepalive, clean_session,
			username, password,
			will, tls);

	if(!rc && userdata.max_msg_count == 0){
		*messages = userdata.messages;
		return dimq_ERR_SUCCESS;
	}else{
		for(i=0; i<msg_count; i++){
			dimq_message_free_contents(&userdata.messages[i]);
		}
		free(userdata.messages);
		userdata.messages = NULL;
		return rc;
	}
}


libdimq_EXPORT int dimq_subscribe_callback(
		int (*callback)(struct dimq *, void *, const struct dimq_message *),
		void *userdata,
		const char *topic,
		int qos,
		const char *host,
		int port,
		const char *client_id,
		int keepalive,
		bool clean_session,
		const char *username,
		const char *password,
		const struct libdimq_will *will,
		const struct libdimq_tls *tls)
{
	struct dimq *dimq;
	struct userdata__callback cb_userdata;
	int rc;

	if(!callback || !topic){
		return dimq_ERR_INVAL;
	}

	cb_userdata.topic = topic;
	cb_userdata.qos = qos;
	cb_userdata.userdata = userdata;
	cb_userdata.callback = callback;

	dimq = dimq_new(client_id, clean_session, &cb_userdata);
	if(!dimq){
		return dimq_ERR_NOMEM;
	}

	if(will){
		rc = dimq_will_set(dimq, will->topic, will->payloadlen, will->payload, will->qos, will->retain);
		if(rc){
			dimq_destroy(dimq);
			return rc;
		}
	}
	if(username){
		rc = dimq_username_pw_set(dimq, username, password);
		if(rc){
			dimq_destroy(dimq);
			return rc;
		}
	}
	if(tls){
		rc = dimq_tls_set(dimq, tls->cafile, tls->capath, tls->certfile, tls->keyfile, tls->pw_callback);
		if(rc){
			dimq_destroy(dimq);
			return rc;
		}
		rc = dimq_tls_opts_set(dimq, tls->cert_reqs, tls->tls_version, tls->ciphers);
		if(rc){
			dimq_destroy(dimq);
			return rc;
		}
	}

	dimq_connect_callback_set(dimq, on_connect);
	dimq_message_callback_set(dimq, on_message_callback);

	rc = dimq_connect(dimq, host, port, keepalive);
	if(rc){
		dimq_destroy(dimq);
		return rc;
	}
	rc = dimq_loop_forever(dimq, -1, 1);
	dimq_destroy(dimq);
	return rc;
}

