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
#ifndef READ_HANDLE_H
#define READ_HANDLE_H

#include "dimq.h"
struct dimq_db;

int handle__pingreq(struct dimq *dimq);
int handle__pingresp(struct dimq *dimq);
#ifdef WITH_BROKER
int handle__pubackcomp(struct dimq *dimq, const char *type);
#else
int handle__packet(struct dimq *dimq);
int handle__connack(struct dimq *dimq);
int handle__disconnect(struct dimq *dimq);
int handle__pubackcomp(struct dimq *dimq, const char *type);
int handle__publish(struct dimq *dimq);
int handle__auth(struct dimq *dimq);
#endif
int handle__pubrec(struct dimq *dimq);
int handle__pubrel(struct dimq *dimq);
int handle__suback(struct dimq *dimq);
int handle__unsuback(struct dimq *dimq);


#endif
