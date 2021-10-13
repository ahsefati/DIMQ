/*
Copyright (c) 2012-2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License 2.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   https://www.eclipse.org/legal/epl-2.0/
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#ifndef dimq_PLUGIN_H
#define dimq_PLUGIN_H

#define dimq_AUTH_PLUGIN_VERSION 2

#define dimq_ACL_NONE 0x00
#define dimq_ACL_READ 0x01
#define dimq_ACL_WRITE 0x02

struct dimq_auth_opt {
	char *key;
	char *value;
};

/*
 * To create an authentication plugin you must include this file then implement
 * the functions listed below. The resulting code should then be compiled as a
 * shared library. Using gcc this can be achieved as follows:
 *
 * gcc -I<path to dimq_plugin.h> -fPIC -shared plugin.c -o plugin.so
 *
 * On Mac OS X:
 *
 * gcc -I<path to dimq_plugin.h> -fPIC -shared plugin.c -undefined dynamic_lookup -o plugin.so
 *
 */

/* =========================================================================
 *
 * Utility Functions
 *
 * Use these functions from within your plugin.
 *
 * There are also very useful functions in libdimq.
 *
 * ========================================================================= */

/*
 * Function: dimq_log_printf
 *
 * Write a log message using the broker configured logging.
 *
 * Parameters:
 * 	level -    Log message priority. Can currently be one of:
 *
 *             dimq_LOG_INFO
 *             dimq_LOG_NOTICE
 *             dimq_LOG_WARNING
 *             dimq_LOG_ERR
 *             dimq_LOG_DEBUG
 *             dimq_LOG_SUBSCRIBE (not recommended for use by plugins)
 *             dimq_LOG_UNSUBSCRIBE (not recommended for use by plugins)
 *
 *             These values are defined in dimq.h.
 *
 *	fmt, ... - printf style format and arguments.
 */
void dimq_log_printf(int level, const char *fmt, ...);



/* =========================================================================
 *
 * Plugin Functions
 *
 * You must implement these functions in your plugin.
 *
 * ========================================================================= */

/*
 * Function: dimq_auth_plugin_version
 *
 * The broker will call this function immediately after loading the plugin to
 * check it is a supported plugin version. Your code must simply return
 * dimq_AUTH_PLUGIN_VERSION.
 */
int dimq_auth_plugin_version(void);

/*
 * Function: dimq_auth_plugin_init
 *
 * Called after the plugin has been loaded and <dimq_auth_plugin_version>
 * has been called. This will only ever be called once and can be used to
 * initialise the plugin.
 *
 * Parameters:
 *
 *	user_data :      The pointer set here will be passed to the other plugin
 *	                 functions. Use to hold connection information for example.
 *	auth_opts :      Pointer to an array of struct dimq_auth_opt, which
 *	                 provides the plugin options defined in the configuration file.
 *	auth_opt_count : The number of elements in the auth_opts array.
 *
 * Return value:
 *	Return 0 on success
 *	Return >0 on failure.
 */
int dimq_auth_plugin_init(void **user_data, struct dimq_auth_opt *auth_opts, int auth_opt_count);

/*
 * Function: dimq_auth_plugin_cleanup
 *
 * Called when the broker is shutting down. This will only ever be called once.
 * Note that <dimq_auth_security_cleanup> will be called directly before
 * this function.
 *
 * Parameters:
 *
 *	user_data :      The pointer provided in <dimq_auth_plugin_init>.
 *	auth_opts :      Pointer to an array of struct dimq_auth_opt, which
 *	                 provides the plugin options defined in the configuration file.
 *	auth_opt_count : The number of elements in the auth_opts array.
 *
 * Return value:
 *	Return 0 on success
 *	Return >0 on failure.
 */
int dimq_auth_plugin_cleanup(void *user_data, struct dimq_auth_opt *auth_opts, int auth_opt_count);

/*
 * Function: dimq_auth_security_init
 *
 * Called when the broker initialises the security functions when it starts up.
 * If the broker is requested to reload its configuration whilst running,
 * <dimq_auth_security_cleanup> will be called, followed by this function.
 * In this situation, the reload parameter will be true.
 *
 * Parameters:
 *
 *	user_data :      The pointer provided in <dimq_auth_plugin_init>.
 *	auth_opts :      Pointer to an array of struct dimq_auth_opt, which
 *	                 provides the plugin options defined in the configuration file.
 *	auth_opt_count : The number of elements in the auth_opts array.
 *	reload :         If set to false, this is the first time the function has
 *	                 been called. If true, the broker has received a signal
 *	                 asking to reload its configuration.
 *
 * Return value:
 *	Return 0 on success
 *	Return >0 on failure.
 */
int dimq_auth_security_init(void *user_data, struct dimq_auth_opt *auth_opts, int auth_opt_count, bool reload);

/* 
 * Function: dimq_auth_security_cleanup
 *
 * Called when the broker cleans up the security functions when it shuts down.
 * If the broker is requested to reload its configuration whilst running,
 * this function will be called, followed by <dimq_auth_security_init>.
 * In this situation, the reload parameter will be true.
 *
 * Parameters:
 *
 *	user_data :      The pointer provided in <dimq_auth_plugin_init>.
 *	auth_opts :      Pointer to an array of struct dimq_auth_opt, which
 *	                 provides the plugin options defined in the configuration file.
 *	auth_opt_count : The number of elements in the auth_opts array.
 *	reload :         If set to false, this is the first time the function has
 *	                 been called. If true, the broker has received a signal
 *	                 asking to reload its configuration.
 *
 * Return value:
 *	Return 0 on success
 *	Return >0 on failure.
 */
int dimq_auth_security_cleanup(void *user_data, struct dimq_auth_opt *auth_opts, int auth_opt_count, bool reload);

/*
 * Function: dimq_auth_acl_check
 *
 * Called by the broker when topic access must be checked. access will be one
 * of dimq_ACL_READ (for subscriptions) or dimq_ACL_WRITE (for publish). Return
 * dimq_ERR_SUCCESS if access was granted, dimq_ERR_ACL_DENIED if access was
 * not granted, or dimq_ERR_UNKNOWN for an application specific error.
 */
int dimq_auth_acl_check(void *user_data, const char *clientid, const char *username, const char *topic, int access);

/*
 * Function: dimq_auth_unpwd_check
 *
 * Called by the broker when a username/password must be checked. Return
 * dimq_ERR_SUCCESS if the user is authenticated, dimq_ERR_AUTH if
 * authentication failed, or dimq_ERR_UNKNOWN for an application specific
 * error.
 */
int dimq_auth_unpwd_check(void *user_data, const char *username, const char *password);

/*
 * Function: dimq_psk_key_get
 *
 * Called by the broker when a client connects to a listener using TLS/PSK.
 * This is used to retrieve the pre-shared-key associated with a client
 * identity.
 *
 * Examine hint and identity to determine the required PSK (which must be a
 * hexadecimal string with no leading "0x") and copy this string into key.
 *
 * Parameters:
 *	user_data :   the pointer provided in <dimq_auth_plugin_init>.
 *	hint :        the psk_hint for the listener the client is connecting to.
 *	identity :    the identity string provided by the client
 *	key :         a string where the hex PSK should be copied
 *	max_key_len : the size of key
 *
 * Return value:
 *	Return 0 on success.
 *	Return >0 on failure.
 *	Return >0 if this function is not required.
 */
int dimq_auth_psk_key_get(void *user_data, const char *hint, const char *identity, char *key, int max_key_len);

#endif
