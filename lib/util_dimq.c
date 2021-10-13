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
#include <ctype.h>
#include <string.h>

#ifdef WIN32
#  include <winsock2.h>
#  include <aclapi.h>
#  include <io.h>
#  include <lmcons.h>
#else
#  include <sys/stat.h>
#endif

#if !defined(WITH_TLS) && defined(__linux__) && defined(__GLIBC__)
#  if __GLIBC_PREREQ(2, 25)
#    include <sys/random.h>
#    define HAVE_GETRANDOM 1
#  endif
#endif

#ifdef WITH_TLS
#  include <openssl/bn.h>
#  include <openssl/rand.h>
#endif

#ifdef WITH_BROKER
#include "dimq_broker_internal.h"
#endif

#include "dimq.h"
#include "memory_dimq.h"
#include "net_dimq.h"
#include "send_dimq.h"
#include "time_dimq.h"
#include "tls_dimq.h"
#include "util_dimq.h"

#ifdef WITH_WEBSOCKETS
#include <libwebsockets.h>
#endif

int dimq__check_keepalive(struct dimq *dimq)
{
	time_t next_msg_out;
	time_t last_msg_in;
	time_t now;
#ifndef WITH_BROKER
	int rc;
#endif
	enum dimq_client_state state;

	assert(dimq);
#ifdef WITH_BROKER
	now = db.now_s;
#else
	now = dimq_time();
#endif

#if defined(WITH_BROKER) && defined(WITH_BRIDGE)
	/* Check if a lazy bridge should be timed out due to idle. */
	if(dimq->bridge && dimq->bridge->start_type == bst_lazy
				&& dimq->sock != INVALID_SOCKET
				&& now - dimq->next_msg_out - dimq->keepalive >= dimq->bridge->idle_timeout){

		log__printf(NULL, dimq_LOG_NOTICE, "Bridge connection %s has exceeded idle timeout, disconnecting.", dimq->id);
		net__socket_close(dimq);
		return dimq_ERR_SUCCESS;
	}
#endif
	pthread_mutex_lock(&dimq->msgtime_mutex);
	next_msg_out = dimq->next_msg_out;
	last_msg_in = dimq->last_msg_in;
	pthread_mutex_unlock(&dimq->msgtime_mutex);
	if(dimq->keepalive && dimq->sock != INVALID_SOCKET &&
			(now >= next_msg_out || now - last_msg_in >= dimq->keepalive)){

		state = dimq__get_state(dimq);
		if(state == dimq_cs_active && dimq->ping_t == 0){
			send__pingreq(dimq);
			/* Reset last msg times to give the server time to send a pingresp */
			pthread_mutex_lock(&dimq->msgtime_mutex);
			dimq->last_msg_in = now;
			dimq->next_msg_out = now + dimq->keepalive;
			pthread_mutex_unlock(&dimq->msgtime_mutex);
		}else{
#ifdef WITH_BROKER
			net__socket_close(dimq);
#else
			net__socket_close(dimq);
			state = dimq__get_state(dimq);
			if(state == dimq_cs_disconnecting){
				rc = dimq_ERR_SUCCESS;
			}else{
				rc = dimq_ERR_KEEPALIVE;
			}
			pthread_mutex_lock(&dimq->callback_mutex);
			if(dimq->on_disconnect){
				dimq->in_callback = true;
				dimq->on_disconnect(dimq, dimq->userdata, rc);
				dimq->in_callback = false;
			}
			if(dimq->on_disconnect_v5){
				dimq->in_callback = true;
				dimq->on_disconnect_v5(dimq, dimq->userdata, rc, NULL);
				dimq->in_callback = false;
			}
			pthread_mutex_unlock(&dimq->callback_mutex);

			return rc;
#endif
		}
	}
	return dimq_ERR_SUCCESS;
}

uint16_t dimq__mid_generate(struct dimq *dimq)
{
	/* FIXME - this would be better with atomic increment, but this is safer
	 * for now for a bug fix release.
	 *
	 * If this is changed to use atomic increment, callers of this function
	 * will have to be aware that they may receive a 0 result, which may not be
	 * used as a mid.
	 */
	uint16_t mid;
	assert(dimq);

	pthread_mutex_lock(&dimq->mid_mutex);
	dimq->last_mid++;
	if(dimq->last_mid == 0) dimq->last_mid++;
	mid = dimq->last_mid;
	pthread_mutex_unlock(&dimq->mid_mutex);

	return mid;
}


