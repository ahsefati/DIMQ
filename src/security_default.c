/*
Copyright (c) 2011-2020 Roger Light <roger@atchoo.org>

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

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "dimq_broker_internal.h"
#include "memory_dimq.h"
#include "mqtt_protocol.h"
#include "send_dimq.h"
#include "misc_dimq.h"
#include "util_dimq.h"

static int aclfile__parse(struct dimq__security_options *security_opts);
static int unpwd__file_parse(struct dimq__unpwd **unpwd, const char *password_file);
static int acl__cleanup(bool reload);
static int unpwd__cleanup(struct dimq__unpwd **unpwd, bool reload);
static int psk__file_parse(struct dimq__unpwd **psk_id, const char *psk_file);
#ifdef WITH_TLS
static int pw__digest(const char *password, const unsigned char *salt, unsigned int salt_len, unsigned char *hash, unsigned int *hash_len, enum dimq_pwhash_type hashtype, int iterations);
#endif
static int dimq_unpwd_check_default(int event, void *event_data, void *userdata);
static int dimq_acl_check_default(int event, void *event_data, void *userdata);



int dimq_security_init_default(bool reload)
{
	int rc;
	int i;
	char *pwf;
	char *pskf = NULL;

	UNUSED(reload);

	/* Configure plugin identifier */
	if(db.config->per_listener_settings){
		for(i=0; i<db.config->listener_count; i++){
			db.config->listeners[i].security_options.pid = dimq__calloc(1, sizeof(dimq_plugin_id_t));
			if(db.config->listeners[i].security_options.pid == NULL){
				log__printf(NULL, dimq_LOG_ERR, "Error: Out of memory.");
				return dimq_ERR_NOMEM;
			}
			db.config->listeners[i].security_options.pid->listener = &db.config->listeners[i];
		}
	}else{
		db.config->security_options.pid = dimq__calloc(1, sizeof(dimq_plugin_id_t));
		if(db.config->security_options.pid == NULL){
			log__printf(NULL, dimq_LOG_ERR, "Error: Out of memory.");
			return dimq_ERR_NOMEM;
		}
	}

	/* Load username/password data if required. */
	if(db.config->per_listener_settings){
		for(i=0; i<db.config->listener_count; i++){
			pwf = db.config->listeners[i].security_options.password_file;
			if(pwf){
				rc = unpwd__file_parse(&db.config->listeners[i].security_options.unpwd, pwf);
				if(rc){
					log__printf(NULL, dimq_LOG_ERR, "Error opening password file \"%s\".", pwf);
					return rc;
				}
				dimq_callback_register(db.config->listeners[i].security_options.pid,
						dimq_EVT_BASIC_AUTH, dimq_unpwd_check_default, NULL, NULL);
			}
		}
	}else{
		if(db.config->security_options.password_file){
			pwf = db.config->security_options.password_file;
			if(pwf){
				rc = unpwd__file_parse(&db.config->security_options.unpwd, pwf);
				if(rc){
					log__printf(NULL, dimq_LOG_ERR, "Error opening password file \"%s\".", pwf);
					return rc;
				}
			}
			dimq_callback_register(db.config->security_options.pid,
					dimq_EVT_BASIC_AUTH, dimq_unpwd_check_default, NULL, NULL);
		}
	}

	/* Load acl data if required. */
	if(db.config->per_listener_settings){
		for(i=0; i<db.config->listener_count; i++){
			if(db.config->listeners[i].security_options.acl_file){
				rc = aclfile__parse(&db.config->listeners[i].security_options);
				if(rc){
					log__printf(NULL, dimq_LOG_ERR, "Error opening acl file \"%s\".", db.config->listeners[i].security_options.acl_file);
					return rc;
				}
				dimq_callback_register(db.config->listeners[i].security_options.pid,
						dimq_EVT_ACL_CHECK, dimq_acl_check_default, NULL, NULL);
			}
		}
	}else{
		if(db.config->security_options.acl_file){
			rc = aclfile__parse(&db.config->security_options);
			if(rc){
				log__printf(NULL, dimq_LOG_ERR, "Error opening acl file \"%s\".", db.config->security_options.acl_file);
				return rc;
			}
			dimq_callback_register(db.config->security_options.pid,
					dimq_EVT_ACL_CHECK, dimq_acl_check_default, NULL, NULL);
		}
	}

	/* Load psk data if required. */
	if(db.config->per_listener_settings){
		for(i=0; i<db.config->listener_count; i++){
			pskf = db.config->listeners[i].security_options.psk_file;
			if(pskf){
				rc = psk__file_parse(&db.config->listeners[i].security_options.psk_id, pskf);
				if(rc){
					log__printf(NULL, dimq_LOG_ERR, "Error opening psk file \"%s\".", pskf);
					return rc;
				}
			}
		}
	}else{
		pskf = db.config->security_options.psk_file;
		if(pskf){
			rc = psk__file_parse(&db.config->security_options.psk_id, pskf);
			if(rc){
				log__printf(NULL, dimq_LOG_ERR, "Error opening psk file \"%s\".", pskf);
				return rc;
			}
		}
	}

	return dimq_ERR_SUCCESS;
}

