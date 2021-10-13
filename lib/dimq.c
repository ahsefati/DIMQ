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

#include <errno.h>
#include <signal.h>
#include <string.h>
#ifndef WIN32
#include <sys/time.h>
#include <strings.h>
#endif

#if defined(__APPLE__)
#  include <mach/mach_time.h>
#endif

#include "logging_dimq.h"
#include "dimq.h"
#include "dimq_internal.h"
#include "memory_dimq.h"
#include "messages_dimq.h"
#include "mqtt_protocol.h"
#include "net_dimq.h"
#include "packet_dimq.h"
#include "will_dimq.h"

static unsigned int init_refcount = 0;

void dimq__destroy(struct dimq *dimq);

int dimq_lib_version(int *major, int *minor, int *revision)
{
	if(major) *major = LIBdimq_MAJOR;
	if(minor) *minor = LIBdimq_MINOR;
	if(revision) *revision = LIBdimq_REVISION;
	return LIBdimq_VERSION_NUMBER;
}

int dimq_lib_init(void)
{
	int rc;

	if (init_refcount == 0) {
#ifdef WIN32
		srand((unsigned int)GetTickCount64());
#elif _POSIX_TIMERS>0 && defined(_POSIX_MONOTONIC_CLOCK)
		struct timespec tp;

		clock_gettime(CLOCK_MONOTONIC, &tp);
		srand((unsigned int)tp.tv_nsec);
#elif defined(__APPLE__)
		uint64_t ticks;

		ticks = mach_absolute_time();
		srand((unsigned int)ticks);
#else
		struct timeval tv;

		gettimeofday(&tv, NULL);
		srand(tv.tv_sec*1000 + tv.tv_usec/1000);
#endif

		rc = net__init();
		if (rc != dimq_ERR_SUCCESS) {
			return rc;
		}
	}

	init_refcount++;
	return dimq_ERR_SUCCESS;
}

int dimq_lib_cleanup(void)
{
	if (init_refcount == 1) {
		net__cleanup();
	}

	if (init_refcount > 0) {
		--init_refcount;
	}

	return dimq_ERR_SUCCESS;
}

struct dimq *dimq_new(const char *id, bool clean_start, void *userdata)
{
	struct dimq *dimq = NULL;
	int rc;

	if(clean_start == false && id == NULL){
		errno = EINVAL;
		return NULL;
	}

#ifndef WIN32
	signal(SIGPIPE, SIG_IGN);
#endif

	dimq = (struct dimq *)dimq__calloc(1, sizeof(struct dimq));
	if(dimq){
		dimq->sock = INVALID_SOCKET;
#ifdef WITH_THREADING
		dimq->thread_id = pthread_self();
#endif
		dimq->sockpairR = INVALID_SOCKET;
		dimq->sockpairW = INVALID_SOCKET;
		rc = dimq_reinitialise(dimq, id, clean_start, userdata);
		if(rc){
			dimq_destroy(dimq);
			if(rc == dimq_ERR_INVAL){
				errno = EINVAL;
			}else if(rc == dimq_ERR_NOMEM){
				errno = ENOMEM;
			}
			return NULL;
		}
	}else{
		errno = ENOMEM;
	}
	return dimq;
}

