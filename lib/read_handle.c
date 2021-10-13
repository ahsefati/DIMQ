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

#include "dimq.h"
#include "logging_dimq.h"
#include "memory_dimq.h"
#include "messages_dimq.h"
#include "mqtt_protocol.h"
#include "net_dimq.h"
#include "packet_dimq.h"
#include "read_handle.h"
#include "send_dimq.h"
#include "time_dimq.h"
#include "util_dimq.h"

int handle__packet(struct dimq *dimq)
{
	assert(dimq);

	switch((dimq->in_packet.command)&0xF0){
		case CMD_PINGREQ:
			return handle__pingreq(dimq);
		case CMD_PINGRESP:
			return handle__pingresp(dimq);
		case CMD_PUBACK:
			return handle__pubackcomp(dimq, "PUBACK");
		case CMD_PUBCOMP:
			return handle__pubackcomp(dimq, "PUBCOMP");
		case CMD_PUBLISH:
			return handle__publish(dimq);
		case CMD_PUBREC:
			return handle__pubrec(dimq);
		case CMD_PUBREL:
			return handle__pubrel(dimq);
		case CMD_CONNACK:
			return handle__connack(dimq);
		case CMD_SUBACK:
			return handle__suback(dimq);
		case CMD_UNSUBACK:
			return handle__unsuback(dimq);
		case CMD_DISCONNECT:
			return handle__disconnect(dimq);
		case CMD_AUTH:
			return handle__auth(dimq);
		default:
			/* If we don't recognise the command, return an error straight away. */
			log__printf(dimq, dimq_LOG_ERR, "Error: Unrecognised command %d\n", (dimq->in_packet.command)&0xF0);
			return dimq_ERR_PROTOCOL;
	}
}

