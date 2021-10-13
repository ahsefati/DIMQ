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
#ifndef MESSAGES_dimq_H
#define MESSAGES_dimq_H

#include "dimq_internal.h"
#include "dimq.h"

void message__cleanup_all(struct dimq *dimq);
void message__cleanup(struct dimq_message_all **message);
int message__delete(struct dimq *dimq, uint16_t mid, enum dimq_msg_direction dir, int qos);
int message__queue(struct dimq *dimq, struct dimq_message_all *message, enum dimq_msg_direction dir);
void message__reconnect_reset(struct dimq *dimq, bool update_quota_only);
int message__release_to_inflight(struct dimq *dimq, enum dimq_msg_direction dir);
int message__remove(struct dimq *dimq, uint16_t mid, enum dimq_msg_direction dir, struct dimq_message_all **message, int qos);
void message__retry_check(struct dimq *dimq);
int message__out_update(struct dimq *dimq, uint16_t mid, enum dimq_msg_state state, int qos);

#endif