int dimq_reinitialise(struct dimq *dimq, const char *id, bool clean_start, void *userdata)
{
	if(!dimq) return dimq_ERR_INVAL;

	if(clean_start == false && id == NULL){
		return dimq_ERR_INVAL;
	}

	dimq__destroy(dimq);
	memset(dimq, 0, sizeof(struct dimq));

	if(userdata){
		dimq->userdata = userdata;
	}else{
		dimq->userdata = dimq;
	}
	dimq->protocol = dimq_p_mqtt311;
	dimq->sock = INVALID_SOCKET;
	dimq->keepalive = 60;
	dimq->clean_start = clean_start;
	if(id){
		if(STREMPTY(id)){
			return dimq_ERR_INVAL;
		}
		if(dimq_validate_utf8(id, (int)strlen(id))){
			return dimq_ERR_MALFORMED_UTF8;
		}
		dimq->id = dimq__strdup(id);
	}
	dimq->in_packet.payload = NULL;
	packet__cleanup(&dimq->in_packet);
	dimq->out_packet = NULL;
	dimq->out_packet_count = 0;
	dimq->current_out_packet = NULL;
	dimq->last_msg_in = dimq_time();
	dimq->next_msg_out = dimq_time() + dimq->keepalive;
	dimq->ping_t = 0;
	dimq->last_mid = 0;
	dimq->state = dimq_cs_new;
	dimq->max_qos = 2;
	dimq->msgs_in.inflight_maximum = 20;
	dimq->msgs_out.inflight_maximum = 20;
	dimq->msgs_in.inflight_quota = 20;
	dimq->msgs_out.inflight_quota = 20;
	dimq->will = NULL;
	dimq->on_connect = NULL;
	dimq->on_publish = NULL;
	dimq->on_message = NULL;
	dimq->on_subscribe = NULL;
	dimq->on_unsubscribe = NULL;
	dimq->host = NULL;
	dimq->port = 1883;
	dimq->in_callback = false;
	dimq->reconnect_delay = 1;
	dimq->reconnect_delay_max = 1;
	dimq->reconnect_exponential_backoff = false;
	dimq->threaded = dimq_ts_none;
#ifdef WITH_TLS
	dimq->ssl = NULL;
	dimq->ssl_ctx = NULL;
	dimq->ssl_ctx_defaults = true;
	dimq->tls_cert_reqs = SSL_VERIFY_PEER;
	dimq->tls_insecure = false;
	dimq->want_write = false;
	dimq->tls_ocsp_required = false;
#endif
#ifdef WITH_THREADING
	pthread_mutex_init(&dimq->callback_mutex, NULL);
	pthread_mutex_init(&dimq->log_callback_mutex, NULL);
	pthread_mutex_init(&dimq->state_mutex, NULL);
	pthread_mutex_init(&dimq->out_packet_mutex, NULL);
	pthread_mutex_init(&dimq->current_out_packet_mutex, NULL);
	pthread_mutex_init(&dimq->msgtime_mutex, NULL);
	pthread_mutex_init(&dimq->msgs_in.mutex, NULL);
	pthread_mutex_init(&dimq->msgs_out.mutex, NULL);
	pthread_mutex_init(&dimq->mid_mutex, NULL);
	dimq->thread_id = pthread_self();
#endif
	/* This must be after pthread_mutex_init(), otherwise the log mutex may be
	 * used before being initialised. */
	if(net__socketpair(&dimq->sockpairR, &dimq->sockpairW)){
		log__printf(dimq, dimq_LOG_WARNING,
				"Warning: Unable to open socket pair, outgoing publish commands may be delayed.");
	}

	return dimq_ERR_SUCCESS;
}


