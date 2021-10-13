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
#ifndef UTIL_dimq_H
#define UTIL_dimq_H

#include <stdio.h>

#include "tls_dimq.h"
#include "dimq.h"
#include "dimq_internal.h"
#ifdef WITH_BROKER
#  include "dimq_broker_internal.h"
#endif

int dimq__check_keepalive(struct dimq *dimq);
uint16_t dimq__mid_generate(struct dimq *dimq);

int dimq__set_state(struct dimq *dimq, enum dimq_client_state state);
enum dimq_client_state dimq__get_state(struct dimq *dimq);

#ifdef WITH_TLS
int dimq__hex2bin_sha1(const char *hex, unsigned char **bin);
int dimq__hex2bin(const char *hex, unsigned char *bin, int bin_max_len);
#endif

int util__random_bytes(void *bytes, int count);

void util__increment_receive_quota(struct dimq *dimq);
void util__increment_send_quota(struct dimq *dimq);
void util__decrement_receive_quota(struct dimq *dimq);
void util__decrement_send_quota(struct dimq *dimq);


#endif
