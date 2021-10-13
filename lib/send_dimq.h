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
#ifndef SEND_dimq_H
#define SEND_dimq_H

#include "dimq.h"
#include "property_dimq.h"

int send__simple_command(struct dimq *dimq, uint8_t command);
int send__command_with_mid(struct dimq *dimq, uint8_t command, uint16_t mid, bool dup, uint8_t reason_code, const dimq_property *properties);
int send__real_publish(struct dimq *dimq, uint16_t mid, const char *topic, uint32_t payloadlen, const void *payload, uint8_t qos, bool retain, bool dup, const dimq_property *cmsg_props, const dimq_property *store_props, uint32_t expiry_interval);

int send__connect(struct dimq *dimq, uint16_t keepalive, bool clean_session, const dimq_property *properties);
int send__disconnect(struct dimq *dimq, uint8_t reason_code, const dimq_property *properties);
int send__pingreq(struct dimq *dimq);
int send__pingresp(struct dimq *dimq);
int send__puback(struct dimq *dimq, uint16_t mid, uint8_t reason_code, const dimq_property *properties);
int send__pubcomp(struct dimq *dimq, uint16_t mid, const dimq_property *properties);
int send__publish(struct dimq *dimq, uint16_t mid, const char *topic, uint32_t payloadlen, const void *payload, uint8_t qos, bool retain, bool dup, const dimq_property *cmsg_props, const dimq_property *store_props, uint32_t expiry_interval);
int send__pubrec(struct dimq *dimq, uint16_t mid, uint8_t reason_code, const dimq_property *properties);
int send__pubrel(struct dimq *dimq, uint16_t mid, const dimq_property *properties);
int send__subscribe(struct dimq *dimq, int *mid, int topic_count, char *const *const topic, int topic_qos, const dimq_property *properties);
int send__unsubscribe(struct dimq *dimq, int *mid, int topic_count, char *const *const topic, const dimq_property *properties);

#endif
