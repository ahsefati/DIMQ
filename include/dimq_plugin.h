/*
Copyright (c) 2012-2020 Roger Light <roger@atchoo.org>

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

#ifndef dimq_PLUGIN_H
#define dimq_PLUGIN_H

/*
 * File: dimq_plugin.h
 *
 * This header contains function declarations for use when writing a dimq plugin.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* The generic plugin interface starts at version 5 */
#define dimq_PLUGIN_VERSION 5

/* The old auth only interface stopped at version 4 */
#define dimq_AUTH_PLUGIN_VERSION 4

#define dimq_ACL_NONE 0x00
#define dimq_ACL_READ 0x01
#define dimq_ACL_WRITE 0x02
#define dimq_ACL_SUBSCRIBE 0x04
#define dimq_ACL_UNSUBSCRIBE 0x08

#include <stdbool.h>
#include <stdint.h>

struct dimq;

struct dimq_opt {
	char *key;
	char *value;
};

struct dimq_auth_opt {
	char *key;
	char *value;
};

struct dimq_acl_msg {
	const char *topic;
	const void *payload;
	long payloadlen;
	int qos;
	bool retain;
};

#ifdef WIN32
#  define dimq_plugin_EXPORT __declspec(dllexport)
#else
#  define dimq_plugin_EXPORT
#endif

/*
 * To create an authentication plugin you must include this file then implement
 * the functions listed in the "Plugin Functions" section below. The resulting
 * code should then be compiled as a shared library. Using gcc this can be
 * achieved as follows:
 *
 * gcc -I<path to dimq_plugin.h> -fPIC -shared plugin.c -o plugin.so
 *
 * On Mac OS X:
 *
 * gcc -I<path to dimq_plugin.h> -fPIC -shared plugin.c -undefined dynamic_lookup -o plugin.so
 *
 * Authentication plugins can implement one or both of authentication and
 * access control. If your plugin does not wish to handle either of
 * authentication or access control it should return dimq_ERR_PLUGIN_DEFER. In
 * this case, the next plugin will handle it. If all plugins return
 * dimq_ERR_PLUGIN_DEFER, the request will be denied.
 *
 * For each check, the following flow happens:
 *
 * * The default password file and/or acl file checks are made. If either one
 *   of these is not defined, then they are considered to be deferred. If either
 *   one accepts the check, no further checks are made. If an error occurs, the
 *   check is denied
 * * The first plugin does the check, if it returns anything other than
 *   dimq_ERR_PLUGIN_DEFER, then the check returns immediately. If the plugin
 *   returns dimq_ERR_PLUGIN_DEFER then the next plugin runs its check.
 * * If the final plugin returns dimq_ERR_PLUGIN_DEFER, then access will be
 *   denied.
 */

/* =========================================================================
 *
 * Helper Functions
 *
 * ========================================================================= */

/* There are functions that are available for plugin developers to use in
 * dimq_broker.h, including logging and accessor functions.
 */


/* =========================================================================
 *
 * Section: Plugin Functions v5
 *
 * This is the plugin version 5 interface, which covers authentication, access
 * control, the $CONTROL topic space handling, and message inspection and
 * modification.
 *
 * This interface is available from v2.0 onwards.
 *
 * There are just three functions to implement in your plugin. You should
 * register callbacks to handle different events in your
 * dimq_plugin_init() function. See dimq_broker.h for the events and
 * callback registering functions.
 *
 * ========================================================================= */

/*
 * Function: dimq_plugin_version
 *
 * The broker will attempt to call this function immediately after loading the
 * plugin to check it is a supported plugin version. Your code must simply
 * return the plugin interface version you support, i.e. 5.
 *
 * The supported_versions array tells you which plugin versions the broker supports.
 *
 * If the broker does not support the version that you require, return -1 to
 * indicate failure.
 */
dimq_plugin_EXPORT int dimq_plugin_version(int supported_version_count, const int *supported_versions);

/*
 * Function: dimq_plugin_init
 *
 * Called after the plugin has been loaded and <dimq_plugin_version>
 * has been called. This will only ever be called once and can be used to
 * initialise the plugin.
 *
 * Parameters:
 *
 *  identifier -     This is a pointer to an opaque structure which you must
 *                   save and use when registering/unregistering callbacks.
 *	user_data -      The pointer set here will be passed to the other plugin
 *	                 functions. Use to hold connection information for example.
 *	opts -           Pointer to an array of struct dimq_opt, which
 *	                 provides the plugin options defined in the configuration file.
 *	opt_count -      The number of elements in the opts array.
 *
 * Return value:
 *	Return 0 on success
 *	Return >0 on failure.
 */
dimq_plugin_EXPORT int dimq_plugin_init(dimq_plugin_id_t *identifier, void **userdata, struct dimq_opt *options, int option_count);