int dimq_security_cleanup_default(bool reload)
{
	int rc;
	int i;

	rc = acl__cleanup(reload);
	if(rc != dimq_ERR_SUCCESS) return rc;

	rc = unpwd__cleanup(&db.config->security_options.unpwd, reload);
	if(rc != dimq_ERR_SUCCESS) return rc;

	for(i=0; i<db.config->listener_count; i++){
		if(db.config->listeners[i].security_options.unpwd){
			rc = unpwd__cleanup(&db.config->listeners[i].security_options.unpwd, reload);
			if(rc != dimq_ERR_SUCCESS) return rc;
		}
	}

	rc = unpwd__cleanup(&db.config->security_options.psk_id, reload);
	if(rc != dimq_ERR_SUCCESS) return rc;

	for(i=0; i<db.config->listener_count; i++){
		if(db.config->listeners[i].security_options.psk_id){
			rc = unpwd__cleanup(&db.config->listeners[i].security_options.psk_id, reload);
			if(rc != dimq_ERR_SUCCESS) return rc;
		}
	}

	if(db.config->per_listener_settings){
		for(i=0; i<db.config->listener_count; i++){
			if(db.config->listeners[i].security_options.pid){
				dimq_callback_unregister(db.config->listeners[i].security_options.pid,
						dimq_EVT_BASIC_AUTH, dimq_unpwd_check_default, NULL);
				dimq_callback_unregister(db.config->listeners[i].security_options.pid,
						dimq_EVT_ACL_CHECK, dimq_acl_check_default, NULL);

				dimq__free(db.config->listeners[i].security_options.pid);
			}
		}
	}else{
		if(db.config->security_options.pid){
			dimq_callback_unregister(db.config->security_options.pid,
					dimq_EVT_BASIC_AUTH, dimq_unpwd_check_default, NULL);
			dimq_callback_unregister(db.config->security_options.pid,
					dimq_EVT_ACL_CHECK, dimq_acl_check_default, NULL);

			dimq__free(db.config->security_options.pid);
		}
	}
	return dimq_ERR_SUCCESS;
}


static int add__acl(struct dimq__security_options *security_opts, const char *user, const char *topic, int access)
{
	struct dimq__acl_user *acl_user=NULL, *user_tail;
	struct dimq__acl *acl, *acl_tail;
	char *local_topic;
	bool new_user = false;

	if(!security_opts || !topic) return dimq_ERR_INVAL;

	local_topic = dimq__strdup(topic);
	if(!local_topic){
		return dimq_ERR_NOMEM;
	}

	if(security_opts->acl_list){
		user_tail = security_opts->acl_list;
		while(user_tail){
			if(user == NULL){
				if(user_tail->username == NULL){
					acl_user = user_tail;
					break;
				}
			}else if(user_tail->username && !strcmp(user_tail->username, user)){
				acl_user = user_tail;
				break;
			}
			user_tail = user_tail->next;
		}
	}
	if(!acl_user){
		acl_user = dimq__malloc(sizeof(struct dimq__acl_user));
		if(!acl_user){
			dimq__free(local_topic);
			return dimq_ERR_NOMEM;
		}
		new_user = true;
		if(user){
			acl_user->username = dimq__strdup(user);
			if(!acl_user->username){
				dimq__free(local_topic);
				dimq__free(acl_user);
				return dimq_ERR_NOMEM;
			}
		}else{
			acl_user->username = NULL;
		}
		acl_user->next = NULL;
		acl_user->acl = NULL;
	}

	acl = dimq__malloc(sizeof(struct dimq__acl));
	if(!acl){
		dimq__free(local_topic);
		dimq__free(acl_user->username);
		dimq__free(acl_user);
		return dimq_ERR_NOMEM;
	}
	acl->access = access;
	acl->topic = local_topic;
	acl->next = NULL;
	acl->ccount = 0;
	acl->ucount = 0;

	/* Add acl to user acl list */
	if(acl_user->acl){
		acl_tail = acl_user->acl;
		if(access == dimq_ACL_NONE){
			/* Put "deny" acls at front of the list */
			acl->next = acl_tail;
			acl_user->acl = acl;
		}else{
			while(acl_tail->next){
				acl_tail = acl_tail->next;
			}
			acl_tail->next = acl;
		}
	}else{
		acl_user->acl = acl;
	}

	if(new_user){
		/* Add to end of list */
		if(security_opts->acl_list){
			user_tail = security_opts->acl_list;
			while(user_tail->next){
				user_tail = user_tail->next;
			}
			user_tail->next = acl_user;
		}else{
			security_opts->acl_list = acl_user;
		}
	}

	return dimq_ERR_SUCCESS;
}

static int add__acl_pattern(struct dimq__security_options *security_opts, const char *topic, int access)
{
	struct dimq__acl *acl, *acl_tail;
	char *local_topic;
	char *s;

	if(!security_opts| !topic) return dimq_ERR_INVAL;

	local_topic = dimq__strdup(topic);
	if(!local_topic){
		return dimq_ERR_NOMEM;
	}

	acl = dimq__malloc(sizeof(struct dimq__acl));
	if(!acl){
		dimq__free(local_topic);
		return dimq_ERR_NOMEM;
	}
	acl->access = access;
	acl->topic = local_topic;
	acl->next = NULL;

	acl->ccount = 0;
	s = local_topic;
	while(s){
		s = strstr(s, "%c");
		if(s){
			acl->ccount++;
			s+=2;
		}
	}

	acl->ucount = 0;
	s = local_topic;
	while(s){
		s = strstr(s, "%u");
		if(s){
			acl->ucount++;
			s+=2;
		}
	}

	if(acl->ccount == 0 && acl->ucount == 0){
		log__printf(NULL, dimq_LOG_WARNING,
				"Warning: ACL pattern '%s' does not contain '%%c' or '%%u'.",
				topic);
	}

	if(security_opts->acl_patterns){
		acl_tail = security_opts->acl_patterns;
		if(access == dimq_ACL_NONE){
			/* Put "deny" acls at front of the list */
			acl->next = acl_tail;
			security_opts->acl_patterns = acl;
		}else{
			while(acl_tail->next){
				acl_tail = acl_tail->next;
			}
			acl_tail->next = acl;
		}
	}else{
		security_opts->acl_patterns = acl;
	}

	return dimq_ERR_SUCCESS;
}

