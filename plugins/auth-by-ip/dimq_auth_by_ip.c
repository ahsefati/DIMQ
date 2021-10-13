/*
Copyright (c) 2021 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License 2.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   https://www.eclipse.org/legal/epl-2.0/
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
SPDX-License-Identifier: EPL-2.0 OR EDL-1.0

Contributors:
   Roger Light - initial implementation and documentation.
*/

/*
 * This is an example plugin showing how to use the basic authentication
 * callback to allow/disallow client connections based on client IP addresses.
 *
 * This is an extremely basic type of access control, password based or similar
 * authentication is preferred.
 *
 * Compile with:
 *   gcc -I<path to dimq-repo/include> -fPIC -shared dimq_auth_by_ip.c -o dimq_auth_by_ip.so
 *
 * Use in config with:
 *
 *   plugin /path/to/dimq_auth_by_ip.so
 *
 * Note that this only works on dimq 2.0 or later.
 */
#include "config.h"

#include <stdio.h>
#include <string.h>

#include "dimq_broker.h"
#include "dimq_plugin.h"
#include "dimq.h"
#include "mqtt_protocol.h"

static dimq_plugin_id_t *dimq_pid = NULL;

static int basic_auth_callback(int event, void *event_data, void *userdata)
{
	struct dimq_evt_basic_auth *ed = event_data;
	const char *ip_address;

	UNUSED(event);
	UNUSED(userdata);

	ip_address = dimq_client_address(ed->client);
	if(!strcmp(ip_address, "127.0.0.1")){
		/* Only allow connections from localhost */
		return dimq_ERR_SUCCESS;
	}else{
		return dimq_ERR_AUTH;
	}
}

int dimq_plugin_version(int supported_version_count, const int *supported_versions)
{
	int i;

	for(i=0; i<supported_version_count; i++){
		if(supported_versions[i] == 5){
			return 5;
		}
	}
	return -1;
}

int dimq_plugin_init(dimq_plugin_id_t *identifier, void **user_data, struct dimq_opt *opts, int opt_count)
{
	UNUSED(user_data);
	UNUSED(opts);
	UNUSED(opt_count);

	dimq_pid = identifier;
	return dimq_callback_register(dimq_pid, dimq_EVT_BASIC_AUTH, basic_auth_callback, NULL, NULL);
}

int dimq_plugin_cleanup(void *user_data, struct dimq_opt *opts, int opt_count)
{
	UNUSED(user_data);
	UNUSED(opts);
	UNUSED(opt_count);

	return dimq_callback_unregister(dimq_pid, dimq_EVT_BASIC_AUTH, basic_auth_callback, NULL);
}
