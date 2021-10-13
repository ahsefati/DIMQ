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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "logging_dimq.h"
#include "dimq_internal.h"
#include "dimq.h"
#include "memory_dimq.h"

int log__printf(struct dimq *dimq, unsigned int priority, const char *fmt, ...)
{
	va_list va;
	char *s;
	size_t len;

	assert(dimq);
	assert(fmt);

	pthread_mutex_lock(&dimq->log_callback_mutex);
	if(dimq->on_log){
		len = strlen(fmt) + 500;
		s = dimq__malloc(len*sizeof(char));
		if(!s){
			pthread_mutex_unlock(&dimq->log_callback_mutex);
			return dimq_ERR_NOMEM;
		}

		va_start(va, fmt);
		vsnprintf(s, len, fmt, va);
		va_end(va);
		s[len-1] = '\0'; /* Ensure string is null terminated. */

		dimq->on_log(dimq, dimq->userdata, (int)priority, s);

		dimq__free(s);
	}
	pthread_mutex_unlock(&dimq->log_callback_mutex);

	return dimq_ERR_SUCCESS;
}