static int dimq_acl_check_default(int event, void *event_data, void *userdata)
{
	struct dimq_evt_acl_check *ed = event_data;
	char *local_acl;
	struct dimq__acl *acl_root;
	bool result;
	size_t i;
	size_t len, tlen, clen, ulen;
	char *s;
	struct dimq__security_options *security_opts = NULL;

	UNUSED(event);
	UNUSED(userdata);

	if(ed->client->bridge) return dimq_ERR_SUCCESS;
	if(ed->access == dimq_ACL_SUBSCRIBE || ed->access == dimq_ACL_UNSUBSCRIBE) return dimq_ERR_SUCCESS; /* FIXME - implement ACL subscription strings. */

	if(db.config->per_listener_settings){
		if(!ed->client->listener) return dimq_ERR_ACL_DENIED;
		security_opts = &ed->client->listener->security_options;
	}else{
		security_opts = &db.config->security_options;
	}
	if(!security_opts->acl_file && !security_opts->acl_list && !security_opts->acl_patterns){
		return dimq_ERR_PLUGIN_DEFER;
	}

	if(!ed->client->acl_list && !security_opts->acl_patterns) return dimq_ERR_ACL_DENIED;

	if(ed->client->acl_list){
		acl_root = ed->client->acl_list->acl;
	}else{
		acl_root = NULL;
	}

	/* Loop through all ACLs for this client. ACL denials are iterated over first. */
	while(acl_root){
		/* Loop through the topic looking for matches to this ACL. */

		/* If subscription starts with $, acl_root->topic must also start with $. */
		if(ed->topic[0] == '$' && acl_root->topic[0] != '$'){
			acl_root = acl_root->next;
			continue;
		}
		dimq_topic_matches_sub(acl_root->topic, ed->topic, &result);
		if(result){
			if(acl_root->access == dimq_ACL_NONE){
				/* Access was explicitly denied for this topic. */
				return dimq_ERR_ACL_DENIED;
			}
			if(ed->access & acl_root->access){
				/* And access is allowed. */
				return dimq_ERR_SUCCESS;
			}
		}
		acl_root = acl_root->next;
	}

	acl_root = security_opts->acl_patterns;

	if(acl_root){
		/* We are using pattern based acls. Check whether the username or
		 * client id contains a + or # and if so deny access.
		 *
		 * Without this, a malicious client may configure its username/client
		 * id to bypass ACL checks (or have a username/client id that cannot
		 * publish or receive messages to its own place in the hierarchy).
		 */
		if(ed->client->username && strpbrk(ed->client->username, "+#")){
			log__printf(NULL, dimq_LOG_NOTICE, "ACL denying access to client with dangerous username \"%s\"", ed->client->username);
			return dimq_ERR_ACL_DENIED;
		}

		if(ed->client->id && strpbrk(ed->client->id, "+#")){
			log__printf(NULL, dimq_LOG_NOTICE, "ACL denying access to client with dangerous client id \"%s\"", ed->client->id);
			return dimq_ERR_ACL_DENIED;
		}
	}

	/* Loop through all pattern ACLs. ACL denial patterns are iterated over first. */
	if(!ed->client->id) return dimq_ERR_ACL_DENIED;
	clen = strlen(ed->client->id);

	while(acl_root){
		tlen = strlen(acl_root->topic);

		if(acl_root->ucount && !ed->client->username){
			acl_root = acl_root->next;
			continue;
		}

		if(ed->client->username){
			ulen = strlen(ed->client->username);
			len = tlen + (size_t)acl_root->ccount*(clen-2) + (size_t)acl_root->ucount*(ulen-2);
		}else{
			ulen = 0;
			len = tlen + (size_t)acl_root->ccount*(clen-2);
		}
		local_acl = dimq__malloc(len+1);
		if(!local_acl) return dimq_ERR_NOMEM;
		s = local_acl;
		for(i=0; i<tlen; i++){
			if(i<tlen-1 && acl_root->topic[i] == '%'){
				if(acl_root->topic[i+1] == 'c'){
					i++;
					strncpy(s, ed->client->id, clen);
					s+=clen;
					continue;
				}else if(ed->client->username && acl_root->topic[i+1] == 'u'){
					i++;
					strncpy(s, ed->client->username, ulen);
					s+=ulen;
					continue;
				}
			}
			s[0] = acl_root->topic[i];
			s++;
		}
		local_acl[len] = '\0';

		dimq_topic_matches_sub(local_acl, ed->topic, &result);
		dimq__free(local_acl);
		if(result){
			if(acl_root->access == dimq_ACL_NONE){
				/* Access was explicitly denied for this topic pattern. */
				return dimq_ERR_ACL_DENIED;
			}
			if(ed->access & acl_root->access){
				/* And access is allowed. */
				return dimq_ERR_SUCCESS;
			}
		}

		acl_root = acl_root->next;
	}

	return dimq_ERR_ACL_DENIED;
}


