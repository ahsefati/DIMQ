/*
Copyright (c) 2010-2019 Roger Light <roger@atchoo.org>

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

#include "dimq_broker_internal.h"
#include "memory_dimq.h"
#include "mqtt_protocol.h"
#include "util_dimq.h"

#include "utlist.h"

static struct dimq__retainhier *retain__add_hier_entry(struct dimq__retainhier *parent, struct dimq__retainhier **sibling, const char *topic, uint16_t len)
{
	struct dimq__retainhier *child;

	assert(sibling);

	child = dimq__calloc(1, sizeof(struct dimq__retainhier));
	if(!child){
		log__printf(NULL, dimq_LOG_ERR, "Error: Out of memory.");
		return NULL;
	}
	child->parent = parent;
	child->topic_len = len;
	child->topic = dimq__malloc((size_t)len+1);
	if(!child->topic){
		child->topic_len = 0;
		dimq__free(child);
		log__printf(NULL, dimq_LOG_ERR, "Error: Out of memory.");
		return NULL;
	}else{
		strncpy(child->topic, topic, (size_t)child->topic_len+1);
	}

	HASH_ADD_KEYPTR(hh, *sibling, child->topic, child->topic_len, child);

	return child;
}


int retain__init(void)
{
	struct dimq__retainhier *retainhier;

	retainhier = retain__add_hier_entry(NULL, &db.retains, "", 0);
	if(!retainhier) return dimq_ERR_NOMEM;

	retainhier = retain__add_hier_entry(NULL, &db.retains, "$SYS", (uint16_t)strlen("$SYS"));
	if(!retainhier) return dimq_ERR_NOMEM;

	return dimq_ERR_SUCCESS;
}


int retain__store(const char *topic, struct dimq_msg_store *stored, char **split_topics)
{
	struct dimq__retainhier *retainhier;
	struct dimq__retainhier *branch;
	int i;
	size_t slen;

	assert(stored);
	assert(split_topics);

	HASH_FIND(hh, db.retains, split_topics[0], strlen(split_topics[0]), retainhier);
	if(retainhier == NULL){
		retainhier = retain__add_hier_entry(NULL, &db.retains, split_topics[0], (uint16_t)strlen(split_topics[0]));
		if(!retainhier) return dimq_ERR_NOMEM;
	}

	for(i=0; split_topics[i] != NULL; i++){
		slen = strlen(split_topics[i]);
		HASH_FIND(hh, retainhier->children, split_topics[i], slen, branch);
		if(branch == NULL){
			branch = retain__add_hier_entry(retainhier, &retainhier->children, split_topics[i], (uint16_t)slen);
			if(branch == NULL){
				return dimq_ERR_NOMEM;
			}
		}
		retainhier = branch;
	}

#ifdef WITH_PERSISTENCE
	if(strncmp(topic, "$SYS", 4)){
		/* Retained messages count as a persistence change, but only if
		 * they aren't for $SYS. */
		db.persistence_changes++;
	}
#else
	UNUSED(topic);
#endif

	if(retainhier->retained){
		db__msg_store_ref_dec(&retainhier->retained);
#ifdef WITH_SYS_TREE
		db.retained_count--;
#endif
	}
	if(stored->payloadlen){
		retainhier->retained = stored;
		db__msg_store_ref_inc(retainhier->retained);
#ifdef WITH_SYS_TREE
		db.retained_count++;
#endif
	}else{
		retainhier->retained = NULL;
	}

	return dimq_ERR_SUCCESS;
}


