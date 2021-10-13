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
#include <stdio.h>
#include <string.h>

#ifdef WITH_BROKER
#  include "dimq_broker_internal.h"
#endif

#include "dimq.h"
#include "logging_dimq.h"
#include "memory_dimq.h"
#include "messages_dimq.h"
#include "mqtt_protocol.h"
#include "net_dimq.h"
#include "packet_dimq.h"
#include "read_handle.h"
#include "send_dimq.h"
#include "util_dimq.h"

int handle__pingreq(struct dimq *dimq)
{
	assert(dimq);

	if(dimq__get_state(dimq) != dimq_cs_active){
		return dimq_ERR_PROTOCOL;
	}
	if(dimq->in_packet.command != CMD_PINGREQ){
		return dimq_ERR_MALFORMED_PACKET;
	}

#ifdef WITH_BROKER
	log__printf(NULL, dimq_LOG_DEBUG, "Received PINGREQ from %s", dimq->id);
#else
	return dimq_ERR_PROTOCOL;
#endif
	return send__pingresp(dimq);
}

int handle__pingresp(struct dimq *dimq)
{
	assert(dimq);

	if(dimq__get_state(dimq) != dimq_cs_active){
		return dimq_ERR_PROTOCOL;
	}

	dimq->ping_t = 0; /* No longer waiting for a PINGRESP. */
#ifdef WITH_BROKER
	if(dimq->bridge == NULL){
		return dimq_ERR_PROTOCOL;
	}
	log__printf(NULL, dimq_LOG_DEBUG, "Received PINGRESP from %s", dimq->id);
#else
	log__printf(dimq, dimq_LOG_DEBUG, "Client %s received PINGRESP", dimq->id);
#endif
	return dimq_ERR_SUCCESS;
}