static int aclfile__parse(struct dimq__security_options *security_opts)
{
	FILE *aclfptr = NULL;
	char *token;
	char *user = NULL;
	char *topic;
	char *access_s;
	int access;
	int rc = dimq_ERR_SUCCESS;
	size_t slen;
	int topic_pattern;
	char *saveptr = NULL;
	char *buf = NULL;
	int buflen = 256;

	if(!db.config) return dimq_ERR_INVAL;
	if(!security_opts) return dimq_ERR_INVAL;
	if(!security_opts->acl_file) return dimq_ERR_SUCCESS;

	buf = dimq__malloc((size_t)buflen);
	if(buf == NULL){
		log__printf(NULL, dimq_LOG_ERR, "Error: Out of memory.");
		return dimq_ERR_NOMEM;
	}

	aclfptr = dimq__fopen(security_opts->acl_file, "rt", false);
	if(!aclfptr){
		dimq__free(buf);
		log__printf(NULL, dimq_LOG_ERR, "Error: Unable to open acl_file \"%s\".", security_opts->acl_file);
		return dimq_ERR_UNKNOWN;
	}

	/* topic [read|write] <topic>
	 * user <user>
	 */

	while(fgets_extending(&buf, &buflen, aclfptr)){
		slen = strlen(buf);
		while(slen > 0 && isspace(buf[slen-1])){
			buf[slen-1] = '\0';
			slen = strlen(buf);
		}
		if(buf[0] == '#'){
			continue;
		}
		token = strtok_r(buf, " ", &saveptr);
		if(token){
			if(!strcmp(token, "topic") || !strcmp(token, "pattern")){
				if(!strcmp(token, "topic")){
					topic_pattern = 0;
				}else{
					topic_pattern = 1;
				}

				access_s = strtok_r(NULL, " ", &saveptr);
				if(!access_s){
					log__printf(NULL, dimq_LOG_ERR, "Error: Empty topic in acl_file \"%s\".", security_opts->acl_file);
					rc = dimq_ERR_INVAL;
					break;
				}
				token = strtok_r(NULL, "", &saveptr);
				if(token){
					topic = misc__trimblanks(token);
				}else{
					topic = access_s;
					access_s = NULL;
				}
				if(access_s){
					if(!strcmp(access_s, "read")){
						access = dimq_ACL_READ;
					}else if(!strcmp(access_s, "write")){
						access = dimq_ACL_WRITE;
					}else if(!strcmp(access_s, "readwrite")){
						access = dimq_ACL_READ | dimq_ACL_WRITE;
					}else if(!strcmp(access_s, "deny")){
						access = dimq_ACL_NONE;
					}else{
						log__printf(NULL, dimq_LOG_ERR, "Error: Invalid topic access type \"%s\" in acl_file \"%s\".", access_s, security_opts->acl_file);
						rc = dimq_ERR_INVAL;
						break;
					}
				}else{
					access = dimq_ACL_READ | dimq_ACL_WRITE;
				}
				rc = dimq_sub_topic_check(topic);
				if(rc != dimq_ERR_SUCCESS){
					log__printf(NULL, dimq_LOG_ERR, "Error: Invalid ACL topic \"%s\" in acl_file \"%s\".", topic, security_opts->acl_file);
					rc = dimq_ERR_INVAL;
					break;
				}

				if(topic_pattern == 0){
					rc = add__acl(security_opts, user, topic, access);
				}else{
					rc = add__acl_pattern(security_opts, topic, access);
				}
				if(rc){
					break;
				}
			}else if(!strcmp(token, "user")){
				token = strtok_r(NULL, "", &saveptr);
				if(token){
					token = misc__trimblanks(token);
					if(slen == 0){
						log__printf(NULL, dimq_LOG_ERR, "Error: Missing username in acl_file \"%s\".", security_opts->acl_file);
						rc = dimq_ERR_INVAL;
						break;
					}
					dimq__free(user);
					user = dimq__strdup(token);
					if(!user){
						rc = dimq_ERR_NOMEM;
						break;
					}
				}else{
					log__printf(NULL, dimq_LOG_ERR, "Error: Missing username in acl_file \"%s\".", security_opts->acl_file);
					rc = dimq_ERR_INVAL;
					break;
				}
			}else{
				log__printf(NULL, dimq_LOG_ERR, "Error: Invalid line in acl_file \"%s\": %s.", security_opts->acl_file, buf);
				rc = dimq_ERR_INVAL;
				break;
			}
		}
	}

	dimq__free(buf);
	dimq__free(user);
	fclose(aclfptr);

	return rc;
}

static void free__acl(struct dimq__acl *acl)
{
	if(!acl) return;

	if(acl->next){
		free__acl(acl->next);
	}
	dimq__free(acl->topic);
	dimq__free(acl);
}


static void acl__cleanup_single(struct dimq__security_options *security_opts)
{
	struct dimq__acl_user *user_tail;

	while(security_opts->acl_list){
		user_tail = security_opts->acl_list->next;

		free__acl(security_opts->acl_list->acl);
		dimq__free(security_opts->acl_list->username);
		dimq__free(security_opts->acl_list);

		security_opts->acl_list = user_tail;
	}

	if(security_opts->acl_patterns){
		free__acl(security_opts->acl_patterns);
		security_opts->acl_patterns = NULL;
	}
}


