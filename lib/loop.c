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
#ifndef WIN32
#include <sys/select.h>
#include <time.h>
#endif

#include "dimq.h"
#include "dimq_internal.h"
#include "net_dimq.h"
#include "packet_dimq.h"
#include "socks_dimq.h"
#include "tls_dimq.h"
#include "util_dimq.h"

#if !defined(WIN32) && !defined(__SYMBIAN32__) && !defined(__QNX__)
#define HAVE_PSELECT
#endif

int dimq_loop(struct dimq *dimq, int timeout, int max_packets)
{
#ifdef HAVE_PSELECT
	struct timespec local_timeout;
#else
	struct timeval local_timeout;
#endif
	fd_set readfds, writefds;
	int fdcount;
	int rc;
	char pairbuf;
	int maxfd = 0;
	time_t now;
	time_t timeout_ms;

	if(!dimq || max_packets < 1) return dimq_ERR_INVAL;
#ifndef WIN32
	if(dimq->sock >= FD_SETSIZE || dimq->sockpairR >= FD_SETSIZE){
		return dimq_ERR_INVAL;
	}
#endif

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	if(dimq->sock != INVALID_SOCKET){
		maxfd = dimq->sock;
		FD_SET(dimq->sock, &readfds);
		pthread_mutex_lock(&dimq->current_out_packet_mutex);
		pthread_mutex_lock(&dimq->out_packet_mutex);
		if(dimq->out_packet || dimq->current_out_packet){
			FD_SET(dimq->sock, &writefds);
		}
#ifdef WITH_TLS
		if(dimq->ssl){
			if(dimq->want_write){
				FD_SET(dimq->sock, &writefds);
			}else if(dimq->want_connect){
				/* Remove possible FD_SET from above, we don't want to check
				 * for writing if we are still connecting, unless want_write is
				 * definitely set. The presence of outgoing packets does not
				 * matter yet. */
				FD_CLR(dimq->sock, &writefds);
			}
		}
#endif
		pthread_mutex_unlock(&dimq->out_packet_mutex);
		pthread_mutex_unlock(&dimq->current_out_packet_mutex);
	}else{
#ifdef WITH_SRV
		if(dimq->achan){
			if(dimq__get_state(dimq) == dimq_cs_connect_srv){
				rc = ares_fds(dimq->achan, &readfds, &writefds);
				if(rc > maxfd){
					maxfd = rc;
				}
			}else{
				return dimq_ERR_NO_CONN;
			}
		}
#else
		return dimq_ERR_NO_CONN;
#endif
	}
	if(dimq->sockpairR != INVALID_SOCKET){
		/* sockpairR is used to break out of select() before the timeout, on a
		 * call to publish() etc. */
		FD_SET(dimq->sockpairR, &readfds);
		if((int)dimq->sockpairR > maxfd){
			maxfd = dimq->sockpairR;
		}
	}

	timeout_ms = timeout;
	if(timeout_ms < 0){
		timeout_ms = 1000;
	}

	now = dimq_time();
	if(dimq->next_msg_out && now + timeout_ms/1000 > dimq->next_msg_out){
		timeout_ms = (dimq->next_msg_out - now)*1000;
	}

	if(timeout_ms < 0){
		/* There has been a delay somewhere which means we should have already
		 * sent a message. */
		timeout_ms = 0;
	}

	local_timeout.tv_sec = timeout_ms/1000;
#ifdef HAVE_PSELECT
	local_timeout.tv_nsec = (timeout_ms-local_timeout.tv_sec*1000)*1000000;
#else
	local_timeout.tv_usec = (timeout_ms-local_timeout.tv_sec*1000)*1000;
#endif

#ifdef HAVE_PSELECT
	fdcount = pselect(maxfd+1, &readfds, &writefds, NULL, &local_timeout, NULL);
#else
	fdcount = select(maxfd+1, &readfds, &writefds, NULL, &local_timeout);
#endif
	if(fdcount == -1){
#ifdef WIN32
		errno = WSAGetLastError();
#endif
		if(errno == EINTR){
			return dimq_ERR_SUCCESS;
		}else{
			return dimq_ERR_ERRNO;
		}
	}else{
		if(dimq->sock != INVALID_SOCKET){
			if(FD_ISSET(dimq->sock, &readfds)){
				rc = dimq_loop_read(dimq, max_packets);
				if(rc || dimq->sock == INVALID_SOCKET){
					return rc;
				}
			}
			if(dimq->sockpairR != INVALID_SOCKET && FD_ISSET(dimq->sockpairR, &readfds)){
#ifndef WIN32
				if(read(dimq->sockpairR, &pairbuf, 1) == 0){
				}
#else
				recv(dimq->sockpairR, &pairbuf, 1, 0);
#endif
				/* Fake write possible, to stimulate output write even though
				 * we didn't ask for it, because at that point the publish or
				 * other command wasn't present. */
				if(dimq->sock != INVALID_SOCKET)
					FD_SET(dimq->sock, &writefds);
			}
			if(dimq->sock != INVALID_SOCKET && FD_ISSET(dimq->sock, &writefds)){
#ifdef WITH_TLS
				if(dimq->want_connect){
					rc = net__socket_connect_tls(dimq);
					if(rc) return rc;
				}else
#endif
				{
					rc = dimq_loop_write(dimq, max_packets);
					if(rc || dimq->sock == INVALID_SOCKET){
						return rc;
					}
				}
			}
		}
#ifdef WITH_SRV
		if(dimq->achan){
			ares_process(dimq->achan, &readfds, &writefds);
		}
#endif
	}
	return dimq_loop_misc(dimq);
}