static int retain__process(struct dimq__retainhier *branch, struct dimq *context, uint8_t sub_qos, uint32_t subscription_identifier)
{
	int rc = 0;
	uint8_t qos;
	uint16_t mid;
	dimq_property *properties = NULL;
	struct dimq_msg_store *retained;

	if(branch->retained->message_expiry_time > 0 && db.now_real_s >= branch->retained->message_expiry_time){
		db__msg_store_ref_dec(&branch->retained);
		branch->retained = NULL;
#ifdef WITH_SYS_TREE
		db.retained_count--;
#endif
		return dimq_ERR_SUCCESS;
	}

	retained = branch->retained;

	rc = dimq_acl_check(context, retained->topic, retained->payloadlen, retained->payload,
			retained->qos, retained->retain, dimq_ACL_READ);
	if(rc == dimq_ERR_ACL_DENIED){
		return dimq_ERR_SUCCESS;
	}else if(rc != dimq_ERR_SUCCESS){
		return rc;
	}

	/* Check for original source access */
	if(db.config->check_retain_source && retained->origin != dimq_mo_broker && retained->source_id){
		struct dimq retain_ctxt;
		memset(&retain_ctxt, 0, sizeof(struct dimq));

		retain_ctxt.id = retained->source_id;
		retain_ctxt.username = retained->source_username;
		retain_ctxt.listener = retained->source_listener;

		rc = acl__find_acls(&retain_ctxt);
		if(rc) return rc;

		rc = dimq_acl_check(&retain_ctxt, retained->topic, retained->payloadlen, retained->payload,
				retained->qos, retained->retain, dimq_ACL_WRITE);
		if(rc == dimq_ERR_ACL_DENIED){
			return dimq_ERR_SUCCESS;
		}else if(rc != dimq_ERR_SUCCESS){
			return rc;
		}
	}

	if (db.config->upgrade_outgoing_qos){
		qos = sub_qos;
	} else {
		qos = retained->qos;
		if(qos > sub_qos) qos = sub_qos;
	}
	if(qos > 0){
		mid = dimq__mid_generate(context);
	}else{
		mid = 0;
	}
	if(subscription_identifier > 0){
		dimq_property_add_varint(&properties, MQTT_PROP_SUBSCRIPTION_IDENTIFIER, subscription_identifier);
	}
	return db__message_insert(context, mid, dimq_md_out, qos, true, retained, properties, false);
}


static int retain__search(struct dimq__retainhier *retainhier, char **split_topics, struct dimq *context, const char *sub, uint8_t sub_qos, uint32_t subscription_identifier, int level)
{
	struct dimq__retainhier *branch, *branch_tmp;
	int flag = 0;

	if(!strcmp(split_topics[0], "#") && split_topics[1] == NULL){
		HASH_ITER(hh, retainhier->children, branch, branch_tmp){
			/* Set flag to indicate that we should check for retained messages
			 * on "foo" when we are subscribing to e.g. "foo/#" and then exit
			 * this function and return to an earlier retain__search().
			 */
			flag = -1;
			if(branch->retained){
				retain__process(branch, context, sub_qos, subscription_identifier);
			}
			if(branch->children){
				retain__search(branch, split_topics, context, sub, sub_qos, subscription_identifier, level+1);
			}
		}
	}else{
		if(!strcmp(split_topics[0], "+")){
			HASH_ITER(hh, retainhier->children, branch, branch_tmp){
				if(split_topics[1] != NULL){
					if(retain__search(branch, &(split_topics[1]), context, sub, sub_qos, subscription_identifier, level+1) == -1
							|| (split_topics[1] != NULL && !strcmp(split_topics[1], "#") && level>0)){

						if(branch->retained){
							retain__process(branch, context, sub_qos, subscription_identifier);
						}
					}
				}else{
					if(branch->retained){
						retain__process(branch, context, sub_qos, subscription_identifier);
					}
				}
			}
		}else{
			HASH_FIND(hh, retainhier->children, split_topics[0], strlen(split_topics[0]), branch);
			if(branch){
				if(split_topics[1] != NULL){
					if(retain__search(branch, &(split_topics[1]), context, sub, sub_qos, subscription_identifier, level+1) == -1
							|| (split_topics[1] != NULL && !strcmp(split_topics[1], "#") && level>0)){

						if(branch->retained){
							retain__process(branch, context, sub_qos, subscription_identifier);
						}
					}
				}else{
					if(branch->retained){
						retain__process(branch, context, sub_qos, subscription_identifier);
					}
				}
			}
		}
	}
	return flag;
}


int retain__queue(struct dimq *context, const char *sub, uint8_t sub_qos, uint32_t subscription_identifier)
{
	struct dimq__retainhier *retainhier;
	char *local_sub;
	char **split_topics;
	int rc;

	assert(context);
	assert(sub);

	rc = sub__topic_tokenise(sub, &local_sub, &split_topics, NULL);
	if(rc) return rc;

	HASH_FIND(hh, db.retains, split_topics[0], strlen(split_topics[0]), retainhier);

	if(retainhier){
		retain__search(retainhier, split_topics, context, sub, sub_qos, subscription_identifier, 0);
	}
	dimq__free(local_sub);
	dimq__free(split_topics);

	return dimq_ERR_SUCCESS;
}


void retain__clean(struct dimq__retainhier **retainhier)
{
	struct dimq__retainhier *peer, *retainhier_tmp;

	HASH_ITER(hh, *retainhier, peer, retainhier_tmp){
		if(peer->retained){
			db__msg_store_ref_dec(&peer->retained);
		}
		retain__clean(&peer->children);
		dimq__free(peer->topic);

		HASH_DELETE(hh, *retainhier, peer);
		dimq__free(peer);
	}
}