static int acl__cleanup(bool reload)
{
	struct dimq *context, *ctxt_tmp = NULL;
	int i;

	UNUSED(reload);

	/* As we're freeing ACLs, we must clear context->acl_list to ensure no
	 * invalid memory accesses take place later.
	 * This *requires* the ACLs to be reapplied after acl__cleanup()
	 * is called if we are reloading the config. If this is not done, all 
	 * access will be denied to currently connected clients.
	 */
	HASH_ITER(hh_id, db.contexts_by_id, context, ctxt_tmp){
		context->acl_list = NULL;
	}

	if(db.config->per_listener_settings){
		for(i=0; i<db.config->listener_count; i++){
			acl__cleanup_single(&db.config->listeners[i].security_options);
		}
	}else{
		acl__cleanup_single(&db.config->security_options);
	}

	return dimq_ERR_SUCCESS;
}


int acl__find_acls(struct dimq *context)
{
	struct dimq__acl_user *acl_tail;
	struct dimq__security_options *security_opts;

	/* Associate user with its ACL, assuming we have ACLs loaded. */
	if(db.config->per_listener_settings){
		if(!context->listener){
			return dimq_ERR_INVAL;
		}
		security_opts = &context->listener->security_options;
	}else{
		security_opts = &db.config->security_options;
	}

	if(security_opts->acl_list){
		acl_tail = security_opts->acl_list;
		while(acl_tail){
			if(context->username){
				if(acl_tail->username && !strcmp(context->username, acl_tail->username)){
					context->acl_list = acl_tail;
					break;
				}
			}else{
				if(acl_tail->username == NULL){
					context->acl_list = acl_tail;
					break;
				}
			}
			acl_tail = acl_tail->next;
		}
	}else{
		context->acl_list = NULL;
	}

	return dimq_ERR_SUCCESS;
}


static int pwfile__parse(const char *file, struct dimq__unpwd **root)
{
	FILE *pwfile;
	struct dimq__unpwd *unpwd;
	char *username, *password;
	char *saveptr = NULL;
	char *buf;
	int buflen = 256;

	buf = dimq__malloc((size_t)buflen);
	if(buf == NULL){
		log__printf(NULL, dimq_LOG_ERR, "Error: Out of memory.");
		return dimq_ERR_NOMEM;
	}
	
	pwfile = dimq__fopen(file, "rt", false);
	if(!pwfile){
		log__printf(NULL, dimq_LOG_ERR, "Error: Unable to open pwfile \"%s\".", file);
		dimq__free(buf);
		return dimq_ERR_UNKNOWN;
	}

	while(!feof(pwfile)){
		if(fgets_extending(&buf, &buflen, pwfile)){
			if(buf[0] == '#') continue;
			if(!strchr(buf, ':')) continue;

			username = strtok_r(buf, ":", &saveptr);
			if(username){
				unpwd = dimq__calloc(1, sizeof(struct dimq__unpwd));
				if(!unpwd){
					fclose(pwfile);
					dimq__free(buf);
					return dimq_ERR_NOMEM;
				}
				username = misc__trimblanks(username);
				if(strlen(username) > 65535){
					log__printf(NULL, dimq_LOG_NOTICE, "Warning: Invalid line in password file '%s', username too long.", file);
					dimq__free(unpwd);
					continue;
				}

				unpwd->username = dimq__strdup(username);
				if(!unpwd->username){
					dimq__free(unpwd);
					dimq__free(buf);
					fclose(pwfile);
					return dimq_ERR_NOMEM;
				}
				password = strtok_r(NULL, ":", &saveptr);
				if(password){
					password = misc__trimblanks(password);

					if(strlen(password) > 65535){
						log__printf(NULL, dimq_LOG_NOTICE, "Warning: Invalid line in password file '%s', password too long.", file);
						dimq__free(unpwd->username);
						dimq__free(unpwd);
						continue;
					}

					unpwd->password = dimq__strdup(password);
					if(!unpwd->password){
						fclose(pwfile);
						dimq__free(unpwd->username);
						dimq__free(unpwd);
						dimq__free(buf);
						return dimq_ERR_NOMEM;
					}

					HASH_ADD_KEYPTR(hh, *root, unpwd->username, strlen(unpwd->username), unpwd);
				}else{
					log__printf(NULL, dimq_LOG_NOTICE, "Warning: Invalid line in password file '%s': %s", file, buf);
					dimq__free(unpwd->username);
					dimq__free(unpwd);
				}
			}
		}
	}
	fclose(pwfile);
	dimq__free(buf);

	return dimq_ERR_SUCCESS;
}


void unpwd__free_item(struct dimq__unpwd **unpwd, struct dimq__unpwd *item)
{
	dimq__free(item->username);
	dimq__free(item->password);
#ifdef WITH_TLS
	dimq__free(item->salt);
#endif
	HASH_DEL(*unpwd, item);
	dimq__free(item);
}