static int interruptible_sleep(struct dimq *dimq, time_t reconnect_delay)
{
#ifdef HAVE_PSELECT
	struct timespec local_timeout;
#else
	struct timeval local_timeout;
#endif
	fd_set readfds;
	int fdcount;
	char pairbuf;
	int maxfd = 0;

#ifndef WIN32
	while(dimq->sockpairR != INVALID_SOCKET && read(dimq->sockpairR, &pairbuf, 1) > 0);
#else
	while(dimq->sockpairR != INVALID_SOCKET && recv(dimq->sockpairR, &pairbuf, 1, 0) > 0);
#endif

	local_timeout.tv_sec = reconnect_delay;
#ifdef HAVE_PSELECT
	local_timeout.tv_nsec = 0;
#else
	local_timeout.tv_usec = 0;
#endif
	FD_ZERO(&readfds);
	maxfd = 0;
	if(dimq->sockpairR != INVALID_SOCKET){
		/* sockpairR is used to break out of select() before the
		 * timeout, when dimq_loop_stop() is called */
		FD_SET(dimq->sockpairR, &readfds);
		maxfd = dimq->sockpairR;
	}
#ifdef HAVE_PSELECT
	fdcount = pselect(maxfd+1, &readfds, NULL, NULL, &local_timeout, NULL);
#else
	fdcount = select(maxfd+1, &readfds, NULL, NULL, &local_timeout);
#endif
	if(fdcount == -1){
#ifdef WIN32
		errno = WSAGetLastError();
#endif
		if(errno == EINTR){
			return dimq_ERR_SUCCESS;
		}else{
			return dimq_ERR_ERRNO;
		}
	}else if(dimq->sockpairR != INVALID_SOCKET && FD_ISSET(dimq->sockpairR, &readfds)){
#ifndef WIN32
		if(read(dimq->sockpairR, &pairbuf, 1) == 0){
		}
#else
		recv(dimq->sockpairR, &pairbuf, 1, 0);
#endif
	}
	return dimq_ERR_SUCCESS;
}


