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

#include <stdio.h>
#include <string.h>

#include "logging_dimq.h"
#include "mqtt_protocol.h"
#include "memory_dimq.h"
#include "net_dimq.h"
#include "packet_dimq.h"
#include "property_dimq.h"
#include "read_handle.h"
#include "send_dimq.h"
#include "util_dimq.h"

int handle__disconnect(struct dimq *dimq)
{
	int rc;
	uint8_t reason_code;
	dimq_property *properties = NULL;

	if(!dimq){
		return dimq_ERR_INVAL;
	}

	if(dimq->protocol != dimq_p_mqtt5){
		return dimq_ERR_PROTOCOL;
	}
	if(dimq->in_packet.command != CMD_DISCONNECT){
		return dimq_ERR_MALFORMED_PACKET;
	}

	rc = packet__read_byte(&dimq->in_packet, &reason_code);
	if(rc) return rc;

	if(dimq->in_packet.remaining_length > 2){
		rc = property__read_all(CMD_DISCONNECT, &dimq->in_packet, &properties);
		if(rc) return rc;
		dimq_property_free_all(&properties);
	}

	log__printf(dimq, dimq_LOG_DEBUG, "Received DISCONNECT (%d)", reason_code);

	do_client_disconnect(dimq, reason_code, properties);

	dimq_property_free_all(&properties);

	return dimq_ERR_SUCCESS;
}