#ifdef WITH_TLS
static int unpwd__decode_passwords(struct dimq__unpwd **unpwd)
{
	struct dimq__unpwd *u, *tmp = NULL;
	char *token;
	unsigned char *salt;
	unsigned int salt_len;
	unsigned char *password;
	unsigned int password_len;
	int rc;
	enum dimq_pwhash_type hashtype;

	HASH_ITER(hh, *unpwd, u, tmp){
		/* Need to decode password into hashed data + salt. */
		if(u->password == NULL){
			log__printf(NULL, dimq_LOG_ERR, "Error: Missing password hash for user %s, removing entry.", u->username);
			unpwd__free_item(unpwd, u);
			continue;
		}

		token = strtok(u->password, "$");
		if(token == NULL){
			log__printf(NULL, dimq_LOG_ERR, "Error: Invalid password hash for user %s, removing entry.", u->username);
			unpwd__free_item(unpwd, u);
			continue;
		}

		if(!strcmp(token, "6")){
			hashtype = pw_sha512;
		}else if(!strcmp(token, "7")){
			hashtype = pw_sha512_pbkdf2;
		}else{
			log__printf(NULL, dimq_LOG_ERR, "Error: Invalid password hash type for user %s, removing entry.", u->username);
			unpwd__free_item(unpwd, u);
			continue;
		}

		if(hashtype == pw_sha512_pbkdf2){
			token = strtok(NULL, "$");
			if(token == NULL){
				log__printf(NULL, dimq_LOG_ERR, "Error: Invalid password hash for user %s, removing entry.", u->username);
				unpwd__free_item(unpwd, u);
				continue;
			}
			u->iterations = atoi(token);
			if(u->iterations < 1){
				log__printf(NULL, dimq_LOG_ERR, "Error: Invalid hash iterations for user %s, removing entry.", u->username);
				unpwd__free_item(unpwd, u);
				continue;
			}
		}

		token = strtok(NULL, "$");
		if(token == NULL){
			log__printf(NULL, dimq_LOG_ERR, "Error: Invalid password hash for user %s, removing entry.", u->username);
			unpwd__free_item(unpwd, u);
			continue;
		}
		rc = base64__decode(token, &salt, &salt_len);
		if(rc == dimq_ERR_SUCCESS && salt_len == 12){
			u->salt = salt;
			u->salt_len = salt_len;
			token = strtok(NULL, "$");
			if(token){
				rc = base64__decode(token, &password, &password_len);
				if(rc == dimq_ERR_SUCCESS && password_len == 64){
					dimq__free(u->password);
					u->password = (char *)password;
					u->password_len = password_len;
					u->hashtype = hashtype;
				}else{
					log__printf(NULL, dimq_LOG_ERR, "Error: Unable to decode password for user %s, removing entry.", u->username);
					unpwd__free_item(unpwd, u);
				}
			}else{
				log__printf(NULL, dimq_LOG_ERR, "Error: Invalid password hash for user %s, removing entry.", u->username);
				unpwd__free_item(unpwd, u);
			}
		}else{
			log__printf(NULL, dimq_LOG_ERR, "Error: Unable to decode password salt for user %s, removing entry.", u->username);
			unpwd__free_item(unpwd, u);
		}
	}

	return dimq_ERR_SUCCESS;
}
#endif


static int unpwd__file_parse(struct dimq__unpwd **unpwd, const char *password_file)
{
	int rc;
	if(!unpwd) return dimq_ERR_INVAL;

	if(!password_file) return dimq_ERR_SUCCESS;

	rc = pwfile__parse(password_file, unpwd);

#ifdef WITH_TLS
	if(rc) return rc;
	rc = unpwd__decode_passwords(unpwd);
#endif

	return rc;
}

static int psk__file_parse(struct dimq__unpwd **psk_id, const char *psk_file)
{
	int rc;
	struct dimq__unpwd *u, *tmp = NULL;

	if(!db.config || !psk_id) return dimq_ERR_INVAL;

	/* We haven't been asked to parse a psk file. */
	if(!psk_file) return dimq_ERR_SUCCESS;

	rc = pwfile__parse(psk_file, psk_id);
	if(rc) return rc;

	HASH_ITER(hh, (*psk_id), u, tmp){
		/* Check for hex only digits */
		if(!u->password){
			log__printf(NULL, dimq_LOG_ERR, "Error: Empty psk for identity \"%s\".", u->username);
			return dimq_ERR_INVAL;
		}
		if(strspn(u->password, "0123456789abcdefABCDEF") < strlen(u->password)){
			log__printf(NULL, dimq_LOG_ERR, "Error: psk for identity \"%s\" contains non-hexadecimal characters.", u->username);
			return dimq_ERR_INVAL;
		}
	}
	return dimq_ERR_SUCCESS;
}


#ifdef WITH_TLS
static int dimq__memcmp_const(const void *a, const void *b, size_t len)
{
	size_t i;
	int rc = 0;

	if(!a || !b) return 1;

	for(i=0; i<len; i++){
		if( ((char *)a)[i] != ((char *)b)[i] ){
			rc = 1;
		}
	}
	return rc;
}
#endif


static int dimq_unpwd_check_default(int event, void *event_data, void *userdata)
{
	struct dimq_evt_basic_auth *ed = event_data;
	struct dimq__unpwd *u;
	struct dimq__unpwd *unpwd_ref;
#ifdef WITH_TLS
	unsigned char hash[EVP_MAX_MD_SIZE];
	unsigned int hash_len;
	int rc;
#endif

	UNUSED(event);
	UNUSED(userdata);

	if(ed->client->username == NULL){
		return dimq_ERR_PLUGIN_DEFER;
	}

	if(db.config->per_listener_settings){
		if(ed->client->bridge) return dimq_ERR_SUCCESS;
		if(!ed->client->listener) return dimq_ERR_INVAL;
		unpwd_ref = ed->client->listener->security_options.unpwd;
	}else{
		unpwd_ref = db.config->security_options.unpwd;
	}

	HASH_FIND(hh, unpwd_ref, ed->client->username, strlen(ed->client->username), u);
	if(u){
		if(u->password){
			if(ed->client->password){
#ifdef WITH_TLS
				rc = pw__digest(ed->client->password, u->salt, u->salt_len, hash, &hash_len, u->hashtype, u->iterations);
				if(rc == dimq_ERR_SUCCESS){
					if(hash_len == u->password_len && !dimq__memcmp_const(u->password, hash, hash_len)){
						return dimq_ERR_SUCCESS;
					}else{
						return dimq_ERR_AUTH;
					}
				}else{
					return rc;
				}
#else
				if(!strcmp(u->password, ed->client->password)){
					return dimq_ERR_SUCCESS;
				}
#endif
			}else{
				return dimq_ERR_AUTH;
			}
		}else{
			return dimq_ERR_SUCCESS;
		}
	}

	return dimq_ERR_AUTH;
}

