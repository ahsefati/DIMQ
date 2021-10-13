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
#  include "sys_tree.h"
#else
#  define G_PUB_BYTES_SENT_INC(A)
#endif

#include "dimq.h"
#include "dimq_internal.h"
#include "logging_dimq.h"
#include "mqtt_protocol.h"
#include "memory_dimq.h"
#include "net_dimq.h"
#include "packet_dimq.h"
#include "property_dimq.h"
#include "send_dimq.h"
#include "time_dimq.h"
#include "util_dimq.h"

int send__pingreq(struct dimq *dimq)
{
	int rc;
	assert(dimq);
#ifdef WITH_BROKER
	log__printf(NULL, dimq_LOG_DEBUG, "Sending PINGREQ to %s", dimq->id);
#else
	log__printf(dimq, dimq_LOG_DEBUG, "Client %s sending PINGREQ", dimq->id);
#endif
	rc = send__simple_command(dimq, CMD_PINGREQ);
	if(rc == dimq_ERR_SUCCESS){
		dimq->ping_t = dimq_time();
	}
	return rc;
}

int send__pingresp(struct dimq *dimq)
{
#ifdef WITH_BROKER
	log__printf(NULL, dimq_LOG_DEBUG, "Sending PINGRESP to %s", dimq->id);
#else
	log__printf(dimq, dimq_LOG_DEBUG, "Client %s sending PINGRESP", dimq->id);
#endif
	return send__simple_command(dimq, CMD_PINGRESP);
}

int send__puback(struct dimq *dimq, uint16_t mid, uint8_t reason_code, const dimq_property *properties)
{
#ifdef WITH_BROKER
	log__printf(NULL, dimq_LOG_DEBUG, "Sending PUBACK to %s (m%d, rc%d)", dimq->id, mid, reason_code);
#else
	log__printf(dimq, dimq_LOG_DEBUG, "Client %s sending PUBACK (m%d, rc%d)", dimq->id, mid, reason_code);
#endif
	util__increment_receive_quota(dimq);
	/* We don't use Reason String or User Property yet. */
	return send__command_with_mid(dimq, CMD_PUBACK, mid, false, reason_code, properties);
}

int send__pubcomp(struct dimq *dimq, uint16_t mid, const dimq_property *properties)
{
#ifdef WITH_BROKER
	log__printf(NULL, dimq_LOG_DEBUG, "Sending PUBCOMP to %s (m%d)", dimq->id, mid);
#else
	log__printf(dimq, dimq_LOG_DEBUG, "Client %s sending PUBCOMP (m%d)", dimq->id, mid);
#endif
	util__increment_receive_quota(dimq);
	/* We don't use Reason String or User Property yet. */
	return send__command_with_mid(dimq, CMD_PUBCOMP, mid, false, 0, properties);
}


int send__pubrec(struct dimq *dimq, uint16_t mid, uint8_t reason_code, const dimq_property *properties)
{
#ifdef WITH_BROKER
	log__printf(NULL, dimq_LOG_DEBUG, "Sending PUBREC to %s (m%d, rc%d)", dimq->id, mid, reason_code);
#else
	log__printf(dimq, dimq_LOG_DEBUG, "Client %s sending PUBREC (m%d, rc%d)", dimq->id, mid, reason_code);
#endif
	if(reason_code >= 0x80 && dimq->protocol == dimq_p_mqtt5){
		util__increment_receive_quota(dimq);
	}
	/* We don't use Reason String or User Property yet. */
	return send__command_with_mid(dimq, CMD_PUBREC, mid, false, reason_code, properties);
}

int send__pubrel(struct dimq *dimq, uint16_t mid, const dimq_property *properties)
{
#ifdef WITH_BROKER
	log__printf(NULL, dimq_LOG_DEBUG, "Sending PUBREL to %s (m%d)", dimq->id, mid);
#else
	log__printf(dimq, dimq_LOG_DEBUG, "Client %s sending PUBREL (m%d)", dimq->id, mid);
#endif
	/* We don't use Reason String or User Property yet. */
	return send__command_with_mid(dimq, CMD_PUBREL|2, mid, false, 0, properties);
}

/* For PUBACK, PUBCOMP, PUBREC, and PUBREL */
int send__command_with_mid(struct dimq *dimq, uint8_t command, uint16_t mid, bool dup, uint8_t reason_code, const dimq_property *properties)
{
	struct dimq__packet *packet = NULL;
	int rc;

	assert(dimq);
	packet = dimq__calloc(1, sizeof(struct dimq__packet));
	if(!packet) return dimq_ERR_NOMEM;

	packet->command = command;
	if(dup){
		packet->command |= 8;
	}
	packet->remaining_length = 2;

	if(dimq->protocol == dimq_p_mqtt5){
		if(reason_code != 0 || properties){
			packet->remaining_length += 1;
		}

		if(properties){
			packet->remaining_length += property__get_remaining_length(properties);
		}
	}

	rc = packet__alloc(packet);
	if(rc){
		dimq__free(packet);
		return rc;
	}

	packet__write_uint16(packet, mid);

	if(dimq->protocol == dimq_p_mqtt5){
		if(reason_code != 0 || properties){
			packet__write_byte(packet, reason_code);
		}
		if(properties){
			property__write_all(packet, properties, true);
		}
	}

	return packet__queue(dimq, packet);
}

/* For DISCONNECT, PINGREQ and PINGRESP */
int send__simple_command(struct dimq *dimq, uint8_t command)
{
	struct dimq__packet *packet = NULL;
	int rc;

	assert(dimq);
	packet = dimq__calloc(1, sizeof(struct dimq__packet));
	if(!packet) return dimq_ERR_NOMEM;

	packet->command = command;
	packet->remaining_length = 0;

	rc = packet__alloc(packet);
	if(rc){
		dimq__free(packet);
		return rc;
	}

	return packet__queue(dimq, packet);
}