/*
 * Function: dimq_plugin_cleanup
 *
 * Called when the broker is shutting down. This will only ever be called once
 * per plugin.
 *
 * Parameters:
 *
 *	user_data -      The pointer provided in <dimq_plugin_init>.
 *	opts -           Pointer to an array of struct dimq_opt, which
 *	                 provides the plugin options defined in the configuration file.
 *	opt_count -      The number of elements in the opts array.
 *
 * Return value:
 *	Return 0 on success
 *	Return >0 on failure.
 */
dimq_plugin_EXPORT int dimq_plugin_cleanup(void *userdata, struct dimq_opt *options, int option_count);



/* =========================================================================
 *
 * Section: Plugin Functions v4
 *
 * This is the plugin version 4 interface, which is exclusively for
 * authentication and access control, and which is still supported for existing
 * plugins. If you are developing a new plugin, please use the v5 interface.
 *
 * You must implement these functions in your plugin.
 *
 * ========================================================================= */

/*
 * Function: dimq_auth_plugin_version
 *
 * The broker will call this function immediately after loading the plugin to
 * check it is a supported plugin version. Your code must simply return
 * the version of the plugin interface you support, i.e. 4.
 */
dimq_plugin_EXPORT int dimq_auth_plugin_version(void);


/*
 * Function: dimq_auth_plugin_init
 *
 * Called after the plugin has been loaded and <dimq_auth_plugin_version>
 * has been called. This will only ever be called once and can be used to
 * initialise the plugin.
 *
 * Parameters:
 *
 *	user_data -      The pointer set here will be passed to the other plugin
 *	                 functions. Use to hold connection information for example.
 *	opts -           Pointer to an array of struct dimq_opt, which
 *	                 provides the plugin options defined in the configuration file.
 *	opt_count -      The number of elements in the opts array.
 *
 * Return value:
 *	Return 0 on success
 *	Return >0 on failure.
 */
dimq_plugin_EXPORT int dimq_auth_plugin_init(void **user_data, struct dimq_opt *opts, int opt_count);


/*
 * Function: dimq_auth_plugin_cleanup
 *
 * Called when the broker is shutting down. This will only ever be called once
 * per plugin.
 * Note that <dimq_auth_security_cleanup> will be called directly before
 * this function.
 *
 * Parameters:
 *
 *	user_data -      The pointer provided in <dimq_auth_plugin_init>.
 *	opts -           Pointer to an array of struct dimq_opt, which
 *	                 provides the plugin options defined in the configuration file.
 *	opt_count -      The number of elements in the opts array.
 *
 * Return value:
 *	Return 0 on success
 *	Return >0 on failure.
 */
dimq_plugin_EXPORT int dimq_auth_plugin_cleanup(void *user_data, struct dimq_opt *opts, int opt_count);


/*
 * Function: dimq_auth_security_init
 *
 * This function is called in two scenarios:
 *
 * 1. When the broker starts up.
 * 2. If the broker is requested to reload its configuration whilst running. In
 *    this case, <dimq_auth_security_cleanup> will be called first, then
 *    this function will be called.  In this situation, the reload parameter
 *    will be true.
 *
 * Parameters:
 *
 *	user_data -      The pointer provided in <dimq_auth_plugin_init>.
 *	opts -           Pointer to an array of struct dimq_opt, which
 *	                 provides the plugin options defined in the configuration file.
 *	opt_count -      The number of elements in the opts array.
 *	reload -         If set to false, this is the first time the function has
 *	                 been called. If true, the broker has received a signal
 *	                 asking to reload its configuration.
 *
 * Return value:
 *	Return 0 on success
 *	Return >0 on failure.
 */
dimq_plugin_EXPORT int dimq_auth_security_init(void *user_data, struct dimq_opt *opts, int opt_count, bool reload);


/* 
 * Function: dimq_auth_security_cleanup
 *
 * This function is called in two scenarios:
 *
 * 1. When the broker is shutting down.
 * 2. If the broker is requested to reload its configuration whilst running. In
 *    this case, this function will be called, followed by
 *    <dimq_auth_security_init>. In this situation, the reload parameter
 *    will be true.
 *
 * Parameters:
 *
 *	user_data -      The pointer provided in <dimq_auth_plugin_init>.
 *	opts -           Pointer to an array of struct dimq_opt, which
 *	                 provides the plugin options defined in the configuration file.
 *	opt_count -      The number of elements in the opts array.
 *	reload -         If set to false, this is the first time the function has
 *	                 been called. If true, the broker has received a signal
 *	                 asking to reload its configuration.
 *
 * Return value:
 *	Return 0 on success
 *	Return >0 on failure.
 */