static int unpwd__cleanup(struct dimq__unpwd **root, bool reload)
{
	struct dimq__unpwd *u, *tmp = NULL;

	UNUSED(reload);

	if(!root) return dimq_ERR_INVAL;

	HASH_ITER(hh, *root, u, tmp){
		HASH_DEL(*root, u);
		dimq__free(u->password);
		dimq__free(u->username);
#ifdef WITH_TLS
		dimq__free(u->salt);
#endif
		dimq__free(u);
	}

	*root = NULL;

	return dimq_ERR_SUCCESS;
}


#ifdef WITH_TLS
static void security__disconnect_auth(struct dimq *context)
{
	if(context->protocol == dimq_p_mqtt5){
		send__disconnect(context, MQTT_RC_ADMINISTRATIVE_ACTION, NULL);
	}
	dimq__set_state(context, dimq_cs_disconnecting);
	do_disconnect(context, dimq_ERR_AUTH);
}
#endif

/* Apply security settings after a reload.
 * Includes:
 * - Disconnecting anonymous users if appropriate
 * - Disconnecting users with invalid passwords
 * - Reapplying ACLs
 */
int dimq_security_apply_default(void)
{
	struct dimq *context, *ctxt_tmp = NULL;
	struct dimq__acl_user *acl_user_tail;
	bool allow_anonymous;
	struct dimq__security_options *security_opts = NULL;
#ifdef WITH_TLS
	int i;
	X509 *client_cert = NULL;
	X509_NAME *name;
	X509_NAME_ENTRY *name_entry;
	ASN1_STRING *name_asn1 = NULL;
	struct dimq__listener *listener;
	BIO *subject_bio;
	char *data_start;
	size_t name_length;
	char *subject;
#endif

#ifdef WITH_TLS
	for(i=0; i<db.config->listener_count; i++){
		listener = &db.config->listeners[i];
		if(listener && listener->ssl_ctx && listener->certfile && listener->keyfile && listener->crlfile && listener->require_certificate){
			if(net__tls_server_ctx(listener)){
				return dimq_ERR_TLS;
			}

			if(net__tls_load_verify(listener)){
				return dimq_ERR_TLS;
			}
		}
	}
#endif

	HASH_ITER(hh_id, db.contexts_by_id, context, ctxt_tmp){
		if(context->bridge){
			continue;
		}

		/* Check for anonymous clients when allow_anonymous is false */
		if(db.config->per_listener_settings){
			if(context->listener){
				allow_anonymous = context->listener->security_options.allow_anonymous;
			}else{
				/* Client not currently connected, so defer judgement until it does connect */
				allow_anonymous = true;
			}
		}else{
			allow_anonymous = db.config->security_options.allow_anonymous;
		}

		if(!allow_anonymous && !context->username){
			dimq__set_state(context, dimq_cs_disconnecting);
			do_disconnect(context, dimq_ERR_AUTH);
			continue;
		}

		/* Check for connected clients that are no longer authorised */
#ifdef WITH_TLS
		if(context->listener && context->listener->ssl_ctx && (context->listener->use_identity_as_username || context->listener->use_subject_as_username)){
			/* Client must have either a valid certificate, or valid PSK used as a username. */
			if(!context->ssl){
				if(context->protocol == dimq_p_mqtt5){
					send__disconnect(context, MQTT_RC_ADMINISTRATIVE_ACTION, NULL);
				}
				dimq__set_state(context, dimq_cs_disconnecting);
				do_disconnect(context, dimq_ERR_AUTH);
				continue;
			}
#ifdef FINAL_WITH_TLS_PSK
			if(context->listener->psk_hint){
				/* Client should have provided an identity to get this far. */
				if(!context->username){
					security__disconnect_auth(context);
					continue;
				}
			}else
#endif /* FINAL_WITH_TLS_PSK */
			{
				/* Free existing credentials and then recover them. */
				dimq__free(context->username);
				context->username = NULL;
				dimq__free(context->password);
				context->password = NULL;

				client_cert = SSL_get_peer_certificate(context->ssl);
				if(!client_cert){
					security__disconnect_auth(context);
					continue;
				}
				name = X509_get_subject_name(client_cert);
				if(!name){
					X509_free(client_cert);
					client_cert = NULL;
					security__disconnect_auth(context);
					continue;
				}
				if (context->listener->use_identity_as_username) { /* use_identity_as_username */
					i = X509_NAME_get_index_by_NID(name, NID_commonName, -1);
					if(i == -1){
						X509_free(client_cert);
						client_cert = NULL;
						security__disconnect_auth(context);
						continue;
					}
					name_entry = X509_NAME_get_entry(name, i);
					if(name_entry){
						name_asn1 = X509_NAME_ENTRY_get_data(name_entry);
						if (name_asn1 == NULL) {
							X509_free(client_cert);
							client_cert = NULL;
							security__disconnect_auth(context);
							continue;
						}
#if OPENSSL_VERSION_NUMBER < 0x10100000L
						context->username = dimq__strdup((char *) ASN1_STRING_data(name_asn1));
#else
						context->username = dimq__strdup((char *) ASN1_STRING_get0_data(name_asn1));
#endif
						if(!context->username){
							X509_free(client_cert);
							client_cert = NULL;
							security__disconnect_auth(context);
							continue;
						}
						/* Make sure there isn't an embedded NUL character in the CN */
						if ((size_t)ASN1_STRING_length(name_asn1) != strlen(context->username)) {
							X509_free(client_cert);
							client_cert = NULL;
							security__disconnect_auth(context);
							continue;
						}
					}
				} else { /* use_subject_as_username */
					subject_bio = BIO_new(BIO_s_mem());
					X509_NAME_print_ex(subject_bio, X509_get_subject_name(client_cert), 0, XN_FLAG_RFC2253);
					data_start = NULL;
					name_length = (size_t)BIO_get_mem_data(subject_bio, &data_start);
					subject = dimq__malloc(sizeof(char)*name_length+1);
					if(!subject){
						BIO_free(subject_bio);
						X509_free(client_cert);
						client_cert = NULL;
						security__disconnect_auth(context);
						continue;
					}
					memcpy(subject, data_start, name_length);
					subject[name_length] = '\0';
					BIO_free(subject_bio);
					context->username = subject;
				}
				if(!context->username){
					X509_free(client_cert);
					client_cert = NULL;
					security__disconnect_auth(context);
					continue;
				}
				X509_free(client_cert);
				client_cert = NULL;
			}
		}else
#endif
		{
			/* Username/password check only if the identity/subject check not used */
			if(dimq_unpwd_check(context) != dimq_ERR_SUCCESS){
				dimq__set_state(context, dimq_cs_disconnecting);
				do_disconnect(context, dimq_ERR_AUTH);
				continue;
			}
		}


		/* Check for ACLs and apply to user. */
		if(db.config->per_listener_settings){
			if(context->listener){
				security_opts = &context->listener->security_options;
			}else{
				if(context->state != dimq_cs_active){
					dimq__set_state(context, dimq_cs_disconnecting);
					do_disconnect(context, dimq_ERR_AUTH);
					continue;
				}
			}
		}else{
			security_opts = &db.config->security_options;
		}

		if(security_opts && security_opts->acl_list){
			acl_user_tail = security_opts->acl_list;
			while(acl_user_tail){
				if(acl_user_tail->username){
					if(context->username){
						if(!strcmp(acl_user_tail->username, context->username)){
							context->acl_list = acl_user_tail;
							break;
						}
					}
				}else{
					if(!context->username){
						context->acl_list = acl_user_tail;
						break;
					}
				}
				acl_user_tail = acl_user_tail->next;
			}
		}
	}
	return dimq_ERR_SUCCESS;
}

