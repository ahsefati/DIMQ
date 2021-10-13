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
#ifndef PACKET_dimq_H
#define PACKET_dimq_H

#include "dimq_internal.h"
#include "dimq.h"

int packet__alloc(struct dimq__packet *packet);
void packet__cleanup(struct dimq__packet *packet);
void packet__cleanup_all(struct dimq *dimq);
void packet__cleanup_all_no_locks(struct dimq *dimq);
int packet__queue(struct dimq *dimq, struct dimq__packet *packet);

int packet__check_oversize(struct dimq *dimq, uint32_t remaining_length);

int packet__read_byte(struct dimq__packet *packet, uint8_t *byte);
int packet__read_bytes(struct dimq__packet *packet, void *bytes, uint32_t count);
int packet__read_binary(struct dimq__packet *packet, uint8_t **data, uint16_t *length);
int packet__read_string(struct dimq__packet *packet, char **str, uint16_t *length);
int packet__read_uint16(struct dimq__packet *packet, uint16_t *word);
int packet__read_uint32(struct dimq__packet *packet, uint32_t *word);
int packet__read_varint(struct dimq__packet *packet, uint32_t *word, uint8_t *bytes);

void packet__write_byte(struct dimq__packet *packet, uint8_t byte);
void packet__write_bytes(struct dimq__packet *packet, const void *bytes, uint32_t count);
void packet__write_string(struct dimq__packet *packet, const char *str, uint16_t length);
void packet__write_uint16(struct dimq__packet *packet, uint16_t word);
void packet__write_uint32(struct dimq__packet *packet, uint32_t word);
int packet__write_varint(struct dimq__packet *packet, uint32_t word);

unsigned int packet__varint_bytes(uint32_t word);

int packet__write(struct dimq *dimq);
int packet__read(struct dimq *dimq);

#endif
