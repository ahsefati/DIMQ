/*
Copyright (c) 2011-2020 Roger Light <roger@atchoo.org>

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

#ifndef WIN32
#include <time.h>
#endif

#if defined(WITH_THREADING)
#if defined(__linux__) || defined(__NetBSD__)
#  include <pthread.h>
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
#  include <pthread_np.h>
#endif
#endif

#include "dimq_internal.h"
#include "net_dimq.h"
#include "util_dimq.h"

void *dimq__thread_main(void *obj);

int dimq_loop_start(struct dimq *dimq)
{
#if defined(WITH_THREADING)
	if(!dimq || dimq->threaded != dimq_ts_none) return dimq_ERR_INVAL;

	dimq->threaded = dimq_ts_self;
	if(!pthread_create(&dimq->thread_id, NULL, dimq__thread_main, dimq)){
#if defined(__linux__)
		pthread_setname_np(dimq->thread_id, "dimq loop");
#elif defined(__NetBSD__)
		pthread_setname_np(dimq->thread_id, "%s", "dimq loop");
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
		pthread_set_name_np(dimq->thread_id, "dimq loop");
#endif
		return dimq_ERR_SUCCESS;
	}else{
		return dimq_ERR_ERRNO;
	}
#else
	UNUSED(dimq);
	return dimq_ERR_NOT_SUPPORTED;
#endif
}

int dimq_loop_stop(struct dimq *dimq, bool force)
{
#if defined(WITH_THREADING)
#  ifndef WITH_BROKER
	char sockpair_data = 0;
#  endif

	if(!dimq || dimq->threaded != dimq_ts_self) return dimq_ERR_INVAL;


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
	
#ifdef HAVE_PTHREAD_CANCEL
	if(force){
		pthread_cancel(dimq->thread_id);
	}
#endif
	pthread_join(dimq->thread_id, NULL);
	dimq->thread_id = pthread_self();
	dimq->threaded = dimq_ts_none;

	return dimq_ERR_SUCCESS;
#else
	UNUSED(dimq);
	UNUSED(force);
	return dimq_ERR_NOT_SUPPORTED;
#endif
}

#ifdef WITH_THREADING
void *dimq__thread_main(void *obj)
{
	struct dimq *dimq = obj;
#ifndef WIN32
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 10000000;
#endif

	if(!dimq) return NULL;

	do{
		if(dimq__get_state(dimq) == dimq_cs_new){
#ifdef WIN32
			Sleep(10);
#else
			nanosleep(&ts, NULL);
#endif
		}else{
			break;
		}
	}while(1);

	if(!dimq->keepalive){
		/* Sleep for a day if keepalive disabled. */
		dimq_loop_forever(dimq, 1000*86400, 1);
	}else{
		/* Sleep for our keepalive value. publish() etc. will wake us up. */
		dimq_loop_forever(dimq, dimq->keepalive*1000, 1);
	}
	if(dimq->threaded == dimq_ts_self){
		dimq->threaded = dimq_ts_none;
	}

	return obj;
}
#endif

int dimq_threaded_set(struct dimq *dimq, bool threaded)
{
	if(!dimq) return dimq_ERR_INVAL;

	if(threaded){
		dimq->threaded = dimq_ts_external;
	}else{
		dimq->threaded = dimq_ts_none;
	}

	return dimq_ERR_SUCCESS;
}