int dimq_psk_key_get_default(struct dimq *context, const char *hint, const char *identity, char *key, int max_key_len)
{
	struct dimq__unpwd *u, *tmp = NULL;
	struct dimq__unpwd *psk_id_ref = NULL;

	if(!hint || !identity || !key) return dimq_ERR_INVAL;

	if(db.config->per_listener_settings){
		if(!context->listener) return dimq_ERR_INVAL;
		psk_id_ref = context->listener->security_options.psk_id;
	}else{
		psk_id_ref = db.config->security_options.psk_id;
	}
	if(!psk_id_ref) return dimq_ERR_PLUGIN_DEFER;

	HASH_ITER(hh, psk_id_ref, u, tmp){
		if(!strcmp(u->username, identity)){
			strncpy(key, u->password, (size_t)max_key_len);
			return dimq_ERR_SUCCESS;
		}
	}

	return dimq_ERR_AUTH;
}

#ifdef WITH_TLS
int pw__digest(const char *password, const unsigned char *salt, unsigned int salt_len, unsigned char *hash, unsigned int *hash_len, enum dimq_pwhash_type hashtype, int iterations)
{
	const EVP_MD *digest;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_MD_CTX context;
#else
	EVP_MD_CTX *context;
#endif

	digest = EVP_get_digestbyname("sha512");
	if(!digest){
		/* FIXME fprintf(stderr, "Error: Unable to create openssl digest.\n"); */
		return 1;
	}

	if(hashtype == pw_sha512){
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		EVP_MD_CTX_init(&context);
		EVP_DigestInit_ex(&context, digest, NULL);
		EVP_DigestUpdate(&context, password, strlen(password));
		EVP_DigestUpdate(&context, salt, salt_len);
		/* hash is assumed to be EVP_MAX_MD_SIZE bytes long. */
		EVP_DigestFinal_ex(&context, hash, hash_len);
		EVP_MD_CTX_cleanup(&context);
#else
		context = EVP_MD_CTX_new();
		EVP_DigestInit_ex(context, digest, NULL);
		EVP_DigestUpdate(context, password, strlen(password));
		EVP_DigestUpdate(context, salt, salt_len);
		/* hash is assumed to be EVP_MAX_MD_SIZE bytes long. */
		EVP_DigestFinal_ex(context, hash, hash_len);
		EVP_MD_CTX_free(context);
#endif
	}else{
		*hash_len = EVP_MAX_MD_SIZE;
		PKCS5_PBKDF2_HMAC(password, (int)strlen(password),
			salt, (int)salt_len, iterations,
			digest, (int)(*hash_len), hash);
	}

	return dimq_ERR_SUCCESS;
}

#endif