void dimq__destroy(struct dimq *dimq)
{
	if(!dimq) return;

#ifdef WITH_THREADING
#  ifdef HAVE_PTHREAD_CANCEL
	if(dimq->threaded == dimq_ts_self && !pthread_equal(dimq->thread_id, pthread_self())){
		pthread_cancel(dimq->thread_id);
		pthread_join(dimq->thread_id, NULL);
		dimq->threaded = dimq_ts_none;
	}
#  endif

	if(dimq->id){
		/* If dimq->id is not NULL then the client has already been initialised
		 * and so the mutexes need destroying. If dimq->id is NULL, the mutexes
		 * haven't been initialised. */
		pthread_mutex_destroy(&dimq->callback_mutex);
		pthread_mutex_destroy(&dimq->log_callback_mutex);
		pthread_mutex_destroy(&dimq->state_mutex);
		pthread_mutex_destroy(&dimq->out_packet_mutex);
		pthread_mutex_destroy(&dimq->current_out_packet_mutex);
		pthread_mutex_destroy(&dimq->msgtime_mutex);
		pthread_mutex_destroy(&dimq->msgs_in.mutex);
		pthread_mutex_destroy(&dimq->msgs_out.mutex);
		pthread_mutex_destroy(&dimq->mid_mutex);
	}
#endif
	if(dimq->sock != INVALID_SOCKET){
		net__socket_close(dimq);
	}
	message__cleanup_all(dimq);
	will__clear(dimq);
#ifdef WITH_TLS
	if(dimq->ssl){
		SSL_free(dimq->ssl);
	}
	if(dimq->ssl_ctx){
		SSL_CTX_free(dimq->ssl_ctx);
	}
	dimq__free(dimq->tls_cafile);
	dimq__free(dimq->tls_capath);
	dimq__free(dimq->tls_certfile);
	dimq__free(dimq->tls_keyfile);
	if(dimq->tls_pw_callback) dimq->tls_pw_callback = NULL;
	dimq__free(dimq->tls_version);
	dimq__free(dimq->tls_ciphers);
	dimq__free(dimq->tls_psk);
	dimq__free(dimq->tls_psk_identity);
	dimq__free(dimq->tls_alpn);
#endif

	dimq__free(dimq->address);
	dimq->address = NULL;

	dimq__free(dimq->id);
	dimq->id = NULL;

	dimq__free(dimq->username);
	dimq->username = NULL;

	dimq__free(dimq->password);
	dimq->password = NULL;

	dimq__free(dimq->host);
	dimq->host = NULL;

	dimq__free(dimq->bind_address);
	dimq->bind_address = NULL;

	dimq_property_free_all(&dimq->connect_properties);

	packet__cleanup_all_no_locks(dimq);

	packet__cleanup(&dimq->in_packet);
	if(dimq->sockpairR != INVALID_SOCKET){
		COMPAT_CLOSE(dimq->sockpairR);
		dimq->sockpairR = INVALID_SOCKET;
	}
	if(dimq->sockpairW != INVALID_SOCKET){
		COMPAT_CLOSE(dimq->sockpairW);
		dimq->sockpairW = INVALID_SOCKET;
	}
}

void dimq_destroy(struct dimq *dimq)
{
	if(!dimq) return;

	dimq__destroy(dimq);
	dimq__free(dimq);
}

int dimq_socket(struct dimq *dimq)
{
	if(!dimq) return INVALID_SOCKET;
	return dimq->sock;
}


bool dimq_want_write(struct dimq *dimq)
{
	bool result = false;
	if(dimq->out_packet || dimq->current_out_packet){
		result = true;
	}
#ifdef WITH_TLS
	if(dimq->ssl){
		if (dimq->want_write) {
			result = true;
		}else if(dimq->want_connect){
			result = false;
		}
	}
#endif
	return result;
}


int dimq_sub_topic_tokenise(const char *subtopic, char ***topics, int *count)
{
	size_t len;
	size_t hier_count = 1;
	size_t start, stop;
	size_t hier;
	size_t tlen;
	size_t i, j;

	if(!subtopic || !topics || !count) return dimq_ERR_INVAL;

	len = strlen(subtopic);

	for(i=0; i<len; i++){
		if(subtopic[i] == '/'){
			if(i > len-1){
				/* Separator at end of line */
			}else{
				hier_count++;
			}
		}
	}

	(*topics) = dimq__calloc(hier_count, sizeof(char *));
	if(!(*topics)) return dimq_ERR_NOMEM;

	start = 0;
	hier = 0;

	for(i=0; i<len+1; i++){
		if(subtopic[i] == '/' || subtopic[i] == '\0'){
			stop = i;
			if(start != stop){
				tlen = stop-start + 1;
				(*topics)[hier] = dimq__calloc(tlen, sizeof(char));
				if(!(*topics)[hier]){
					for(j=0; j<hier; j++){
						dimq__free((*topics)[j]);
					}
					dimq__free((*topics));
					return dimq_ERR_NOMEM;
				}
				for(j=start; j<stop; j++){
					(*topics)[hier][j-start] = subtopic[j];
				}
			}
			start = i+1;
			hier++;
		}
	}

	*count = (int)hier_count;

	return dimq_ERR_SUCCESS;
}

int dimq_sub_topic_tokens_free(char ***topics, int count)
{
	int i;

	if(!topics || !(*topics) || count<1) return dimq_ERR_INVAL;

	for(i=0; i<count; i++){
		dimq__free((*topics)[i]);
	}
	dimq__free(*topics);

	return dimq_ERR_SUCCESS;
}