dimq_plugin_EXPORT int dimq_auth_security_cleanup(void *user_data, struct dimq_opt *opts, int opt_count, bool reload);


/*
 * Function: dimq_auth_acl_check
 *
 * Called by the broker when topic access must be checked. access will be one
 * of:
 *  dimq_ACL_SUBSCRIBE when a client is asking to subscribe to a topic string.
 *                     This differs from dimq_ACL_READ in that it allows you to
 *                     deny access to topic strings rather than by pattern. For
 *                     example, you may use dimq_ACL_SUBSCRIBE to deny
 *                     subscriptions to '#', but allow all topics in
 *                     dimq_ACL_READ. This allows clients to subscribe to any
 *                     topic they want, but not discover what topics are in use
 *                     on the server.
 *  dimq_ACL_READ      when a message is about to be sent to a client (i.e. whether
 *                     it can read that topic or not).
 *  dimq_ACL_WRITE     when a message has been received from a client (i.e. whether
 *                     it can write to that topic or not).
 *
 * Return:
 *	dimq_ERR_SUCCESS if access was granted.
 *	dimq_ERR_ACL_DENIED if access was not granted.
 *	dimq_ERR_UNKNOWN for an application specific error.
 *	dimq_ERR_PLUGIN_DEFER if your plugin does not wish to handle this check.
 */
dimq_plugin_EXPORT int dimq_auth_acl_check(void *user_data, int access, struct dimq *client, const struct dimq_acl_msg *msg);


/*
 * Function: dimq_auth_unpwd_check
 *
 * This function is OPTIONAL. Only include this function in your plugin if you
 * are making basic username/password checks.
 *
 * Called by the broker when a username/password must be checked.
 *
 * Return:
 *	dimq_ERR_SUCCESS if the user is authenticated.
 *	dimq_ERR_AUTH if authentication failed.
 *	dimq_ERR_UNKNOWN for an application specific error.
 *	dimq_ERR_PLUGIN_DEFER if your plugin does not wish to handle this check.
 */
dimq_plugin_EXPORT int dimq_auth_unpwd_check(void *user_data, struct dimq *client, const char *username, const char *password);


/*
 * Function: dimq_psk_key_get
 *
 * This function is OPTIONAL. Only include this function in your plugin if you
 * are making TLS-PSK checks.
 *
 * Called by the broker when a client connects to a listener using TLS/PSK.
 * This is used to retrieve the pre-shared-key associated with a client
 * identity.
 *
 * Examine hint and identity to determine the required PSK (which must be a
 * hexadecimal string with no leading "0x") and copy this string into key.
 *
 * Parameters:
 *	user_data -   the pointer provided in <dimq_auth_plugin_init>.
 *	hint -        the psk_hint for the listener the client is connecting to.
 *	identity -    the identity string provided by the client
 *	key -         a string where the hex PSK should be copied
 *	max_key_len - the size of key
 *
 * Return value:
 *	Return 0 on success.
 *	Return >0 on failure.
 *	Return dimq_ERR_PLUGIN_DEFER if your plugin does not wish to handle this check.
 */
dimq_plugin_EXPORT int dimq_auth_psk_key_get(void *user_data, struct dimq *client, const char *hint, const char *identity, char *key, int max_key_len);

/*
 * Function: dimq_auth_start
 *
 * This function is OPTIONAL. Only include this function in your plugin if you
 * are making extended authentication checks.
 *
 * Parameters:
 *	user_data -   the pointer provided in <dimq_auth_plugin_init>.
 *	method - the authentication method
 *	reauth - this is set to false if this is the first authentication attempt
 *	         on a connection, set to true if the client is attempting to
 *	         reauthenticate.
 *	data_in - pointer to authentication data, or NULL
 *	data_in_len - length of data_in, in bytes
 *	data_out - if your plugin wishes to send authentication data back to the
 *	           client, allocate some memory using malloc or friends and set
 *	           data_out. The broker will free the memory after use.
 *	data_out_len - Set the length of data_out in bytes.
 *
 * Return value:
 *	Return dimq_ERR_SUCCESS if authentication was successful.
 *	Return dimq_ERR_AUTH_CONTINUE if the authentication is a multi step process and can continue.
 *	Return dimq_ERR_AUTH if authentication was valid but did not succeed.
 *	Return any other relevant positive integer dimq_ERR_* to produce an error.
 */
dimq_plugin_EXPORT int dimq_auth_start(void *user_data, struct dimq *client, const char *method, bool reauth, const void *data_in, uint16_t data_in_len, void **data_out, uint16_t *data_out_len);

dimq_plugin_EXPORT int dimq_auth_continue(void *user_data, struct dimq *client, const char *method, const void *data_in, uint16_t data_in_len, void **data_out, uint16_t *data_out_len);


#ifdef __cplusplus
}
#endif

#endif