int dimq_loop_forever(struct dimq *dimq, int timeout, int max_packets)
{
	int run = 1;
	int rc = dimq_ERR_SUCCESS;
	unsigned long reconnect_delay;
	enum dimq_client_state state;

	if(!dimq) return dimq_ERR_INVAL;

	dimq->reconnects = 0;

	while(run){
		do{
#ifdef HAVE_PTHREAD_CANCEL
			pthread_testcancel();
#endif
			rc = dimq_loop(dimq, timeout, max_packets);
		}while(run && rc == dimq_ERR_SUCCESS);
		/* Quit after fatal errors. */
		switch(rc){
			case dimq_ERR_NOMEM:
			case dimq_ERR_PROTOCOL:
			case dimq_ERR_INVAL:
			case dimq_ERR_NOT_FOUND:
			case dimq_ERR_TLS:
			case dimq_ERR_PAYLOAD_SIZE:
			case dimq_ERR_NOT_SUPPORTED:
			case dimq_ERR_AUTH:
			case dimq_ERR_ACL_DENIED:
			case dimq_ERR_UNKNOWN:
			case dimq_ERR_EAI:
			case dimq_ERR_PROXY:
				return rc;
			case dimq_ERR_ERRNO:
				break;
		}
		if(errno == EPROTO){
			return rc;
		}
		do{
#ifdef HAVE_PTHREAD_CANCEL
			pthread_testcancel();
#endif
			rc = dimq_ERR_SUCCESS;
			state = dimq__get_state(dimq);
			if(state == dimq_cs_disconnecting || state == dimq_cs_disconnected){
				run = 0;
			}else{
				if(dimq->reconnect_delay_max > dimq->reconnect_delay){
					if(dimq->reconnect_exponential_backoff){
						reconnect_delay = dimq->reconnect_delay*(dimq->reconnects+1)*(dimq->reconnects+1);
					}else{
						reconnect_delay = dimq->reconnect_delay*(dimq->reconnects+1);
					}
				}else{
					reconnect_delay = dimq->reconnect_delay;
				}

				if(reconnect_delay > dimq->reconnect_delay_max){
					reconnect_delay = dimq->reconnect_delay_max;
				}else{
					dimq->reconnects++;
				}

				rc = interruptible_sleep(dimq, (time_t)reconnect_delay);
				if(rc) return rc;

				state = dimq__get_state(dimq);
				if(state == dimq_cs_disconnecting || state == dimq_cs_disconnected){
					run = 0;
				}else{
					rc = dimq_reconnect(dimq);
				}
			}
		}while(run && rc != dimq_ERR_SUCCESS);
	}
	return rc;
}


int dimq_loop_misc(struct dimq *dimq)
{
	if(!dimq) return dimq_ERR_INVAL;
	if(dimq->sock == INVALID_SOCKET) return dimq_ERR_NO_CONN;

	return dimq__check_keepalive(dimq);
}


static int dimq__loop_rc_handle(struct dimq *dimq, int rc)
{
	enum dimq_client_state state;

	if(rc){
		net__socket_close(dimq);
		state = dimq__get_state(dimq);
		if(state == dimq_cs_disconnecting || state == dimq_cs_disconnected){
			rc = dimq_ERR_SUCCESS;
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
	}
	return rc;
}


int dimq_loop_read(struct dimq *dimq, int max_packets)
{
	int rc = dimq_ERR_SUCCESS;
	int i;
	if(max_packets < 1) return dimq_ERR_INVAL;

#ifdef WITH_TLS
	if(dimq->want_connect){
		return net__socket_connect_tls(dimq);
	}
#endif

	pthread_mutex_lock(&dimq->msgs_out.mutex);
	max_packets = dimq->msgs_out.queue_len;
	pthread_mutex_unlock(&dimq->msgs_out.mutex);

	pthread_mutex_lock(&dimq->msgs_in.mutex);
	max_packets += dimq->msgs_in.queue_len;
	pthread_mutex_unlock(&dimq->msgs_in.mutex);

	if(max_packets < 1) max_packets = 1;
	/* Queue len here tells us how many messages are awaiting processing and
	 * have QoS > 0. We should try to deal with that many in this loop in order
	 * to keep up. */
	for(i=0; i<max_packets || SSL_DATA_PENDING(dimq); i++){
#ifdef WITH_SOCKS
		if(dimq->socks5_host){
			rc = socks5__read(dimq);
		}else
#endif
		{
			rc = packet__read(dimq);
		}
		if(rc || errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
			return dimq__loop_rc_handle(dimq, rc);
		}
	}
	return rc;
}


int dimq_loop_write(struct dimq *dimq, int max_packets)
{
	int rc = dimq_ERR_SUCCESS;
	int i;
	if(max_packets < 1) return dimq_ERR_INVAL;

	for(i=0; i<max_packets; i++){
		rc = packet__write(dimq);
		if(rc || errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
			return dimq__loop_rc_handle(dimq, rc);
		}
	}
	return rc;
}

