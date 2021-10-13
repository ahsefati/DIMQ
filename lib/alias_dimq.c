/*
Copyright (c) 2019-2020 Roger Light <roger@atchoo.org>

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

#include "dimq.h"
#include "alias_dimq.h"
#include "memory_dimq.h"

int alias__add(struct dimq *dimq, const char *topic, uint16_t alias)
{
	int i;
	struct dimq__alias *aliases;

	for(i=0; i<dimq->alias_count; i++){
		if(dimq->aliases[i].alias == alias){
			dimq__free(dimq->aliases[i].topic);
			dimq->aliases[i].topic = dimq__strdup(topic);
			if(dimq->aliases[i].topic){
				return dimq_ERR_SUCCESS;
			}else{
				
				return dimq_ERR_NOMEM;
			}
		}
	}

	/* New alias */
	aliases = dimq__realloc(dimq->aliases, sizeof(struct dimq__alias)*(size_t)(dimq->alias_count+1));
	if(!aliases) return dimq_ERR_NOMEM;

	dimq->aliases = aliases;
	dimq->aliases[dimq->alias_count].alias = alias;
	dimq->aliases[dimq->alias_count].topic = dimq__strdup(topic);
	if(!dimq->aliases[dimq->alias_count].topic){
		return dimq_ERR_NOMEM;
	}
	dimq->alias_count++;

	return dimq_ERR_SUCCESS;
}


int alias__find(struct dimq *dimq, char **topic, uint16_t alias)
{
	int i;

	for(i=0; i<dimq->alias_count; i++){
		if(dimq->aliases[i].alias == alias){
			*topic = dimq__strdup(dimq->aliases[i].topic);
			if(*topic){
				return dimq_ERR_SUCCESS;
			}else{
				return dimq_ERR_NOMEM;
			}
		}
	}
	return dimq_ERR_INVAL;
}


void alias__free_all(struct dimq *dimq)
{
	int i;

	for(i=0; i<dimq->alias_count; i++){
		dimq__free(dimq->aliases[i].topic);
	}
	dimq__free(dimq->aliases);
	dimq->aliases = NULL;
	dimq->alias_count = 0;
}
