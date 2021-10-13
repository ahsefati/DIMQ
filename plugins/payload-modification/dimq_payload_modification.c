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
 * This is an *example* plugin which demonstrates how to modify the payload of
 * a message after it is received by the broker and before it is sent on to
 * other clients. 
 *
 * You should be very sure of what you are doing before making use of this feature.
 *
 * Compile with:
 *   gcc -I<path to dimq-repo/include> -fPIC -shared dimq_payload_modification.c -o dimq_payload_modification.so
 *
 * Use in config with:
 *
 *   plugin /path/to/dimq_payload_modification.so
 *
 * Note that this only works on dimq 2.0 or later.
 */
#include <stdio.h>
#include <string.h>

#include "dimq_broker.h"
#include "dimq_plugin.h"
#include "dimq.h"
#include "mqtt_protocol.h"

#define UNUSED(A) (void)(A)

static dimq_plugin_id_t *dimq_pid = NULL;

static int callback_message(int event, void *event_data, void *userdata)
{
	struct dimq_evt_message *ed = event_data;
	char *new_payload;
	uint32_t new_payloadlen;

	UNUSED(event);
	UNUSED(userdata);

	/* This simply adds "hello " to the front of every payload. You can of
	 * course do much more complicated message processing if needed. */

	/* Calculate the length of our new payload */
	new_payloadlen = ed->payloadlen + (uint32_t)strlen("hello ")+1;

	/* Allocate some memory - use
	 * dimq_calloc/dimq_malloc/dimq_strdup when allocating, to
	 * allow the broker to track memory usage */
	new_payload = dimq_calloc(1, new_payloadlen);
	if(new_payload == NULL){
		return dimq_ERR_NOMEM;
	}

	/* Print "hello " to the payload */
	snprintf(new_payload, new_payloadlen, "hello ");
	memcpy(new_payload+(uint32_t)strlen("hello "), ed->payload, ed->payloadlen);

	/* Assign the new payload and payloadlen to the event data structure. You
	 * must *not* free the original payload, it will be handled by the
	 * broker. */
	ed->payload = new_payload;
	ed->payloadlen = new_payloadlen;
	
	return dimq_ERR_SUCCESS;
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
