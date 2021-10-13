/*
Copyright (c) 2020 Roger Light <roger@atchoo.org>

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

/*
 * Add an MQTT v5 user-property with key "timestamp" and value of timestamp in ISO-8601 format to all messages.
 *
 * Compile with:
 *   gcc -I<path to dimq-repo/include> -fPIC -shared dimq_timestamp.c -o dimq_timestamp.so
 *
 * Use in config with:
 *
 *   plugin /path/to/dimq_timestamp.so
 *
 * Note that this only works on dimq 2.0 or later.
 */
#include "config.h"

#include <stdio.h>
#include <time.h>

#include "dimq_broker.h"
#include "dimq_plugin.h"
#include "dimq.h"
#include "mqtt_protocol.h"

static dimq_plugin_id_t *dimq_pid = NULL;

static int callback_message(int event, void *event_data, void *userdata)
{
	struct dimq_evt_message *ed = event_data;
	struct timespec ts;
	struct tm *ti;
	char time_buf[25];

	UNUSED(event);
	UNUSED(userdata);

	clock_gettime(CLOCK_REALTIME, &ts);
	ti = gmtime(&ts.tv_sec);
	strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%SZ", ti);

	return dimq_property_add_string_pair(&ed->properties, MQTT_PROP_USER_PROPERTY, "timestamp", time_buf);
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
	return dimq_callback_register(dimq_pid, dimq_EVT_MESSAGE, callback_message, NULL, NULL);
}

int dimq_plugin_cleanup(void *user_data, struct dimq_opt *opts, int opt_count)
{
	UNUSED(user_data);
	UNUSED(opts);
	UNUSED(opt_count);

	return dimq_callback_unregister(dimq_pid, dimq_EVT_MESSAGE, callback_message, NULL);
}
