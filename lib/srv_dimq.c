/*
Copyright (c) 2013-2020 Roger Light <roger@atchoo.org>

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

#ifdef WITH_SRV
#  include <ares.h>

#  include <arpa/nameser.h>
#  include <stdio.h>
#  include <string.h>
#endif

#include "logging_dimq.h"
#include "memory_dimq.h"
#include "dimq_internal.h"
#include "dimq.h"
#include "util_dimq.h"

#ifdef WITH_SRV
static void srv_callback(void *arg, int status, int timeouts, unsigned char *abuf, int alen)
{   
	struct dimq *dimq = arg;
	struct ares_srv_reply *reply = NULL;

	UNUSED(timeouts);

	if(status == ARES_SUCCESS){
		status = ares_parse_srv_reply(abuf, alen, &reply);
		if(status == ARES_SUCCESS){
			// FIXME - choose which answer to use based on rfc2782 page 3. */
			dimq_connect(dimq, reply->host, reply->port, dimq->keepalive);
		}
	}else{
		log__printf(dimq, dimq_LOG_ERR, "Error: SRV lookup failed (%d).", status);
		/* FIXME - calling on_disconnect here isn't correct. */
		pthread_mutex_lock(&dimq->callback_mutex);
		if(dimq->on_disconnect){
			dimq->in_callback = true;
			dimq->on_disconnect(dimq, dimq->userdata, dimq_ERR_LOOKUP);
			dimq->in_callback = false;
		}
		if(dimq->on_disconnect_v5){
			dimq->in_callback = true;
			dimq->on_disconnect_v5(dimq, dimq->userdata, dimq_ERR_LOOKUP, NULL);
			dimq->in_callback = false;
		}
		pthread_mutex_unlock(&dimq->callback_mutex);
	}
}
#endif

int dimq_connect_srv(struct dimq *dimq, const char *host, int keepalive, const char *bind_address)
{
#ifdef WITH_SRV
	char *h;
	int rc;
	if(!dimq) return dimq_ERR_INVAL;

	UNUSED(bind_address);

	if(keepalive < 0 || keepalive > UINT16_MAX){
		return dimq_ERR_INVAL;
	}

	rc = ares_init(&dimq->achan);
	if(rc != ARES_SUCCESS){
		return dimq_ERR_UNKNOWN;
	}

	if(!host){
		// get local domain
	}else{
#ifdef WITH_TLS
		if(dimq->tls_cafile || dimq->tls_capath || dimq->tls_psk){
			h = dimq__malloc(strlen(host) + strlen("_secure-mqtt._tcp.") + 1);
			if(!h) return dimq_ERR_NOMEM;
			sprintf(h, "_secure-mqtt._tcp.%s", host);
		}else{
#endif
			h = dimq__malloc(strlen(host) + strlen("_mqtt._tcp.") + 1);
			if(!h) return dimq_ERR_NOMEM;
			sprintf(h, "_mqtt._tcp.%s", host);
#ifdef WITH_TLS
		}
#endif
		ares_search(dimq->achan, h, ns_c_in, ns_t_srv, srv_callback, dimq);
		dimq__free(h);
	}

	dimq__set_state(dimq, dimq_cs_connect_srv);

	dimq->keepalive = (uint16_t)keepalive;

	return dimq_ERR_SUCCESS;

#else
	UNUSED(dimq);
	UNUSED(host);
	UNUSED(keepalive);
	UNUSED(bind_address);

	return dimq_ERR_NOT_SUPPORTED;
#endif
}


