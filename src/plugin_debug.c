/*
Copyright (c) 2015-2020 Roger Light <roger@atchoo.org>

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

/* This is a skeleton authentication and access control plugin that print a
 * message when each check occurs. It allows everything. */

#include <stdio.h>

#include "dimq_broker.h"
#include "dimq_plugin.h"
#include "dimq.h"

#define ANSI_GREEN "\e[0;32m"
#define ANSI_BLUE "\e[0;34m"
#define ANSI_MAGENTA "\e[0;35m"
#define ANSI_RESET "\e[0m"

void print_col(struct dimq *client)
{
	switch(dimq_client_protocol(client)){
		case mp_mqtt:
			printf("%s", ANSI_GREEN);
			break;
		case mp_websockets:
			printf("%s", ANSI_MAGENTA);
			break;
		default:
			break;
	}
}

int dimq_auth_plugin_version(void)
{
	printf(ANSI_BLUE "PLUGIN ::: dimq_auth_plugin_version()" ANSI_RESET "\n");
	return 4;
}

int dimq_auth_plugin_init(void **user_data, struct dimq_opt *auth_opts, int auth_opt_count)
{
	printf(ANSI_BLUE "PLUGIN ::: dimq_auth_plugin_init(,,%d)" ANSI_RESET "\n", auth_opt_count);
	return dimq_ERR_SUCCESS;
}

int dimq_auth_plugin_cleanup(void *user_data, struct dimq_opt *auth_opts, int auth_opt_count)
{
	printf(ANSI_BLUE "PLUGIN ::: dimq_auth_plugin_cleanup(,,%d)" ANSI_RESET "\n", auth_opt_count);
	return dimq_ERR_SUCCESS;
}

int dimq_auth_security_init(void *user_data, struct dimq_opt *auth_opts, int auth_opt_count, bool reload)
{
	printf(ANSI_BLUE "PLUGIN ::: dimq_auth_security_init(,,%d, %d)" ANSI_RESET "\n", auth_opt_count, reload);
	return dimq_ERR_SUCCESS;
}

int dimq_auth_security_cleanup(void *user_data, struct dimq_opt *auth_opts, int auth_opt_count, bool reload)
{
	printf(ANSI_BLUE "PLUGIN ::: dimq_auth_security_cleanup(,,%d, %d)" ANSI_RESET "\n", auth_opt_count, reload);
	return dimq_ERR_SUCCESS;
}

int dimq_auth_acl_check(void *user_data, int access, struct dimq *client, const struct dimq_acl_msg *msg)
{
	print_col(client);
	printf("PLUGIN ::: dimq_auth_acl_check(%p, %d, %s, %s)" ANSI_RESET "\n",
			user_data, access, dimq_client_username(client), msg->topic);
	return dimq_ERR_SUCCESS;
}

int dimq_auth_unpwd_check(void *user_data, struct dimq *client, const char *username, const char *password)
{
	print_col(client);
	printf("PLUGIN ::: dimq_auth_unpwd_check(%p, %s, %s)" ANSI_RESET "\n",
			user_data, dimq_client_username(client), username);
	return dimq_ERR_SUCCESS;
}

int dimq_auth_psk_key_get(void *user_data, struct dimq *client, const char *hint, const char *identity, char *key, int max_key_len)
{
	print_col(client);
	printf("PLUGIN ::: dimq_auth_psk_key_get(%p, %s, %s)" ANSI_RESET "\n",
			user_data, dimq_client_username(client), hint);
	return dimq_ERR_SUCCESS;
}

int dimq_auth_start(void *user_data, struct dimq *client, const char *method, bool reauth, const void *data_in, uint16_t data_in_len, void **data_out, uint16_t *data_out_len)
{
	print_col(client);
	printf("PLUGIN ::: dimq_auth_start(%p, %s, %s, %d, %d, %hn)" ANSI_RESET "\n",
			user_data, dimq_client_username(client), method, reauth, data_in_len, data_out_len);
	return dimq_ERR_SUCCESS;
}

int dimq_auth_continue(void *user_data, struct dimq *client, const char *method, const void *data_in, uint16_t data_in_len, void **data_out, uint16_t *data_out_len)
{
	print_col(client);
	printf("PLUGIN ::: dimq_auth_continue(%p, %s, %s, %d, %hn)" ANSI_RESET "\n",
			user_data, dimq_client_username(client), method, data_in_len, data_out_len);
	return dimq_ERR_SUCCESS;
}