#ifdef WITH_TLS
int dimq__hex2bin_sha1(const char *hex, unsigned char **bin)
{
	unsigned char *sha, tmp[SHA_DIGEST_LENGTH];

	if(dimq__hex2bin(hex, tmp, SHA_DIGEST_LENGTH) != SHA_DIGEST_LENGTH){
		return dimq_ERR_INVAL;
	}

	sha = dimq__malloc(SHA_DIGEST_LENGTH);
	if(!sha){
		return dimq_ERR_NOMEM;
	}
	memcpy(sha, tmp, SHA_DIGEST_LENGTH);
	*bin = sha;
	return dimq_ERR_SUCCESS;
}

int dimq__hex2bin(const char *hex, unsigned char *bin, int bin_max_len)
{
	BIGNUM *bn = NULL;
	int len;
	int leading_zero = 0;
	int start = 0;
	size_t i = 0;

	/* Count the number of leading zero */
	for(i=0; i<strlen(hex); i=i+2) {
		if(strncmp(hex + i, "00", 2) == 0) {
			leading_zero++;
			/* output leading zero to bin */
			bin[start++] = 0;
		}else{
			break;
		}
	}

	if(BN_hex2bn(&bn, hex) == 0){
		if(bn) BN_free(bn);
		return 0;
	}
	if(BN_num_bytes(bn) + leading_zero > bin_max_len){
		BN_free(bn);
		return 0;
	}

	len = BN_bn2bin(bn, bin + leading_zero);
	BN_free(bn);
	return len + leading_zero;
}
#endif

void util__increment_receive_quota(struct dimq *dimq)
{
	if(dimq->msgs_in.inflight_quota < dimq->msgs_in.inflight_maximum){
		dimq->msgs_in.inflight_quota++;
	}
}

void util__increment_send_quota(struct dimq *dimq)
{
	if(dimq->msgs_out.inflight_quota < dimq->msgs_out.inflight_maximum){
		dimq->msgs_out.inflight_quota++;
	}
}


void util__decrement_receive_quota(struct dimq *dimq)
{
	if(dimq->msgs_in.inflight_quota > 0){
		dimq->msgs_in.inflight_quota--;
	}
}

void util__decrement_send_quota(struct dimq *dimq)
{
	if(dimq->msgs_out.inflight_quota > 0){
		dimq->msgs_out.inflight_quota--;
	}
}


int util__random_bytes(void *bytes, int count)
{
	int rc = dimq_ERR_UNKNOWN;

#ifdef WITH_TLS
	if(RAND_bytes(bytes, count) == 1){
		rc = dimq_ERR_SUCCESS;
	}
#elif defined(HAVE_GETRANDOM)
	if(getrandom(bytes, (size_t)count, 0) == count){
		rc = dimq_ERR_SUCCESS;
	}
#elif defined(WIN32)
	HCRYPTPROV provider;

	if(!CryptAcquireContext(&provider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)){
		return dimq_ERR_UNKNOWN;
	}

	if(CryptGenRandom(provider, count, bytes)){
		rc = dimq_ERR_SUCCESS;
	}

	CryptReleaseContext(provider, 0);
#else
	int i;

	for(i=0; i<count; i++){
		((uint8_t *)bytes)[i] = (uint8_t )(random()&0xFF);
	}
	rc = dimq_ERR_SUCCESS;
#endif
	return rc;
}


int dimq__set_state(struct dimq *dimq, enum dimq_client_state state)
{
	pthread_mutex_lock(&dimq->state_mutex);
#ifdef WITH_BROKER
	if(dimq->state != dimq_cs_disused)
#endif
	{
		dimq->state = state;
	}
	pthread_mutex_unlock(&dimq->state_mutex);

	return dimq_ERR_SUCCESS;
}

enum dimq_client_state dimq__get_state(struct dimq *dimq)
{
	enum dimq_client_state state;

	pthread_mutex_lock(&dimq->state_mutex);
	state = dimq->state;
	pthread_mutex_unlock(&dimq->state_mutex);

	return state;
}
